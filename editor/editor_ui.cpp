#include "editor_ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <filesystem>
#include "core/log.h"
#include "runtime/game_module.h"
#include "runtime/state_arena.h"
#include "runtime/game_api.h"

namespace editor {
using namespace eng;

static void BuildDefaultLayout(ImGuiID root) {
    ImGui::DockBuilderRemoveNode(root);
    ImGui::DockBuilderAddNode(root, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(root, ImGui::GetMainViewport()->Size);
    ImGuiID right, rest, bottom, center;
    ImGui::DockBuilderSplitNode(root, ImGuiDir_Right, 0.25f, &right, &rest);
    ImGui::DockBuilderSplitNode(rest, ImGuiDir_Down, 0.30f, &bottom, &center);
    ImGui::DockBuilderDockWindow("Game", right);
    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Log", bottom);
    ImGui::DockBuilderDockWindow("Assets", bottom);
    ImGui::DockBuilderFinish(root);
}

static void PanelViewport(EditorContext& c) {
    ImGui::Begin("Viewport");
    ImVec2 sz = ImGui::GetContentRegionAvail();
    ImGui::Text("Renderer: not implemented (IRenderer stub)");
    ImGui::Text("Region: %.0f x %.0f   interp alpha: %.2f", sz.x, sz.y, c.alpha);
    ImGui::TextDisabled("TODO: 渲染目标纹理 -> ImGui::Image 显示场景");
    ImGui::End();
}

static void PanelGame(EditorContext& c) {
    ImGui::Begin("Game");
    ImGui::Text("FPS: %.1f   logic ticks: %llu", c.fps, (unsigned long long)c.ticks);
    ImGui::Separator();
    if (ImGui::Button(c.running ? "Pause" : "Run")) c.running = !c.running;
    ImGui::SameLine(); if (ImGui::Button("Step")) c.stepOnce = true;
    ImGui::SameLine(); if (ImGui::Button("Reset")) c.resetReq = true;
    if (ImGui::Button("Save snapshot")) c.saveReq = true;
    ImGui::SameLine();
    ImGui::BeginDisabled(!c.hasSnapshot);
    if (ImGui::Button("Load snapshot")) c.loadReq = true;
    ImGui::EndDisabled();
    ImGui::Separator();

    const GameAPI* api = c.game ? c.game->Api() : nullptr;
    ImGui::Text("Game module: %s", api ? "loaded" : "NOT LOADED");
    if (c.game) {
        ImGui::Text("Reloads: %u", c.game->ReloadCount());
        ImGui::TextWrapped("%s", c.game->SourcePath().string().c_str());
    }
    if (ImGui::Button("Force reload")) c.reloadReq = true;
    ImGui::Separator();
    if (c.arena && c.arena->Data() && c.arena->Size() >= sizeof(GameStateHeader)) {
        auto* h = (const GameStateHeader*)c.arena->Data();
        ImGui::Text("State size: %llu bytes", (unsigned long long)c.arena->Size());
        ImGui::Text("frame: %u", h->frame);
        ImGui::Text("rng:   %u", h->rng);
    }
    ImGui::TextDisabled("P1: WASD + J/K    P2: arrows + Num1/2");
    ImGui::End();
}

static void PanelLog() {
    ImGui::Begin("Log");
    for (const auto& e : LogEntries()) {
        ImVec4 col = e.level == LogLevel::Error ? ImVec4(1, .4f, .4f, 1) : e.level == LogLevel::Warn ? ImVec4(1, .85f, .3f, 1) : ImVec4(.85f, .85f, .85f, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

static void PanelAssets() {
    ImGui::Begin("Assets");
    ImGui::TextDisabled("TODO: IAssetManager / cooker / 资源浏览器");
    ImGui::End();
}

void DrawEditor(EditorContext& ctx) {
    static bool first = true;
    bool firstRun = first && !std::filesystem::exists("imgui.ini");
    ImGuiID root = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
    if (firstRun) BuildDefaultLayout(root);
    first = false;
    PanelViewport(ctx); PanelGame(ctx); PanelLog(); PanelAssets();
}
}
