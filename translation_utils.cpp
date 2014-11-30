#include "translation_utils.hpp"
#include "errors.hpp"
#include "info.hpp"

namespace mathvm {

AstVar* findVariable(const std::string& name, Scope* scope, AstNode* at) {
  AstVar* var = scope->lookupVariable(name);

  if (var == 0) {
    throw TranslationException(at, "Variable '%s' is not defined", name.c_str());
  }

  return var;
}

AstFunction* findFunction(const std::string& name, Scope* scope, AstNode* at) {
  AstFunction* function = scope->lookupFunction(name);

  if (function == 0) {
    throw TranslationException(at, "Function '%s' is not defined", name.c_str());
  }

  return function;
}

bool isTopLevel(AstFunction* function) {
  return function->name() == AstFunction::top_name;
}

bool isTopLevel(FunctionNode* function) {
  return function->name() == AstFunction::top_name;
}

bool isNumeric(VarType type) {
  return type == VT_INT || type == VT_DOUBLE;
}

bool hasNonEmptyStack(const AstNode* node) {
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