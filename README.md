# project

> **Blueprint VSE 的"早期实验分支"——同一套可视化脚本核心（C++17 + VSE 命名空间）的早期原型，工程结构是 7 个头文件 + 1 个 CMakeLists，无 main，无 .cpp 实现。**

## 这是什么

`project` 仓库是 **Blueprint**（同目录另一仓库，可视化脚本引擎核心）的**早期 / 平行分支**——两者使用完全相同的 `VSE` 命名空间和几乎相同的数据建模（`Pin / Node / Link / Graph / Variant / TypeRegistry / ExecutionContext / GraphExecutor`），但这里的代码还停留在"只有头文件、无 .cpp 实现"的更早阶段。

它存在的意义：

1. **作为"老版本快照"保留**——与 Blueprint 互为对照，能看出 VSE 数据模型是如何从 `Pin::ConnectedTo`（单连接）演化成 `Pin::ConnectedLinkIDs`（多连接 + ID 索引）的；
2. **作为头文件 include-only 的轻量版**——如果只想引用 VSE 的数据结构做 PoC，可以直接 include 这里的 `VSE_*.h` 而不必拉 Blueprint 的 imgui / glad / spdlog 依赖。

## 仓库内容

```
project/
├── CMakeLists.txt                # 与 Blueprint 几乎一致：glad + imgui + glfw 静态库 + 可执行
└── src/
    ├── VSE_BuiltinNodes.h        # 声明 RegisterBuiltinNodes()
    ├── VSE_Executor.h            # GraphExecutor（pull-based data + push-based exec）
    ├── VSE_FunctionTraits.h      # function_traits 模板元编程：把任意函数签名拆成 arg<0>, arg<1>...
    ├── VSE_Graph.h               # Pin / Node / Link / Graph / GraphVariable 实例 + VSE::Variant
    ├── VSE_NodeFactory.h         # NodeFactory::Register<Func>(typeName, category, argNames, func)——根据 C++ 函数自动生成 NodeDefinition
    ├── VSE_Registry.h            # 注册表（TypeName → NodeDefinition）
    └── VSE_Types.h               # Variant / TypeInfo / PinType / PinDirection
```

> 与 Blueprint 相比的差异：
> - 这里的 `Pin` 用 `int ID` + `Pin* ConnectedTo`（**单连接**模型）；
> - Blueprint 已经演化成 `VSE_ID`（UUID）+ `ConnectedLinkIDs`（**多连接**模型）；
> - 这里的 `ExecutionContext` 用引用持有 executor 的状态（`ValueCache & / NodeStates & / ExecutionStack &`），Blueprint 改成 `Executor*` 指针；
> - `NodeFactory` 是本仓库独有——Blueprint 里没有。
> - CMakeLists 写法几乎一样，但 Blueprint 还多 `add_subdirectory(libs/stduuid)` / `add_subdirectory(libs/spdlog)` / 链接 `spdlog::spdlog` / 链接 `stduuid`。
> - `target_link_libraries(${PROJECT_NAME} PRIVATE imgui)` 的 `PROJECT_NAME` 在 Blueprint 是 `Blueprint` 在这里是 `project`（即本目录名），但 `project()` 行写的是 `Blueprint`——所以默认 `add_executable` 出来的二进制会叫 `Blueprint.exe`，是个小 bug。

## 状态

- **v0.x 早期**：仅头文件 + 1 个 CMakeLists，无 main，无任何 .cpp 实现
- **学习 / 备份性质**：作为 Blueprint 的对照原型保留
- **可运行性**：可编译（前提：补全 `libs/imgui` / `libs/glfw` 等 vendored 第三方），但 `main.cpp` 缺失，**产物不可运行**
- **最后提交**：2025-06-13 by wudixzy（"test2"）

## License

仓库内未附 LICENSE 文件，源码默认遵循 "All rights reserved"。
