# input-event

[![build](https://github.com/bang-olufsen/input-event/actions/workflows/build.yml/badge.svg)](https://github.com/bang-olufsen/input-event/actions/workflows/build.yml)

A small C++11 header-only library for handling Linux input events

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
    if (errorCode)
        std::cerr << "Failed to subscribe for input events with error " << errorCode << std::endl;

    while (true);
}
```

## Limitations

* Does not natively support dynamic input devices (could be done via filewatch or udev)
