#pragma once
#include "VSE_Registry.h"
#include "VSE_FunctionTraits.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace VSE
{
    class NodeFactory
    {
    public:
        template <typename Func>
        static void Register(
            const std::string &typeName,
            const std::string &category,
            const std::vector<std::string> &argNames,
            Func &&func)
        {
            using traits = details::function_traits<std::decay_t<Func>>;
            if (argNames.size() != traits::arity)
            {
                throw std::logic_error("Argument names count mismatch for node: " + typeName);
            }

            NodeDefinition def;
            def.TypeName = typeName;
            def.Category = category;
            int inputExecCount = 0;
            int outputExecCount = 0;

            details::for_sequence(std::make_index_sequence<traits::arity>{}, [&](auto i)
                                  {

                using ArgType = typename traits::template arg<i>::type;
                const std::string& pinName = argNames[i]; 
                if constexpr (std::is_same_v<std::decay_t<ArgType>, ExecToken>) {
                    def.PinDefinitions.emplace_back(pinName, PinType::Execution, nullptr);
                    inputExecCount++;
                } else {
                    def.PinDefinitions.emplace_back(pinName, PinType::Data, &TypeRegistry::Get<std::decay_t<ArgType>>());
                } });

            if constexpr (!std::is_same_v<typename traits::result_type, void>)
            {
                if constexpr (std::is_same_v<typename traits::result_type, ExecToken>)
                {
                    // 如果返回 ExecToken, 意味着有多个执行输出
                    // 这种情况需要更复杂的处理，暂时简化
                }
                else
                {
                    def.PinDefinitions.emplace_back("Result", PinType::Data, &TypeRegistry::Get<typename traits::result_type>());
                }
            }

            if (inputExecCount > 0)
            {
                def.PinDefinitions.emplace_back("Exec Out", PinType::Execution, nullptr);
                outputExecCount++;
            }

            // --- 自动生成 Execute lambda ---
            def.Execute = [func, argNames, inputExecCount](ExecutionContext &ctx) -> int
            {
                // 创建一个元组来存放从输入引脚获取的值
                std::tuple<typename traits::template arg<0>::type> args_tuple; // 仅为示例，实际需要完整元组

                // C++17 折叠表达式来填充元组
                auto tuple_of_args = details::for_sequence_to_tuple(std::make_index_sequence<traits::arity>{}, [&](auto i)
                                                                    {
                    using ArgType = typename traits::template arg<i>::type;
                    const std::string& pinName = argNames[i];
                    
                    if constexpr (std::is_same_v<std::decay_t<ArgType>, ExecToken>) {
                        return ExecToken{};
                    } else {
                        return ctx.GetInputValue(pinName).GetValue<std::decay_t<ArgType>>();
                    } });

                // 调用原始函数
                if constexpr (std::is_same_v<typename traits::result_type, void>)
                {
                    std::apply(func, tuple_of_args);
                    // 假设如果第一个参数是ExecToken, 那么就有一个默认的执行输出
                    return (inputExecCount > 0) ? 0 : -1;
                }
                else
                {
                    auto result = std::apply(func, tuple_of_args);
                    ctx.SetOutputValue("Result", result);
                    return (inputExecCount > 0) ? 0 : -1;
                }
            };

            NodeRegistry::Instance().Registry(def);
        }

    private:
        template <typename T, T... S, typename F>
        static constexpr auto for_sequence_to_tuple(std::integer_sequence<T, S...>, F &&f)
        {
            return std::make_tuple(f(std::integral_constant<T, S>{})...);
        }
    };
}