#pragma once
#include "VSE_Graph.h"
#include <functional>

namespace VSE
{
    class GraphExecutor;

    struct ExecutionContext
    {
        GraphExecutor *Executor;
        Node *CurrentNode;
        std::map<int, Variant> &ValueCache;

        Variant GetInputValue(const std::string &pinName)
        {
            for (const auto &pin : CurrentNode->InputPins)
            {
                if (pinName == pin.Name && pin.Type == PinType::Data)
                {
                    if (pin.ConnectedTo)
                    {
                        if (ValueCache.count(pin.ConnectedTo->ID))
                        {
                            return ValueCache.at(pin.ConnectedTo->ID);
                        }
                    }
                    return pin.DefaultValue;
                }
            }
            return Variant();
        }

        void SetOutputValue(const std::string &pinName, Variant value)
        {
            for (auto &pin : CurrentNode->OutputPins)
            {
                if (pin.Name == pinName && pin.Type == PinType::Data)
                {
                    ValueCache[pin.ID] = value;
                    return;
                }
            }
        }
    };

    struct NodeDefinition
    {
        std::string TypeName;
        std::string Category;
        std::vector<std::tuple<std::string, PinType, const TypeInfo *>> PinDefinitions;
        std::function<int(ExecutionContext &)> Execute;
    };

    class NodeRegistry
    {
    public:
        static NodeRegistry &Instance()
        {
            static NodeRegistry instance;
            return instance;
        }

        void Registry(const NodeDefinition &def)
        {
            m_Definitions[def.TypeName] = def;
        }

        const NodeDefinition *Get(const std::string &typeName) const
        {
            auto it = m_Definitions.find(typeName);
            return it != m_Definitions.end() ? &it->second : nullptr;
        }

        const auto &GetAll() const
        {
            return m_Definitions;
        }

    private:
        NodeRegistry() = default;
        std::map<std::string, NodeDefinition> m_Definitions;
    };
}