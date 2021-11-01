# input-event

[![build](https://github.com/bang-olufsen/input-event/actions/workflows/build.yml/badge.svg)](https://github.com/bang-olufsen/input-event/actions/workflows/build.yml)
[![Coverage Status](https://coveralls.io/repos/github/bang-olufsen/input-event/badge.svg?branch=main)](https://coveralls.io/github/bang-olufsen/input-event?branch=main)
[![CodeQL](https://github.com/bang-olufsen/input-event/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/bang-olufsen/input-event/actions/workflows/codeql-analysis.yml)
[![license](https://img.shields.io/badge/license-MIT_License-blue.svg?style=flat)](LICENSE)

A small C++11 header-only library for handling Linux input events

This library can be used for subscribing to Linux input events (e.g. key, mouse, switch, sound and LED events). It is also possible to readout the current value of an event (e.g. to know the state of a switch at boot). For more information on the usage please see the working example below and the unit tests.

## Usage

```cpp
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
```

## Limitations

* Does not natively support dynamic input devices (can be done via file watch or udev)
