#ifndef TRANSLATION_UTILS_HPP
#define TRANSLATION_UTILS_HPP 

#include "ast.h"
#include "mathvm.h"

#include <string>

namespace mathvm {

AstVar* findVariable(const std::string& name, Scope* scope, AstNode* at);
AstFunction* findFunction(const std::string& name, Scope* scope, AstNode* at);

bool isTopLevel(AstFunction* function);
bool isTopLevel(FunctionNode* function);
bool isNumeric(VarType type);
bool hasNonEmptyStack(const AstNode* node);

} // namespace mathvm

#endif