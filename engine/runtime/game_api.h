#pragma once
// 引擎 <-> 游戏 DLL 的唯一契约。游戏状态内存由引擎持有 (StateArena)，DLL 热重载不丢状态。
#include "core/types.h"
namespace eng {
constexpr u32 kGameApiVersion = 1;

enum Button : u16 { BTN_LEFT = 1, BTN_RIGHT = 2, BTN_UP = 4, BTN_DOWN = 8, BTN_A = 16, BTN_B = 32 };
struct GameInput { u16 buttons[2]; };

// 约定: 游戏状态结构体的第一个成员必须是它 (编辑器面板读取)。
struct GameStateHeader { u32 frame; u32 rng; };

struct GameAPI {
    u32 version;
    u64 stateSize;                                   // 状态大小变化 => 引擎重建并重新 Init
    void (*Init)(void* state, u64 size);
    void (*Tick)(void* state, const GameInput* in);  // 固定 60Hz 逻辑帧
};
using GetGameAPIFn = bool (*)(GameAPI*);
}
