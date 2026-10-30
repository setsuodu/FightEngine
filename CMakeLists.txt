cmake_minimum_required(VERSION 3.24)
project(FightEngine LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "Windows only (DX12).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)   # exe 与 game.dll 同目录
add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
if(MSVC)
  add_compile_options(/W4 /utf-8 /permissive-)
endif()

# ---------- Dear ImGui (docking 分支, 由 FetchContent 拉取) ----------
include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.9-docking
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d12 dxgi d3dcompiler dwmapi)

# ---------- engine 静态库 ----------
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS engine/*.cpp)
add_library(engine STATIC ${ENGINE_SRC})
target_include_directories(engine PUBLIC engine)
target_link_libraries(engine PUBLIC imgui d3d12 dxgi)

# ---------- 游戏模块 (热重载 DLL) ----------
add_library(game SHARED game/game_main.cpp)
target_include_directories(game PRIVATE engine)
set_target_properties(game PROPERTIES PREFIX "" OUTPUT_NAME game)

# ---------- 编辑器 exe ----------
add_executable(editor WIN32 editor/main.cpp editor/imgui_layer.cpp editor/editor_ui.cpp)
target_link_libraries(editor PRIVATE engine)
add_dependencies(editor game)
set_target_properties(editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT editor)
