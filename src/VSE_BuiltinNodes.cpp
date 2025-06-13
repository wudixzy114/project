#include "VSE_BuiltinNodes.h"
#include "VSE_NodeFactory.h"
#include <iostream>
#include <string>

using namespace VSE;

namespace NodeFunctions
{
    int Add(int A, int B) { return A + B; }
    bool And(bool A, bool B) { return A && B; }
    bool IsEqual(int A, int B) { return A == B; }

    void PrintString(ExecToken, const std::string &In)
    {
        std::cout << "VSE_LOG: " << In << std::endl;
    }
} // namespace NodeFunctinos

void VSE::RegisterBuiltinNodes()
{
    std::cout << "Registering built-in VSE nodes using NodeFactory...\n";

    // 使用新的 NodeFactory 注册节点
    NodeFactory::Register("math.add_int", "Math", {"A", "B"}, &NodeFunctions::Add);
    NodeFactory::Register("logic.and", "Logic", {"A", "B"}, &NodeFunctions::And);
    NodeFactory::Register("compare.equal_int", "Comparison", {"A", "B"}, &NodeFunctions::IsEqual);

    // 注册带执行流的节点
    NodeFactory::Register("io.print_string", "IO", {"Exec In", "In"}, &NodeFunctions::PrintString);
}