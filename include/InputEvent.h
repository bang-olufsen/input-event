// Copyright (c) 2021 Bang & Olufsen a/s
//
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <unistd.h>

namespace Linux::Input {

using InputEventList = std::vector<uint16_t>;
using InputEventCallback = std::function<void(input_event& event)>;
using InputEventDescriptors = std::vector<int>;

/// Small header-only library for handling Linux input events
///
/// Example:
///
/// #include <InputEvent.h>
///
/// Linux::Input::InputEvent inputEvent;
/// int errorCode = inputEvent.subscribe({ EV_KEY }, { KEY_SPACE },
/// [](input_event& event){
///     if (event.code == KEY_SPACE)
///         std::cout << "Space button " << (event.value ? "pressed" :
///         "released") << std::endl;
///     else if (event.code == UINT16_MAX)
///         std::cerr << "Failed to read input events with error " <<
///         event.value << std::endl;
/// });
/// if (errorCode)
///     std::cerr << "Failed to subscribe for input events with error " <<
///     errorCode << std::endl;

class InputEvent {
public:
    /// @brief InputEvent constructor
    /// @param inputEventPrefix The input event prefix to use (default
    /// /dev/input/event)
    /// @param maxInputEvents The number of /dev/input/eventX files to monitor
    /// (default 10)
    InputEvent(const std::string& inputEventPrefix = "/dev/input/event", const uint8_t maxInputEvents = 10)
        : m_inputEventPrefix(inputEventPrefix)
        , m_maxInputEvents(maxInputEvents)
    {
    }
    ~InputEvent() { unsubscribe(); }

    /// @brief Subscribe for input events matching the specified types and codes
    /// In case of errors an input_event with type UINT16_MAX and code UINT16_MAX
    /// will be injected with value set to the errno value. At the same time the
    /// thread is stopped and a new subscribe function call is required.
    /// @note It is only possible to call subscribe once per instance
    /// @param eventTypes A std::vector with event types from
    /// <linux/input-event-codes.h>. Use UINT16_MAX for all types
    /// @param eventCodes A std::vector with event codes from
    /// <linux/input-event-codes.h>. Use UINT16_MAX for all codes
    /// @param eventCallback A callback to be invoked when an event matching the
    /// specified event types and codes is received
    /// @return An int with the result (0 on success or or a negative value from
    /// errno.h)
    int subscribe(const InputEventList eventTypes, const InputEventList eventCodes, const InputEventCallback eventCallback)
    {
        int result = -EINVAL;
        InputEventDescriptors inputDescriptors;

        unsubscribe();

        if (!eventTypes.size() || !eventCodes.size() || !eventCallback)
            return result;

        for (uint8_t inputEvent = 0; inputEvent < m_maxInputEvents; ++inputEvent) {
            auto device = m_inputEventPrefix + std::to_string(inputEvent);
            int result = open(device.c_str(), O_RDONLY);
            if (result > 0)
                inputDescriptors.push_back(result);
        }

        if (inputDescriptors.empty())
            return result;

        m_stopThread.store(false);
        m_thread = std::thread([this, eventTypes, eventCodes, eventCallback, inputDescriptors]() {
            input_event event;
            InputEventDescriptors inputEventDescriptors;

            while (!m_stopThread.load()) {
                int result = pollForInputEvent(inputDescriptors, inputEventDescriptors);
                if (result >= 0) {
                    for (auto inputDescriptor : inputEventDescriptors) {
                        if (read(inputDescriptor, &event, sizeof(event)) != sizeof(event))
                            continue;

                        for (const auto type : eventTypes) {
                            if (type == event.type || type == UINT16_MAX) {
                                for (const auto code : eventCodes) {
                                    if (code == event.code || code == UINT16_MAX)
                                        eventCallback(event);
                                }
                            }
                        }
                    }
                    inputEventDescriptors.clear();
                } else {
                    event = { 0, 0, UINT16_MAX, UINT16_MAX, errno };
                    eventCallback(event);
                    m_stopThread.store(true);
                }
            }

            for (const auto& inputDescriptor : inputDescriptors)
                close(inputDescriptor);
        });

        return 0;
    }

    /// @brief Unsubscribe for input events
    /// Stops the monitor thread. A new subscribe function call is required
    /// afterwards.
    void unsubscribe()
    {
        m_stopThread.store(true);

        if (m_thread.joinable())
            m_thread.join();
    }

private:
    /// @brief Polls for input events on the specified input descriptors
    /// @param[in] inputDescriptors A reference to an InputEventDescriptors object
    /// with input descriptors
    /// @param[out] inputEventDescriptors A reference to an InputEventDescriptors
    /// object to store input descriptors with events
    /// @return An int with the result of the poll (see poll.h)
    int pollForInputEvent(const InputEventDescriptors& inputDescriptors, InputEventDescriptors& InputEventDescriptors)
    {
        std::vector<pollfd> pollDescriptors;

        for (auto& inputDescriptor : inputDescriptors) {
            pollfd pollDescriptor { inputDescriptor, POLLIN, 0 };
            pollDescriptors.push_back(pollDescriptor);
        }

        int result = poll(pollDescriptors.data(), pollDescriptors.size(), 1000);
        if (result > 0) {
            for (size_t index = 0; index < pollDescriptors.size(); ++index) {
                if (pollDescriptors.at(index).revents)
                    InputEventDescriptors.push_back(pollDescriptors.at(index).fd);
            }
        }

        return result;
    }

    const std::string m_inputEventPrefix;
    const uint8_t m_maxInputEvents;
    std::thread m_thread;
    std::atomic<bool> m_stopThread;
};

} // namespace Linux::Input
