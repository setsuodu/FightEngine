# FightEngine 底座 (空壳版)

Windows + DX12 + ImGui(docking) + C++20。无需 vcpkg, CMake 会自动拉取 ImGui。

## 构建
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build\bin\Debug\editor.exe
```
(或直接用 VS2022 "打开文件夹" 方式打开本目录)

## CI / 发布

- **PR / push main**：`.github/workflows/build.yml` → Windows VS2022 Debug+Release，上传 artifact
- **Tag 发布**：`.github/workflows/release.yml`

```bash
git tag v0.1.0
git push origin v0.1.0
# 或：git tag editor/v0.1.0 && git push origin editor/v0.1.0
```

会自动编译 Release、打 zip，并在 GitHub Releases 挂上：
- `FightEngine-0.1.0-win-x64.zip`
- `editor.exe` + `game.dll`

## 已有
- Win32 窗口 + DX12 (设备/队列/3 帧 in-flight/围栏/清屏/Resize) + ImGui Docking
- 固定 60Hz 逻辑循环 + 变帧率渲染 + 插值系数
- 游戏 DLL 热重载 (状态由引擎持有, 重载不丢; 状态大小变化自动重置)
- StateArena 快照 (Save/Load = memcpy) 与 编辑器面板按钮
- 面板: Viewport(占位) / Game / Log / Assets(占位)

## 试一下热重载
编辑器运行中, 改 `game/game_main.cpp` (如 spd 改成 6), 然后
`cmake --build build --config Debug --target game`, 日志面板会出现 "game module loaded (#2)", 移动速度立即变化。

## 待补 (见 engine/interfaces 与 docs/CONVENTIONS.md)
渲染器 / glTF+cooker / 动画 / 音频 / 反射 / GGPO / 定点数库补全 / 输入缓冲

## 已知限制
- 热重载用的是复制出的 game_live_N.dll, 调试器按原名找 PDB 可能对不上; 需要断点调试时先 Force reload 后再附加, 或之后改用 PDB 重命名方案。
