#pragma once
#include <windows.h>
#include <functional>
#include "core/types.h"
namespace eng {
class Window {
public:
    bool Create(const wchar_t* title, u32 w, u32 h);
    void Destroy();
    bool PumpMessages();                 // false = 请求退出
    HWND Hwnd() const { return hwnd_; }
    u32  Width() const { return w_; }
    u32  Height() const { return h_; }
    bool Minimized() const { return minimized_; }
    std::function<void(u32, u32)> onResize;
private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    HWND hwnd_ = nullptr; u32 w_ = 0, h_ = 0; bool minimized_ = false, quit_ = false;
};
}
