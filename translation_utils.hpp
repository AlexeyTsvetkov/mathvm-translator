#ifndef TRANSLATION_UTILS_HPP
#define TRANSLATION_UTILS_HPP 

#include "ast.h"
#include "mathvm.h"

#include <string>

namespace mathvm {

AstVar* findVariable(const std::string& name, Scope* scope, AstNode* at);
AstFunction* findFunction(const std::string& name, Scope* scope, AstNode* at);

} // namespace mathvm

#endif