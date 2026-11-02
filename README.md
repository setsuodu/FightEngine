# FightEngine

Windows + DX12 + ImGui(docking) + C++20 格斗游戏引擎底座。无需 vcpkg，CMake 自动 FetchContent 拉取 ImGui。

开发路线（MVP / P0–P4）见 **[docs/ROADMAP.md](docs/ROADMAP.md)**。  
编码与 Sim 约束见 **[docs/CONVENTIONS.md](docs/CONVENTIONS.md)**。

---

## 构建

需要 **Visual Studio 2022 或 2026**（带 C++ 工具集）、CMake ≥ 3.24（VS 2026 生成器需 CMake ≥ 4.2）。

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
build\bin\Debug\editor.exe
```

若更换生成器，先删掉旧 `build` 再配置：

```bat
rmdir /s /q build
```

---

## 已有

- Win32 窗口 + DX12（设备 / 队列 / 3 帧 in-flight / 围栏 / 清屏 / Resize）+ ImGui Docking
- 固定 60Hz 逻辑循环 + 变帧率渲染 + 插值系数
- 游戏 DLL 热重载（状态由引擎持有，重载不丢；状态尺寸变化自动重置）
- StateArena 快照（Save/Load = memcpy）与编辑器面板按钮
- **IRenderer 最小实现**：离屏 color RT → Viewport `ImGui::Image`；Demo 两角色色块 + 逻辑帧插值
- 面板：Viewport / Game / Log / Assets（占位）

---

## 试一下

**操控（Demo）**

- P1：WASD 移动（青块）
- P2：方向键移动（橙块）

**热重载**

编辑器运行中，改 `game/game_main.cpp`（如 `spd`），然后：

```bat
cmake --build build --config Debug --target game
```

日志面板出现 `game module loaded (#N)`，移动速度立即变化，帧号不清零。

---

## CI / 发布

- **PR / push `main`**：`.github/workflows/build.yml` → Windows 编译 Debug+Release，上传 artifact
- **Tag 发布**：`.github/workflows/release.yml`

```bash
git tag v0.1.0
git push origin v0.1.0
# 或：git tag editor/v0.1.0 && git push origin editor/v0.1.0
```

Release 会挂：`FightEngine-<ver>-win-x64.zip`、`editor.exe`、`game.dll`。

---

## 下一步（P0）

见 [docs/ROADMAP.md](docs/ROADMAP.md)：定点数补全、输入缓冲、hit/hurtbox、状态机 v0、碰撞与命中事件。

更长期：帧数据编辑器、确定性/回放、GGPO、音频、资源 cooker 等（接口占位在 `engine/interfaces/`）。

---

## 已知限制

- 热重载加载的是复制出的 `game_live_N.dll`，调试器按原名找 PDB 可能对不上；需要断点时先 Force reload 再附加，或后续改 PDB 重命名方案。
- Demo 角色布局仍写在 `engine/render/renderer.cpp`（P1 应改为 GameAPI DebugDraw 契约）。
- 每帧临时 upload 顶点缓冲（三槽续命），后续换环形上传堆。
