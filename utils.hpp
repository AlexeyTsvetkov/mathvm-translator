#ifndef UTILS_HPP
#define UTILS_HPP

#include "ast.h"
#include "mathvm.h"
#include "info.hpp"

#include <iostream>

namespace mathvm {

template<typename A1>
void debug(A1 a1) {
  std::cout << a1;
  std::cout << std::endl;
}

template<typename A1, typename A2>
void debug(A1 a1, A2 a2) {
  std::cout << a1 << a2;
  std::cout << std::endl;
}

template<typename A1, typename A2, typename A3>
void debug(A1 a1, A2 a2, A3 a3) {
  std::cout << a1 << a2 << a3;
  std::cout << std::endl;
}

inline bool isTopLevel(AstFunction* function) {
  return function->name() == AstFunction::top_name;
}

inline bool isTopLevel(FunctionNode* function) {
  return function->name() == AstFunction::top_name;
}

inline bool isNumeric(VarType type) {
  return type == VT_INT || type == VT_DOUBLE;
}

inline bool hasNonEmptyStack(const AstNode* node) {
  if (node->isCallNode()) {
    return typeOf(node) != VT_VOID;
  }

  return node->isBinaryOpNode() 
         || node->isUnaryOpNode()
         || node->isStringLiteralNode()
         || node->isDoubleLiteralNode()
         || node->isIntLiteralNode()
         || node->isLoadNode();
}

} // namespace mathvm

#endif