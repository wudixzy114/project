#pragma once
#include "VSE_Registry.h"
#include "VSE_Graph.h"
#include <stack>
#include <set>
#include <queue>
#include <vector>
#include <stdexcept>
#include <functional>
#include <algorithm>

namespace VSE
{
    class GraphExecutor
    {
    public:
        GraphExecutor(Graph *graph) : m_Graph(graph)
        {
            if (!graph)
            {
                throw std::invalid_argument("Graph cannot be null.");
            }
            BuildAdjacency();
        }

        void TriggerEvent(Node *triggerNode)
        {
            Node *startNode = triggerNode;
            std::queue<Node *> executionQueue;
            executionQueue.push(startNode);
            std::map<int, Variant> valueCache;
            while (!executionQueue.empty())
            {
                Node *currentNode = executionQueue.front();
                executionQueue.pop();
                if (!currentNode)
                {
                    continue;
                }

                if (!ResolveDataDependencies(currentNode, valueCache))
                {
                    std::cerr << "Error: Cyclic dependency detected or failed to resolve data for node " << currentNode->Title << std::endl;
                    throw std::runtime_error("Error: Cyclic dependency detected or failed to resolve data for node");
                }

                ExecutionContext context{
                    this,
                    currentNode,
                    valueCache};

                int outputExecPinIndex = currentNode->Definition->Execute(context);
                if (outputExecPinIndex >= 0)
                {
                    Pin *newExecPin = FindExecutionOutput(currentNode, outputExecPinIndex);
                    if (newExecPin && newExecPin->ConnectedTo)
                    {
                        executionQueue.push(newExecPin->ConnectedTo->ParentNode);
                    }
                }
            }
        }

    private:
        Graph *m_Graph;
        void BuildAdjacency()
        {
            for (const auto &node : m_Graph->GetNodes())
            {
                for (auto &pin : node->InputPins)
                    pin.ConnectedTo = nullptr;
                for (auto &pin : node->OutputPins)
                    pin.ConnectedTo = nullptr;
            }
            for (const auto &link : m_Graph->GetLinks())
            {
                if (link->fromPin && link->toPin)
                {
                    link->fromPin->ConnectedTo = link->toPin;
                    link->toPin->ConnectedTo = link->fromPin;
                }
            }
        }

        bool ResolveDataDependencies(Node *targetNode, std::map<int, Variant> &valueCache)
        {
            std::queue<Node *> dataQueue;    // 用于拓扑排序的队列
            std::vector<Node *> sortedNodes; // 拓扑排序的结果
            std::map<int, int> inDegreeMap;  // <NodeID, InDegree>
            std::vector<Node *> relevantNodes;

            std::function<void(Node *)> buildSubgraph = [&](Node *n)
            {
                relevantNodes.push_back(n);
                inDegreeMap[n->ID] = 0;
                for (const auto &pin : n->InputPins)
                {
                    if (pin.Type == PinType::Data && pin.ConnectedTo)
                    {
                        Node *upstreamNode = pin.ConnectedTo->ParentNode;
                        // 如果上游节点还没被访问过
                        if (std::find(relevantNodes.begin(), relevantNodes.end(), upstreamNode) == relevantNodes.end())
                        {
                            buildSubgraph(upstreamNode);
                        }
                    }
                }
            };

            buildSubgraph(targetNode);

            for (Node *n : relevantNodes)
            {
                for (const auto &pin : n->InputPins)
                {
                    if (pin.Type == PinType::Data && pin.ConnectedTo)
                    {
                        if (std::find(relevantNodes.begin(), relevantNodes.end(), pin.ConnectedTo->ParentNode) != relevantNodes.end())
                        {
                            inDegreeMap[n->ID]++;
                        }
                    }
                }
            }

            for (Node *n : relevantNodes)
            {
                if (inDegreeMap[n->ID] == 0)
                {
                    dataQueue.push(n);
                }
            }

            while (!dataQueue.empty())
            {
                Node *u = dataQueue.front();
                dataQueue.pop();
                sortedNodes.push_back(u);

                for (const auto &outPin : u->OutputPins)
                {
                    if (outPin.Type == PinType::Data && outPin.ConnectedTo)
                    {
                        Node *v = outPin.ConnectedTo->ParentNode;
                        if (inDegreeMap.count(v->ID))
                        {
                            inDegreeMap[v->ID]--;
                            if (inDegreeMap[v->ID] == 0)
                            {
                                dataQueue.push(v);
                            }
                        }
                    }
                }
            }

            if (sortedNodes.size() != relevantNodes.size())
            {
                return false;
            }

            for (Node *nodeToEvaluate : sortedNodes)
            {
                if (nodeToEvaluate == targetNode)
                {
                    continue;
                }
                ExecutionContext context{
                    this, nodeToEvaluate, valueCache};
                nodeToEvaluate->Definition->Execute(context);
            }

            return true;
        }

        Pin *FindExecutionOutput(Node *node, int index)
        {
            int currentIndex = 0;
            for (auto &pin : node->OutputPins)
            {
                if (pin.Type == PinType::Execution)
                {
                    if (currentIndex == index)
                    {
                        return &pin;
                    }
                    currentIndex++;
                }
            }
            return nullptr;
        }

        friend struct ExecutionContext;
    };
}