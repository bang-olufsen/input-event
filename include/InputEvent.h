// Copyright (c) 2021 Bang & Olufsen a/s
//
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
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
    /// @param inputEventPrefix The input event prefix to use (default /dev/input/event)
    /// @param maxInputEvents The number of /dev/input/eventX files to monitor (default 10)
    InputEvent(const std::string& inputEventPrefix = "/dev/input/event", const uint8_t maxInputEvents = 10)
    {
        for (uint8_t inputEvent = 0; inputEvent < maxInputEvents; ++inputEvent) {
            auto device = inputEventPrefix + std::to_string(inputEvent);
            int result = open(device.c_str(), O_RDONLY);
            if (result > 0)
                m_inputDescriptors.push_back(result);
        }
    }

    /// @brief InputEvent destructor
    ~InputEvent()
    {
        m_stopThread.store(true);

        if (m_thread.joinable())
            m_thread.join();
    }

    /// @brief Subscribe for input events matching the specified types and codes
    /// In case of errors an input_event with type UINT16_MAX and code UINT16_MAX will be injected with value set to
    /// the errno value. At the same time the thread is stopped and a new subscribe function call is required.
    /// @note It is only possible to call subscribe once per instance
    /// @param eventTypes A std::vector with event types from <linux/input-event-codes.h>. Use UINT16_MAX for all types
    /// @param eventCodes A std::vector with event codes from <linux/input-event-codes.h>. Use UINT16_MAX for all codes
    /// @param eventCallback A callback to be invoked when an event matching the specified event types and codes is received
    /// @return An int with the result (0 on success or or a negative value from errno.h)
    int subscribe(const InputEventList eventTypes, const InputEventList eventCodes, const InputEventCallback eventCallback)
    {
        if (!eventTypes.size() || !eventCodes.size() || !eventCallback)
            return -EINVAL;

        if (m_inputDescriptors.empty())
            return -EBADF;

        m_stopThread.store(false);
        auto& inputDescriptors = m_inputDescriptors;
        m_thread = std::thread([this, eventTypes = std::move(eventTypes), eventCodes = std::move(eventCodes), eventCallback = std::move(eventCallback), inputDescriptors = std::move(inputDescriptors)]() {
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
                    event = { 0, 0, UINT16_MAX, UINT16_MAX, -errno };
                    eventCallback(event);
                    m_stopThread.store(true);
                }
            }

            for (const auto& inputDescriptor : inputDescriptors)
                close(inputDescriptor);
        });

        return 0;
    }

    /// @brief Get current input event value for the specified event type and code
    /// @param eventType An event type from <linux/input-event-codes.h>
    /// @param eventCode An event code from <linux/input-event-codes.h>
    /// @return An int with the result (event value (0 or 1) on success or or a negative value from errno.h)
    int value(uint16_t eventType, uint16_t eventCode)
    {
        if (m_inputDescriptors.empty())
            return -EBADF;

        int eventValue = 0;
        std::vector<uint8_t> eventCodeBits((eventCode / 8) + 1);

        for (auto inputDescriptor : m_inputDescriptors) {
            int result = -ENOTSUP;

            switch (eventType) {
            case EV_KEY:
                result = ioctl(inputDescriptor, EVIOCGKEY(eventCodeBits.size()), eventCodeBits.data());
                break;
            case EV_SW:
                result = ioctl(inputDescriptor, EVIOCGSW(eventCodeBits.size()), eventCodeBits.data());
                break;
            default:
                return result;
            }

            if (result < 0)
                return -errno;

            eventValue |= (eventCodeBits[eventCode / 8] >> (eventCode % 8)) & 1;
        }

        return eventValue;
    }

private:
    /// @brief Polls for input events on the specified input descriptors
    /// @param[in] inputDescriptors A reference to an InputEventDescriptors object with input descriptors
    /// @param[out] inputEventDescriptors A reference to an InputEventDescriptors object to store input descriptors with events
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

    std::thread m_thread;
    std::atomic<bool> m_stopThread;
    InputEventDescriptors m_inputDescriptors;
};

} // namespace Linux::Input
