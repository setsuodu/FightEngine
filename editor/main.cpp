#include <windows.h>
#include <filesystem>
#include <vector>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "core/log.h"
#include "platform/window.h"
#include "platform/input.h"
#include "rhi/dx12.h"
#include "runtime/game_module.h"
#include "runtime/state_arena.h"
#include "imgui_layer.h"
#include "editor_ui.h"

using namespace eng;

static double NowSec() {
    static LARGE_INTEGER freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER c; QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)freq.QuadPart;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ImGui_ImplWin32_EnableDpiAwareness();

    Window win;
    if (!win.Create(L"Fight Engine Editor", 1600, 900)) return 1;
    Dx12 dx;
    if (!dx.Init(win.Hwnd(), win.Width(), win.Height())) { MessageBoxW(nullptr, L"DX12 init failed", L"Error", MB_OK); return 1; }
    win.onResize = [&](u32 w, u32 h) { dx.Resize(w, h); };
    editor::ImGuiInit(win.Hwnd(), dx);

    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    GameModule game;
    game.Init(std::filesystem::path(exe).parent_path() / L"game.dll");

    StateArena arena;
    std::vector<u8> snapshot;
    auto resetGame = [&] {
        const GameAPI* api = game.Api();
        if (!api) return;
        if (arena.Size() != api->stateSize) arena.Create(api->stateSize); else arena.Clear();
        api->Init(arena.Data(), arena.Size());
        LogInfo("game reset");
    };
    resetGame();

    editor::EditorContext ctx; ctx.game = &game; ctx.arena = &arena;
    constexpr double kStep = 1.0 / 60.0;     // 固定逻辑步长 60Hz
    double last = NowSec(), acc = 0.0, fpsTimer = 0.0; int fpsFrames = 0;
    const float clear[4] = {0.08f, 0.08f, 0.10f, 1.f};

    while (win.PumpMessages()) {
        double now = NowSec(); double dt = now - last; last = now;
        if (dt > 0.25) dt = 0.25;

        // ---- 热重载 ----
        bool reloaded = game.Poll((float)dt);
        if (ctx.reloadReq) { reloaded = game.ForceReload(); ctx.reloadReq = false; }
        if (reloaded && game.Api() && game.Api()->stateSize != arena.Size()) { LogWarn("state layout changed -> reset"); resetGame(); snapshot.clear(); ctx.hasSnapshot = false; }

        // ---- UI 请求 ----
        if (ctx.resetReq) { resetGame(); ctx.resetReq = false; }
        if (ctx.saveReq) { arena.Save(snapshot); ctx.hasSnapshot = true; ctx.saveReq = false; LogInfo("snapshot saved (%zu bytes)", snapshot.size()); }
        if (ctx.loadReq) { if (arena.Load(snapshot)) LogInfo("snapshot loaded"); ctx.loadReq = false; }

        if (win.Minimized()) { Sleep(16); continue; }

        // ---- 固定步长逻辑 ----
        acc += dt;
        u16 in[2]; PollInput(win.Hwnd(), in);
        while (acc >= kStep) {
            if ((ctx.running || ctx.stepOnce) && game.Api()) {
                GameInput gi{{in[0], in[1]}};
                game.Api()->Tick(arena.Data(), &gi);
                ++ctx.ticks;
            }
            ctx.stepOnce = false;
            acc -= kStep;
        }
        ctx.alpha = (float)(acc / kStep);

        ++fpsFrames; fpsTimer += dt;
        if (fpsTimer >= 0.5) { ctx.fps = (float)(fpsFrames / fpsTimer); fpsFrames = 0; fpsTimer = 0; }

        // ---- 渲染 ----
        editor::ImGuiNewFrame();
        editor::DrawEditor(ctx);
        auto* list = dx.BeginFrame(clear);
        // TODO: IRenderer 场景渲染在此
        editor::ImGuiRender(list, dx);
        dx.EndFrame(true);
    }

    dx.WaitIdle();
    editor::ImGuiShutdown();
    dx.Shutdown();
    return 0;
}
