#pragma once
#include "core/types.h"
namespace eng { class GameModule; class StateArena; class IRenderer; }
namespace editor {
// UI 与主循环之间的指令/数据交换。UI 只置位请求，由 main 统一执行。
struct EditorContext {
    eng::GameModule* game = nullptr;
    eng::StateArena* arena = nullptr;
    eng::IRenderer*  renderer = nullptr;
    bool running = true, stepOnce = false;
    bool reloadReq = false, resetReq = false, saveReq = false, loadReq = false;
    bool hasSnapshot = false;
    float fps = 0.f, alpha = 0.f;
    eng::u64 ticks = 0;
    // Viewport 内容区尺寸（由 PanelViewport 写入，供 Resize/Draw）
    eng::u32 vpW = 0, vpH = 0;
};
void DrawEditor(EditorContext& ctx);   // 在 NewFrame 与 Render 之间调用
}
