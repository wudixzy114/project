#pragma once
#include <any>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <iostream>

namespace VSE
{
    struct TypeInfo
    {
        std::type_index Index;
        std::string Name;
    };

    class TypeRegistry;

    class Variant
    {
    public:
        Variant() : m_TypeInfo(nullptr) {}

        template <typename T>
        Variant(T value)
        {
            Assign(std::forward<T>(value));
        }

        template <typename T>
        T &Get() &
        {
            ValidateType<T>();
            return std::any_cast<T &>(m_Data);
        }

        template <typename T>
        T &&Get() &&
        {
            ValidateType<T>();
            return std::any_cast<T &&>(std::move(m_Data));
        }

        template <typename T>
        const T &&Get() const &&
        {
            ValidateType<T>();
            return std::any_cast<const T &&>(std::move(m_Data));
        }

        template <typename T>
        const T &Get() const &
        {
            ValidateType<T>();
            return std::any_cast<const T &>(m_Data);
        }

        template <typename T>
        T GetValue() const
        {
            ValidateType<T>();
            return std::any_cast<T>(m_Data);
        }

        const TypeInfo *GetTypeInfo() const
        {
            return m_TypeInfo;
        }

    private:
        template <typename T>
        void Assign(T &&value);

        template <typename T>
        void ValidateType() const
        {
            if (!m_Data.has_value() || m_TypeInfo->Index != typeid(std::decay_t<T>))
            {
                throw std::bad_any_cast();
            }
        }

        std::any m_Data;
        const TypeInfo *m_TypeInfo;
    };

    class TypeRegistry
    {
    public:
        static TypeRegistry &Instance()
        {
            static TypeRegistry instance;
            return instance;
        }

        template <typename T>
        static void Registry(const std::string &name)
        {
            Instance().m_Types.emplace(typeid(std::decay_t<T>), TypeInfo{typeid(std::decay_t<T>), name});
        }

        template <typename T>
        static const TypeInfo &Get()
        {
            auto it = Instance().m_Types.find(typeid(T));
            if (it == Instance().m_Types.end())
            {
                throw std::runtime_error("Type not registered: " + std::string(typeid(T).name()));
            }
            return it->second;
        }

    private:
        TypeRegistry() = default;
        std::unordered_map<std::type_index, TypeInfo> m_Types;
    };

    template <typename T>
    void Variant::Assign(T &&value)
    {
        // 使用 std::decay_t 获取基础类型
        using PureType = std::decay_t<T>;
        m_TypeInfo = &TypeRegistry::Get<PureType>();
        m_Data = std::forward<T>(value);
    }
}
