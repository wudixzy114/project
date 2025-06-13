#include "src/VSE_BuiltinNodes.h"
#include "src/VSE_Executor.h"
#include <iostream>
#include <string>

using namespace VSE;

void VSE::RegisterBuiltinNodes()
{
    auto &registry = NodeRegistry::Instance();
    std::cout << "Registering built-in VSE nodes...\n";

    registry.Registry({"Event.on_start", " Events", {{"Evec Out", PinType::Execution, nullptr}}, [](ExecutionContext &ctx)
                       { return 0; }});

    registry.Registry({"Io.print_int", "IO", {{"Exec In", PinType::Execution, nullptr}, {"In", PinType::Data, &TypeRegistry::Get<int>()}, {"Exec Out", PinType::Execution, nullptr}}, [](ExecutionContext &ctx) -> int
                       {
                           int val = ctx.GetInputValue("In").GetValue<int>();
                           std::cout << "VSE_LOG: " << val << std::endl;
                           return 0;
                       }});

    registry.Registry({"io.print_string", "IO", {{"Exec In", PinType::Execution, nullptr}, {"Exec Out", PinType::Execution, nullptr}, {"In", PinType::Data, &TypeRegistry::Get<std::string>()}}, [](ExecutionContext &ctx) -> int
                       {
                           std::string val = ctx.GetInputValue("In").GetValue<std::string>();
                           std::cout << "VSE_LOG: " << val << std::endl;
                           return 0;
                       }});

    registry.Registry({"literal.int", "Literals", {{"Value", PinType::Data, &TypeRegistry::Get<int>()}}, [](ExecutionContext &ctx)
                       {
                           ctx.SetOutputValue("Value", ctx.CurrentNode->OutputPins[0].DefaultValue);
                           return -1;
                       }});

    registry.Registry({"literal.bool", "Literals", {{"Value", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx)
                       {
                           ctx.SetOutputValue("Value", ctx.CurrentNode->OutputPins[0].DefaultValue);
                           return -1;
                       }});

    registry.Registry({"literal.string", "Literals", {{"Value", PinType::Data, &TypeRegistry::Get<std::string>()}}, [](ExecutionContext &ctx)
                       {
                           ctx.SetOutputValue("Value", ctx.CurrentNode->OutputPins[0].DefaultValue);
                           return -1;
                       }});

    registry.Registry({"flow.branch", "Flow Control", {{"Exec In", PinType::Execution, nullptr}, {"Condition", PinType::Data, &TypeRegistry::Get<bool>()}, {"True", PinType::Execution, nullptr}, {"False", PinType::Execution, nullptr}}, [](ExecutionContext &ctx) -> int
                       {
                           bool condition = ctx.GetInputValue("Condition").GetValue<bool>();
                           return condition ? 0 : 1; // 0 for True, 1 for False
                       }});

    registry.Registry({"flow.sequence", "Flow Control", {
                                                            {"Exec In", PinType::Execution, nullptr}, {"Then 0", PinType::Execution, nullptr}, {"Then 1", PinType::Execution, nullptr}
                                                            // 可以添加更多 "Then X"
                                                        },
                       [](ExecutionContext &ctx) -> int
                       {
                           // 依次将所有连接的输出节点推入执行栈
                           // 注意：由于栈是后进先出，我们需要反向推入
                           Pin *then1 = ctx.CurrentNode->FindPinByName("Then 1", PinDirection::Output);
                           if (then1 && then1->ConnectedTo)
                           {
                               ctx.ExecutionStack.push(then1->ConnectedTo->ParentNode);
                           }
                           Pin *then0 = ctx.CurrentNode->FindPinByName("Then 0", PinDirection::Output);
                           if (then0 && then0->ConnectedTo)
                           {
                               ctx.ExecutionStack.push(then0->ConnectedTo->ParentNode);
                           }
                           return -1; // 我们已经手动管理了栈，所以返回-1
                       }});

    registry.Registry({"flow.for_loop", "Flow Control", {{"Exec In", PinType::Execution, nullptr}, {"First Index", PinType::Data, &TypeRegistry::Get<int>()}, {"Last Index", PinType::Data, &TypeRegistry::Get<int>()}, {"Loop Body", PinType::Execution, nullptr}, {"Index", PinType::Data, &TypeRegistry::Get<int>()}, {"Completed", PinType::Execution, nullptr}}, [](ExecutionContext &ctx) -> int
                       {
                           // 检查内部状态来判断是首次进入还是迭代
                           int currentIndex;
                           bool isFirstRun = (ctx.CurrentNode->InternalState.GetTypeInfo() == nullptr);

                           if (isFirstRun)
                           {
                               // 首次运行：从输入引脚初始化
                               currentIndex = ctx.GetInputValue("First Index").GetValue<int>();
                               std::cout << "[FLOW] ForLoop Started.\n";
                           }
                           else
                           {
                               // 迭代：从内部状态获取当前索引
                               currentIndex = ctx.CurrentNode->InternalState.GetValue<int>();
                           }

                           int lastIndex = ctx.GetInputValue("Last Index").GetValue<int>();

                           if (currentIndex <= lastIndex)
                           {
                               // 仍在循环内
                               std::cout << "[FLOW] ForLoop Index: " << currentIndex << "\n";
                               // 1. 设置当前循环的输出数据值
                               ctx.SetOutputValue("Index", currentIndex);

                               // 2. 准备下一次迭代的索引并存入内部状态
                               ctx.CurrentNode->InternalState = currentIndex + 1;

                               // 3. 将自己（ForLoop节点）推回栈中，以便进行下一次迭代检查
                               ctx.ExecutionStack.push(ctx.CurrentNode);

                               // 4. 将 "Loop Body" 的执行目标推入栈
                               Pin *loopBodyPin = ctx.CurrentNode->FindPinByName("Loop Body", PinDirection::Output);
                               if (loopBodyPin && loopBodyPin->ConnectedTo)
                               {
                                   ctx.ExecutionStack.push(loopBodyPin->ConnectedTo->ParentNode);
                               }
                           }
                           else
                           {
                               // 循环结束
                               std::cout << "[FLOW] ForLoop Completed.\n";
                               // 1. 清理内部状态，以便下次可以重新开始循环
                               ctx.CurrentNode->InternalState = Variant();

                               // 2. 将 "Completed" 的执行目标推入栈
                               Pin *completedPin = ctx.CurrentNode->FindPinByName("Completed", PinDirection::Output);
                               if (completedPin && completedPin->ConnectedTo)
                               {
                                   ctx.ExecutionStack.push(completedPin->ConnectedTo->ParentNode);
                               }
                           }
                           return -1; // 手动管理栈
                       }});

    registry.Registry({"flow.do_once", "Flow Control", {{"Enter", PinType::Execution, nullptr}, {"Reset", PinType::Execution, nullptr}, // 增加一个重置输入
                                                        {"Exit", PinType::Execution, nullptr},
                                                        {"Completed", PinType::Data, &TypeRegistry::Get<bool>()}},
                       [](ExecutionContext &ctx) -> int
                       {
                           bool hasCompleted = false;
                           if (ctx.CurrentNode->InternalState.GetTypeInfo() == &TypeRegistry::Get<bool>())
                           {
                               hasCompleted = ctx.CurrentNode->InternalState.GetValue<bool>();
                           }

                           // 确定是哪个执行引脚触发了此节点
                           // (这需要更复杂的上下文信息，简单起见，我们假设无法区分)
                           // 改进：一个节点定义应该能知道哪个输入引脚被触发。
                           // 为了简单，我们先用一个逻辑：如果完成了，就不再执行。
                           // Reset的逻辑需要单独处理。

                           // **简化版逻辑**
                           // 假设我们只关心 "Enter"
                           if (!hasCompleted)
                           {
                               std::cout << "[FLOW] DoOnce: Executing for the first time.\n";
                               ctx.CurrentNode->InternalState = true; // 标记为已完成
                               ctx.SetOutputValue("Completed", true);
                               return 0; // 激活 "Exit"
                           }
                           else
                           {
                               std::cout << "[FLOW] DoOnce: Already executed, skipping.\n";
                               ctx.SetOutputValue("Completed", true);
                               // 不激活任何输出，阻塞流程
                               return -1;
                           }
                           // 一个完整的DoOnce需要知道是被Enter还是Reset触发，这需要对执行器做进一步扩展。
                           // 目前这个版本可以工作，但没有Reset功能。
                       }});

    // ====================================================================
    // 数学 & 逻辑运算
    // ====================================================================

    registry.Registry({"math.add_int", "Math", {// 执行引脚可选，使其既可作为纯数据节点，也可在流程中使用
                                                {"Exec In", PinType::Execution, nullptr},
                                                {"A", PinType::Data, &TypeRegistry::Get<int>()},
                                                {"B", PinType::Data, &TypeRegistry::Get<int>()},
                                                {"Exec Out", PinType::Execution, nullptr},
                                                {"Result", PinType::Data, &TypeRegistry::Get<int>()}},
                       [](ExecutionContext &ctx) -> int
                       {
                           int a = ctx.GetInputValue("A").GetValue<int>();
                           int b = ctx.GetInputValue("B").GetValue<int>();
                           int result = a + b;
                           ctx.SetOutputValue("Result", result);
                           std::cout << "[EXEC] Add: " << a << " + " << b << " = " << result << "\n";
                           return 0; // 如果有执行流，则继续
                       }});

    registry.Registry({"logic.and", "Logic", {{"A", PinType::Data, &TypeRegistry::Get<bool>()}, {"B", PinType::Data, &TypeRegistry::Get<bool>()}, {"Result", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx) -> int
                       {
                           bool a = ctx.GetInputValue("A").GetValue<bool>();
                           // 短路求值: 如果a是false，就没必要获取b的值了
                           if (!a)
                           {
                               ctx.SetOutputValue("Result", false);
                               return -1;
                           }
                           bool b = ctx.GetInputValue("B").GetValue<bool>();
                           ctx.SetOutputValue("Result", a && b);
                           return -1;
                       }});

    registry.Registry({"logic.or", "Logic", {{"A", PinType::Data, &TypeRegistry::Get<bool>()}, {"B", PinType::Data, &TypeRegistry::Get<bool>()}, {"Result", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx) -> int
                       {
                           bool a = ctx.GetInputValue("A").GetValue<bool>();
                           // 短路求值
                           if (a)
                           {
                               ctx.SetOutputValue("Result", true);
                               return -1;
                           }
                           bool b = ctx.GetInputValue("B").GetValue<bool>();
                           ctx.SetOutputValue("Result", a || b);
                           return -1;
                       }});

    registry.Registry({"logic.not", "Logic", {{"In", PinType::Data, &TypeRegistry::Get<bool>()}, {"Result", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx) -> int
                       {
                           bool in = ctx.GetInputValue("In").GetValue<bool>();
                           ctx.SetOutputValue("Result", !in);
                           return -1;
                       }});

    registry.Registry({"compare.equal_int", "Comparison", {{"A", PinType::Data, &TypeRegistry::Get<int>()}, {"B", PinType::Data, &TypeRegistry::Get<int>()}, {"Result", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx) -> int
                       {
                           int a = ctx.GetInputValue("A").GetValue<int>();
                           int b = ctx.GetInputValue("B").GetValue<int>();
                           ctx.SetOutputValue("Result", a == b);
                           return -1;
                       }});

    registry.Registry({"compare.greater_int", "Comparison", {{"A", PinType::Data, &TypeRegistry::Get<int>()}, {"B", PinType::Data, &TypeRegistry::Get<int>()}, {"Result", PinType::Data, &TypeRegistry::Get<bool>()}}, [](ExecutionContext &ctx) -> int
                       {
                           int a = ctx.GetInputValue("A").GetValue<int>();
                           int b = ctx.GetInputValue("B").GetValue<int>();
                           ctx.SetOutputValue("Result", a > b);
                           return -1;
                       }});
}
