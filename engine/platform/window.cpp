#include "platform/window.h"
#include "imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace eng {
bool Window::Create(const wchar_t* title, u32 w, u32 h) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc); wc.style = CS_CLASSDC; wc.lpfnWndProc = &Window::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr); wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FightEngineWnd";
    RegisterClassExW(&wc);
    RECT r = {0, 0, (LONG)w, (LONG)h};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd_ = CreateWindowExW(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, this);
    if (!hwnd_) return false;
    ShowWindow(hwnd_, SW_SHOW); UpdateWindow(hwnd_);
    RECT cr; GetClientRect(hwnd_, &cr); w_ = cr.right; h_ = cr.bottom;
    return true;
}
void Window::Destroy() { if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; } }
bool Window::PumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) quit_ = true;
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return !quit_;
}
LRESULT CALLBACK Window::WndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    Window* self = nullptr;
    if (m == WM_NCCREATE) {
        self = (Window*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        self->hwnd_ = h;
    } else self = (Window*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (ImGui_ImplWin32_WndProcHandler(h, m, wp, lp)) return true;
    if (self) switch (m) {
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) { self->minimized_ = true; }
        else {
            self->minimized_ = false;
            self->w_ = LOWORD(lp); self->h_ = HIWORD(lp);
            if (self->onResize && self->w_ && self->h_) self->onResize(self->w_, self->h_);
        }
        return 0;
    case WM_CLOSE:   self->quit_ = true; DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}
}
