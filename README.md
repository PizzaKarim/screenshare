# screenshare

A quick and dirty program for capturing window and screen content.

This program is only compatible with the Windows operating system. The intention was to practice using the Win32 API in conjunction with OpenGL.

The libraries used are not included in this repository, as the intention is simply showcasing the project, not providing a means of compilation.
You can, however, download the latest release from [Releases](https://github.com/PizzaKarim/screenshare/releases) and try the program.

## Images

![new_instance](https://imgur.com/Boz9hUp)
![hovering](https://imgur.com/u9vRZaA)
![clicked](https://imgur.com/LAvKxKm)

## Features

Currently, the program has the following features:
* Detects all visible windows running on the computer and displays previews of them.
* Previews refresh every second.
* Left click on a preview will expand it and refresh it every frame.
* Pressing ESCAPE or mouse button four (if you have extra buttons) will go back to the previews.

## Issues

Some windows may not be rendered. This is usually because they're hardware accelerated.

## Potential features

In the future, I might work on these features:
* Compress and send the data through a UDP socket for live streaming in another application instance or web server.
* Copy current frame to clipboard using CTRL+C.
