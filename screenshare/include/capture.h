#pragma once

#include <Windows.h>

#include <vector>
#include <string>

struct Window
{
    HWND hwnd;
    DWORD pid;
    std::wstring caption;
};

struct Frame
{
    Window* window;
    unsigned int width;
    unsigned int height;
    std::vector<unsigned char> pixels;
};

BOOL enum_windows();
Frame snapshot(Window* window);
