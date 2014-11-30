#include "translation_utils.hpp"
#include "errors.hpp"

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

} // namespace mathvm