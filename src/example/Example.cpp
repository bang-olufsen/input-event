// Copyright (c) 2021 Bang & Olufsen a/s
//
// SPDX-License-Identifier: MIT

#include <InputEvent.h>
#include <iostream>

int main()
{
    Linux::Input::InputEvent inputEvent;
    int errorCode = inputEvent.subscribe({ EV_KEY }, { KEY_SPACE }, [](input_event& event) {
        if (event.code == KEY_SPACE)
            std::cout << "Space button " << (event.value ? "pressed" : "released") << std::endl;
        else if (event.code == UINT16_MAX)
            std::cerr << "Failed to read input events with error " << event.value << std::endl;
    });
    if (errorCode < 0) {
        std::cerr << "Failed to subscribe for input events with error " << errorCode << std::endl;
        return errorCode;
    }

    int value = inputEvent.value(EV_KEY, KEY_SPACE);
    if (value >= 0)
        std::cout << "Space button currently " << (value ? "pressed" : "released") << std::endl;

    while (true);
}
