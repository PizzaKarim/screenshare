#include <pch.h>
#include <capture.h>

#include <dwmapi.h>
#include <psapi.h>

#include <ShlObj.h>
#include <Propkey.h>
#include <atlbase.h>

std::vector<Window> windows;

static bool is_cloaked(HWND hwnd)
{
    int cloaked = 0;
    if (DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)) != S_OK) cloaked = 0;
    return cloaked;
}

static bool is_visible(HWND hwnd)
{
    return !is_cloaked(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd);
}

static std::wstring get_file_path(DWORD pid)
{
    wchar_t szProcessName[MAX_PATH] = L"";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, pid);
    if (hProcess == 0) return L"";

    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
            &cbNeeded))
        {
            GetModuleFileNameEx(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(wchar_t));
        }
    }
    CloseHandle(hProcess);
    return szProcessName;
}

static std::wstring get_file_description(DWORD pid)
{
    wchar_t szProcessName[MAX_PATH] = L"";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, pid);
    if (hProcess == 0) return L"";

    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
            &cbNeeded))
        {
            GetModuleFileNameEx(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(wchar_t));
        }
    }

    std::wstring description;
    CComPtr<IShellItem2> pItem;
    HRESULT hr = SHCreateItemFromParsingName(szProcessName, nullptr, IID_PPV_ARGS(&pItem));
    if (SUCCEEDED(hr))
    {
        LPWSTR var;
        hr = pItem->GetString(PKEY_FileDescription, &var);
        if (SUCCEEDED(hr))
        {
            description = var;
        }
    }

    CloseHandle(hProcess);
    return description;
}

static std::wstring get_window_text(HWND hwnd)
{
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    return title;
}

static std::wstring get_name(HWND hwnd)
{
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    if (wcslen(title) == 0)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        return get_file_description(pid);
    }
    return title;
}

static BOOL windows_proc_callback(HWND hwnd, LPARAM param)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == _getpid()) return TRUE;
    if (!is_visible(hwnd)) return TRUE;
    std::wstring window_name = get_window_text(hwnd);
    if (window_name.empty()) return TRUE;
    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_POPUP)
    {
        if (window_name == L"Program Manager") return TRUE; // Desktop
        if (window_name == L"Task Switching") return TRUE; // ALT+TAB
    }
    windows.push_back({ hwnd, pid, get_name(hwnd) });
    return TRUE;
}

BOOL enum_windows()
{
    windows.clear();
    windows.push_back({ 0, 0, L"Screen" });
    BOOL result = EnumWindows(windows_proc_callback, 0);
    return result;
}

static Frame snapshot_screen(Window* window)
{
    HDC dc = GetDC(0);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HDC capture = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    HGDIOBJ old = SelectObject(capture, bitmap);
    BOOL ok = BitBlt(capture, 0, 0, width, height, dc, 0, 0, SRCCOPY | CAPTUREBLT);
    SelectObject(capture, old);
    DeleteDC(capture);
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    GetDIBits(dc, bitmap, 0, 0, NULL, &bmi, DIB_RGB_COLORS);
    std::vector<unsigned char> pixels(bmi.bmiHeader.biSizeImage);
    bmi.bmiHeader.biCompression = BI_RGB;
    GetDIBits(dc, bitmap, 0, bmi.bmiHeader.biHeight, (LPVOID)&pixels[0], &bmi, DIB_RGB_COLORS);
    DeleteObject(bitmap);
    ReleaseDC(NULL, dc);
    return { window, (unsigned int)width, (unsigned int)height, pixels };
}

static Frame snapshot_window(Window* window)
{
    HWND hwnd = window->hwnd;
    HDC dc = GetWindowDC(hwnd);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Capture window
    HDC capture = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    HGDIOBJ old = SelectObject(capture, bitmap);
    PrintWindow(hwnd, capture, PW_RENDERFULLCONTENT);

    // Crop bitmap
    bool maximized = IsZoomed(hwnd);
    int w = width - 16;
    int h = height - (maximized ? 16 : 8);

    HDC hdc = CreateCompatibleDC(capture);
    HBITMAP bm = CreateCompatibleBitmap(capture, w, h);
    HGDIOBJ ss = SelectObject(hdc, bm);
    BOOL ok = BitBlt(hdc, 0, 0, w, h, capture, 8, maximized ? 8 : 0, SRCCOPY);
    DeleteDC(hdc);
    DeleteObject(bitmap);

    SelectObject(capture, old);
    DeleteDC(capture);
    
    // Write to vector of bytes
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    GetDIBits(dc, bm, 0, 0, NULL, &bmi, DIB_RGB_COLORS);
    std::vector<unsigned char> pixels(bmi.bmiHeader.biSizeImage);
    bmi.bmiHeader.biCompression = BI_RGB;
    GetDIBits(dc, bm, 0, bmi.bmiHeader.biHeight, (LPVOID)&pixels[0], &bmi, DIB_RGB_COLORS);
    DeleteObject(bm);
    ReleaseDC(hwnd, dc);
    return { window, (unsigned int)w, (unsigned int)h, pixels };
}

Frame snapshot(Window* window)
{
    return window->pid != 0 ? snapshot_window(window) : snapshot_screen(window);
}
