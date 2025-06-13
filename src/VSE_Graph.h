#pragma once
#include "VSE_Types.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>

namespace VSE
{
    enum class PinType
    {
        Execution,
        Data
    };

    enum class PinDirection
    {
        Input,
        Output
    };

    struct Pin;
    struct Node;
    struct Link;
    class Graph;

    struct Pin
    {
        int ID;
        Node *ParentNode;
        std::string Name;
        PinType Type;
        PinDirection Direction;

        const TypeInfo *DataType;
        Variant DefaultValue;

        Pin *ConnectedTo = nullptr;
    };

    struct Node
    {
        int ID;
        std::string Title;
        std::vector<Pin> InputPins;
        std::vector<Pin> OutputPins;

        float PosX = 0.0f;
        float PosY = 0.0f;

        class NodeDefinition *Definition = nullptr;

        Variant InternalState;

        int InDegree;

        Pin *FindPinByName(const std::string &name, PinDirection direction)
        {
            if (direction == PinDirection::Input)
            {
                for (auto &pin : InputPins)
                {
                    if (pin.Name == name)
                        return &pin;
                }
            }
            else
            {
                for (auto &pin : OutputPins)
                {
                    if (pin.Name == name)
                        return &pin;
                }
            }
            return nullptr;
        }
    };

    struct Link
    {
        int ID;
        Pin *fromPin;
        Pin *toPin;
    };

    class Graph
    {
    public:
        Graph() = default;
        ~Graph() = default;
        Graph(const Graph &) = delete;
        Graph &operator=(const Graph &) = delete;

        const std::string &GetName() const { return m_Name; }
        void SetName(const std::string &name) { m_Name = name; }
        const std::vector<std::unique_ptr<Node>> &GetNodes() const { return m_Nodes; }
        const std::vector<std::unique_ptr<Link>> &GetLinks() const { return m_Links; }
        Node *FindNodeByID(int id) const
        {
            auto it = m_NodeMap.find(id);
            return it == m_NodeMap.end() ? nullptr : it->second;
        }
        Node *FindNodeByTitle(const std::string &title) const
        {
            for (const auto &node : m_Nodes)
            {
                if (node->Title == title)
                    return node.get();
            }
            return nullptr;
        }
        Pin *FindPinByID(int id) const
        {
            auto it = m_PinMap.find(id);
            return it == m_PinMap.end() ? nullptr : it->second;
        }
        Node *AddNode(std::unique_ptr<Node> node)
        {
            if (!node)
                return nullptr;

            Node *rawPtr = node.get();
            m_Nodes.push_back(std::move(node));

            m_NodeMap[rawPtr->ID] = rawPtr;
            for (auto &pin : rawPtr->InputPins)
            {
                m_PinMap[pin.ID] = &pin;
            }
            for (auto &pin : rawPtr->OutputPins)
            {
                m_PinMap[pin.ID] = &pin;
            }
            return rawPtr;
        }
        void RemoveNode(int nodeId)
        {
            Node *nodeToRemove = FindNodeByID(nodeId);
            if (!nodeToRemove)
                return;
            m_NodeMap.erase(nodeId);
            for (const auto &pin : nodeToRemove->InputPins)
                m_PinMap.erase(pin.ID);
            for (const auto &pin : nodeToRemove->OutputPins)
                m_PinMap.erase(pin.ID);
            m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                                         [nodeId](const auto &node)
                                         { return node->ID == nodeId; }),
                          m_Nodes.end());
            std::vector<int> linksToRemove;
            for (const auto &link : m_Links)
            {
                if (link->fromPin->ParentNode->ID == nodeId || link->toPin->ParentNode->ID == nodeId)
                {
                    linksToRemove.push_back(link->ID);
                }
            }
            for (int linkId : linksToRemove)
            {
                RemoveLink(linkId);
            }
        }
        void RemoveLink(int linkId)
        {
            m_LinkMap.erase(linkId);
            m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
                                         [linkId](const auto &link)
                                         { return link->ID == linkId; }),
                          m_Links.end());
        }
        Link *AddLink(std::unique_ptr<Link> link)
        {
            if (!link)
                return nullptr;
            Link *rawPtr = link.get();
            m_LinkMap[rawPtr->ID] = rawPtr;
            m_Links.push_back(std::move(link));
            return rawPtr;
        }
        void SetVariable(const std::string &name, Variant value)
        {
            m_Variables[name] = value;
        }
        Variant *GetVariable(const std::string &name)
        {
            auto it = m_Variables.find(name);
            return it != m_Variables.end() ? &it->second : nullptr;
        }

    private:
        std::string m_Name;
        std::vector<std::unique_ptr<Node>> m_Nodes;
        std::vector<std::unique_ptr<Link>> m_Links;
        std::map<std::string, Variant> m_Variables;

        std::map<int, Pin *> m_PinMap;
        std::map<int, Node *> m_NodeMap;
        std::map<int, Link *> m_LinkMap;
    };
}