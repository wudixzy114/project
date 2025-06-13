#pragma once
#include <tuple>
#include <functional>

namespace VSE::details
{
    template <typename T, T... S, typename F>
    constexpr void for_sequence(std::integer_sequence<T, S...>, F &&f)
    {
        (static_cast<void>(f(std::integral_constant<T, S>{})), ...);
    }

    template <typename T>
    struct function_traits : function_traits<decltype(&T::operator())>
    {
    };

    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType (*)(Args...)>
    {
        static constexpr size_t arity = sizeof...(Args);
        using result_type = ReturnType;

        template <size_t i>
        struct arg
        {
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType (ClassType::*)(Args...) const>
    {
        static constexpr size_t arity = sizeof...(Args);
        using result_type = ReturnType;

        template <size_t i>
        struct arg
        {
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };
}