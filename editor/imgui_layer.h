#pragma once
#include <windows.h>
#include <d3d12.h>
#include "rhi/dx12.h"
namespace editor {
void ImGuiInit(HWND hwnd, eng::Dx12& dx);
void ImGuiShutdown();
void ImGuiNewFrame();
void ImGuiRender(ID3D12GraphicsCommandList* list, eng::Dx12& dx);   // 内部先 ImGui::Render()
}
