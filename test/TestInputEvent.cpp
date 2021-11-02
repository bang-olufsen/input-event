// Copyright (c) 2021 Bang & Olufsen a/s
//
// SPDX-License-Identifier: MIT

#include "InputEvent.h"

#include <cstring>
#include <fstream>
#include <thread>

#include "turtle/catch.hpp"
#include <catch.hpp>

using namespace std::chrono_literals;

TEST_CASE("InputEvent")
{
    std::string inputEventPrefix = "/tmp/test-input-event";
    std::string inputEventFile = inputEventPrefix + "0";
    std::ofstream inputEventStream(inputEventFile.c_str(), std::ofstream::binary);

    Linux::Input::InputEvent inputEvent(inputEventPrefix);

    SECTION("Test subscribe")
    {
        SECTION("Invalid event types")
        {
            int errorCode = inputEvent.subscribe({}, { KEY_COFFEE }, [](input_event&) {});
            CHECK(errorCode == -EINVAL);
        }

        SECTION("Invalid event codes")
        {
            int errorCode = inputEvent.subscribe({ EV_KEY }, {}, [](input_event&) {});
            CHECK(errorCode == -EINVAL);
        }

        SECTION("Invalid event callback")
        {
            int errorCode = inputEvent.subscribe({ EV_KEY }, { KEY_COFFEE }, nullptr);
            CHECK(errorCode == -EINVAL);
        }

        SECTION("EV_KEY input")
        {
            input_event event { 0, 0, EV_KEY, KEY_COFFEE, 0 };
            std::array<char, sizeof(input_event)> eventBuffer;
            std::memcpy(&eventBuffer, &event, sizeof(eventBuffer));

            int errorCode = inputEvent.subscribe({ EV_KEY }, { KEY_COFFEE }, [](input_event& event) {
                CHECK(event.type == EV_KEY);
                CHECK(event.code == KEY_COFFEE);
            });
            CHECK_FALSE(errorCode);

            inputEventStream.write(eventBuffer.data(), eventBuffer.size());
            inputEventStream.flush();

            // Wait for the poll to succeed and the callback to be invoked
            std::this_thread::sleep_for(100ms);
        }
    }

    SECTION("Test value")
    {
        SECTION("Invalid input")
        {
            int value = inputEvent.value(EV_ABS, ABS_RUDDER);
            CHECK(value == -ENOTSUP);
        }

        SECTION("EV_KEY input")
        {
            int value = inputEvent.value(EV_KEY, KEY_COFFEE);
            // Will fail as we are using a regular file for testing
            CHECK(value == -ENOTTY);
        }

        SECTION("EV_SW input")
        {
            int value = inputEvent.value(EV_SW, SW_MICROPHONE_INSERT);
            // Will fail as we are using a regular file for testing
            CHECK(value == -ENOTTY);
        }
    }

    inputEventStream.close();
    remove(inputEventFile.c_str());
}
