# FightEngine — Agent Notes

Windows-only (Win32 + DX12) C++20 fighting-game engine base with an ImGui docking editor and a hot-reloadable game DLL. Read `docs/CONVENTIONS.md` before changing sim/game state and `docs/ROADMAP.md` before starting larger work.

## Build & Run

- Requires Visual Studio 2022/2026 C++ tools. No vcpkg; CMake `FetchContent` pulls ImGui `v1.91.9-docking`.
- CMake minimum is 3.24. The VS 2026 generator (`Visual Studio 18 2026`) needs CMake >= 4.2; CI uses VS 2022 (`Visual Studio 17 2022`).
- `build/` is git-ignored and currently configured for VS 2026. Do not reconfigure with a different generator without deleting it first.

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
build\bin\Debug\editor.exe
```

- All outputs go to `build/bin/<Config>/` (`editor.exe` + `game.dll`); `editor` depends on `game`, so building `editor` also builds the DLL.
- There is no test, lint, formatter, or typecheck setup. Verify by building Debug and Release (what CI does) and running the editor for behavior.

## Hot Reload

- Edit `game/game_main.cpp`, then build only the DLL while the editor runs:
  `cmake --build build --config Debug --target game`
- The editor polls `game.dll` every 0.25 s, copies it to `game_live_N.dll`, and loads the copy so the original stays writable. A debugger may not bind PDBs to the live copy; use **Force reload** or attach after reload.
- Game state lives in the engine-owned `StateArena` and survives reload only while `GameAPI::stateSize` is unchanged. A size change resets the game and clears the snapshot.

## Architecture

- Layers: `engine/core` (types/log/assert/fixed) → `engine/platform` (Win32) → `engine/rhi` (DX12, not exposed to game) → `engine/runtime` (GameAPI, StateArena, GameModule) → `engine/render` / `editor`; `game/` is the script DLL.
- The game DLL may include only `runtime/game_api.h` and `core/*`. Keep `engine/interfaces/` signatures stable; add the interface before implementing a new module and ask before changing an existing one.
- `CMakeLists.txt` lists engine sources explicitly (`ENGINE_SRC`). Add new `.cpp` files there.
- DLL contract: export `GetGameAPI` as `extern "C" __declspec(dllexport)`, return `kGameApiVersion`, and bump that version when changing the ABI. `GameState` must start with `GameStateHeader`; `Init` receives the whole arena and must zero-initialize it.
- `engine/render/renderer.cpp` currently hardcodes the demo `GameState` layout to match `game/game_main.cpp`. Update both together when changing state layout; the static asserts there enforce part of it. P1 replaces this with a DebugDraw contract.
- Frame order in `editor/main.cpp` is deliberate: `ImGuiNewFrame` → `DrawEditor` (writes Viewport size) → `Renderer.Resize` → `Dx12::BeginFrame` → `Renderer.Draw` into the offscreen RT → `BindBackbuffer` → `ImGuiRender` → `EndFrame`.
- Editor panels only set request flags on `EditorContext`; `main.cpp` executes reset/save/load/reload. Do not mutate the arena directly from UI code.
- The default dock layout is built only when `imgui.ini` is absent from the working directory. Delete `build/bin/<Config>/imgui.ini` to see layout changes.

## Sim Rules (rollback prerequisites)

- State structs must be trivially copyable (add `static_assert(std::is_trivially_copyable_v<T>)`) and zero-initialized. No pointers, virtual functions, `std::string`/containers, or `float`/`double` in state. Use `Fixed`, handles (index + generation), and fixed-size arrays.
- Sim code must not use `new`/`delete`, exceptions, RTTI, system time, or unseeded RNG. RNG state belongs in the state.
- Presentation reads sim state only; trigger audio/effects through events and never write back. Animation poses are `f(clipId, frame)`, not accumulated `dt`.
- `Fixed` is Q16.16. `FromFloat`/`ToFloat` are for initialization, editor, and rendering only — never sim logic.

## Style

- Types and functions `PascalCase`, members `name_`, constants `kName`. Prefer `const` and use `[[nodiscard]]` on functions returning errors/optionals.
- Use `LogInfo`/`LogWarn`/`LogError` and `ENG_ASSERT`/`ENG_HR` instead of ad-hoc logging or asserts.
- `docs/ROADMAP.md` is the plan: implement the current milestone (P0) and do not jump ahead to GGPO/networking.

## CI / Release

- `.github/workflows/build.yml` builds Debug + Release on `windows-2022` for PRs and pushes to `main`.
- `.github/workflows/release.yml` triggers on `v*` or `editor/v*` tags and publishes `editor.exe` + `game.dll`.
