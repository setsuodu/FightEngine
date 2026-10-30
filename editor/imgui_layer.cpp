#include "imgui_layer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

static eng::Dx12* g_dx = nullptr;

namespace editor {
void ImGuiInit(HWND hwnd, eng::Dx12& dx) {
    g_dx = &dx;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo ii;
    ii.Device = dx.Device();
    ii.CommandQueue = dx.Queue();
    ii.NumFramesInFlight = (int)eng::kFrames;
    ii.RTVFormat = eng::Dx12::BackbufferFormat();
    ii.DSVFormat = DXGI_FORMAT_UNKNOWN;
    ii.SrvDescriptorHeap = dx.SrvHeap();
    ii.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* c, D3D12_GPU_DESCRIPTOR_HANDLE* g) { g_dx->AllocSrv(c, g); };
    ii.SrvDescriptorFreeFn  = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {};
    ImGui_ImplDX12_Init(&ii);
}
void ImGuiShutdown() {
    ImGui_ImplDX12_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
}
void ImGuiNewFrame() {
    ImGui_ImplDX12_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
}
void ImGuiRender(ID3D12GraphicsCommandList* list, eng::Dx12& dx) {
    ImGui::Render();
    ID3D12DescriptorHeap* heaps[] = {dx.SrvHeap()};
    list->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);
}
}
