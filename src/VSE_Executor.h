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
    enum class EvalutationState
    {
        NotEvaluated,
        Evaluating,
        Evaluated
    };

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
            std::map<int, Variant> valueCache;
            std::map<int, EvalutationState> nodeStates;
            std::stack<Node *> executionStack;

            if (triggerNode)
            {
                executionStack.push(triggerNode);
            }

            while (!executionStack.empty())
            {
                Node *currentNode = executionStack.top();
                executionStack.pop();

                if (!currentNode)
                {
                    continue;
                }

                ExecutionContext context{
                    this, currentNode, valueCache, nodeStates, executionStack};

                int outputExecPinIndex = currentNode->Definition->Execute(context);

                if (outputExecPinIndex >= 0)
                {
                    Pin *nextExecPin = FindExecutionOutput(currentNode, outputExecPinIndex);
                    if (nextExecPin && nextExecPin->ConnectedTo)
                    {
                        executionStack.push(nextExecPin->ConnectedTo->ParentNode);
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

        void ResolveDataForNode(Node *node, std::map<int, Variant> &valueCache, std::map<int, EvalutationState> &nodeStates)
        {
            auto it = nodeStates.find(node->ID);
            if (it == nodeStates.end())
            {
                it->second = EvalutationState::NotEvaluated;
            }
            EvalutationState state = it->second;
            if (state == EvalutationState::Evaluated)
            {
                return;
            }
            if (state == EvalutationState::Evaluating)
            {
                throw std::runtime_error("Cyclic data dependency detected at node: " + node->Title);
            }
            nodeStates[node->ID] = EvalutationState::Evaluating;
            for (const auto &pin : node->InputPins)
            {
                if (pin.Type == PinType::Data && pin.ConnectedTo)
                {
                    Node *upstreamNode = pin.ConnectedTo->ParentNode;
                    ResolveDataForNode(upstreamNode, valueCache, nodeStates);
                }
            }
            std::stack<Node *> dummyStack;
            ExecutionContext context{
                this, node, valueCache, nodeStates, dummyStack};

            node->Definition->Execute(context);
            nodeStates[node->ID] = EvalutationState::Evaluated;
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