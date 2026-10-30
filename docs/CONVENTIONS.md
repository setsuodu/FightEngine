# 约束文档 (每次给其他 AI 的提示词都附上)

## 分层
- `engine/core`      基础类型/日志/断言/定点数。无外部依赖。
- `engine/platform`  Win32 窗口/输入/计时。
- `engine/rhi`       DX12 薄封装。不暴露给 game。
- `engine/runtime`   GameAPI 契约、StateArena、GameModule 热重载。
- `engine/interfaces` 待实现模块的接口 (按头文件补实现, 不改接口签名; 需改时先提出)。
- `editor/`          ImGui 编辑器。只通过 EditorContext 与主循环交互。
- `game/`            游戏 DLL (脚本层, C++)。只依赖 `runtime/game_api.h` 与 `core/`。

## Sim / 游戏状态规则 (回滚前提)
1. 状态结构体必须 `static_assert(std::is_trivially_copyable_v<T>)`。
2. 状态内禁止: 指针、虚函数、std::string/vector/map、float/double。用 Fixed、句柄(索引+generation)、定长数组。
3. Sim 代码禁止: new/delete、异常、RTTI、系统时间、未种子的随机数 (RNG 状态必须在状态里)。
4. 所有状态初始化为零值。
5. 表现 (音效/特效/震屏) 只读 Sim 状态, 经事件队列触发, 不得回写 Sim。
6. 动画姿势 = f(clipId, frame), 不累加 dt。

## 引擎侧
- 默认 const, `[[nodiscard]]` 用于返回错误/Optional 的函数。
- 命名: 类型 PascalCase, 函数 PascalCase, 成员 `name_`, 常量 `kName`。
- 新增模块先在 `engine/interfaces` 写接口, 再实现。
- 日志用 `LogInfo/Warn/Error`, 断言用 `ENG_ASSERT/ENG_HR`。
