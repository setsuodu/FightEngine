#pragma once
// 最小键盘轮询。后续替换为 Raw Input / XInput / GameInput，并做输入缓冲。
#include <windows.h>
#include "runtime/game_api.h"
namespace eng {
inline void PollInput(HWND focus, u16 out[2]) {
    out[0] = out[1] = 0;
    if (GetForegroundWindow() != focus) return;
    auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    if (down('A')) out[0] |= BTN_LEFT;  if (down('D')) out[0] |= BTN_RIGHT;
    if (down('W')) out[0] |= BTN_UP;    if (down('S')) out[0] |= BTN_DOWN;
    if (down('J')) out[0] |= BTN_A;     if (down('K')) out[0] |= BTN_B;
    if (down(VK_LEFT))  out[1] |= BTN_LEFT;  if (down(VK_RIGHT)) out[1] |= BTN_RIGHT;
    if (down(VK_UP))    out[1] |= BTN_UP;    if (down(VK_DOWN))  out[1] |= BTN_DOWN;
    if (down(VK_NUMPAD1)) out[1] |= BTN_A;   if (down(VK_NUMPAD2)) out[1] |= BTN_B;
}
}
