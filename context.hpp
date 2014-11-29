#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "ast.h"
#include "mathvm.h"

#include "info.hpp"
#include "interpreter_code.hpp"

#include <map>
#include <stack>
#include <string>
#include <vector>
#include <utility>

namespace mathvm {

class Context {
  typedef std::map<AstFunction*, uint16_t> IdByFunctionMap;

  InterpreterCodeImpl* code_;
  std::stack<uint16_t> functionIds_;
  std::stack<Scope*> scopes_;
  std::vector<VarInfo*> varInfos_; 
  IdByFunctionMap idByFunction_;

public:
  Context(InterpreterCodeImpl* code)
    : code_(code) {}

  ~Context() {
    for (size_t i = 0; i < varInfos_.size(); ++i) {
      delete varInfos_[i];
    }
  }

  void addFunction(AstFunction* function) {
    uint16_t deepness = static_cast<uint16_t>(functionIds_.size());
    uint16_t id = code_->addFunction(new InterpreterFunction(function, deepness));
    idByFunction_.insert(std::make_pair(function, id));
  }

  void enterFunction(AstFunction* function) {
    functionIds_.push(getId(function));
  }

  void exitFunction() {
    assert(!functionIds_.empty());
    functionIds_.pop();
  }

  uint16_t getId(AstFunction* function) {
    IdByFunctionMap::iterator it = idByFunction_.find(function);
    assert(it != idByFunction_.end());
    return it->second;
  }

  uint16_t currentFunctionId() const {
    assert(!functionIds_.empty());
    return functionIds_.top();
  }

  void enterScope(Scope* scope) {
    scopes_.push(scope);
  }

  void exitScope() {
    assert(!scopes_.empty());
    scopes_.pop();
  }

  Scope* currentScope() const {
    assert(!scopes_.empty());
    return scopes_.top();
  }

  Bytecode* bytecodeByFunctionId(uint16_t id) {
    BytecodeFunction* function = functionById(id);
    return (function == NULL) ? NULL : function->bytecode();
  }

  InterpreterFunction* currentFunction() const {
    return functionById(currentFunctionId());
  }

  InterpreterFunction* functionById(uint16_t id) const {
    return code_->functionById(id);
  }

  uint16_t makeStringConstant(const std::string& string) {
    return code_->makeStringConstant(string);
  }

  void declare(AstVar* var) {
    InterpreterFunction* function = currentFunction();
    uint16_t functionId = function->id();
    uint16_t localsNumber = static_cast<uint16_t>(function->localsNumber());
    VarInfo* info = new VarInfo(functionId, localsNumber++);
    function->setLocalsNumber(localsNumber);
    var->setInfo(info);
    varInfos_.push_back(info);
  }
};

} // namespace mathvm

#endif