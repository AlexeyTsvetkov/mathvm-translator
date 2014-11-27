#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "ast.h"
#include "mathvm.h"

#include "info.hpp"
#include "interpreter_code.hpp"
#include "report.hpp"

#include <stack>
#include <string>
#include <vector>

namespace mathvm {

class Context {
  InterpreterCodeImpl* code_;
  Report report_;
  std::stack<uint16_t> functionIds_;
  std::stack<Scope*> scopes_;
  std::vector<VarInfo*> varInfos_; 

public:
  Context(InterpreterCodeImpl* code)
    : code_(code), 
      report_() {}

  ~Context() {
    for (size_t i = 0; i < varInfos_.size(); ++i) {
      delete varInfos_[i];
    }
  }

  Report* report() {
    return &report_;
  }

  void enterFunction(AstFunction* function) {
    uint16_t deepness = static_cast<uint16_t>(functionIds_.size());
    uint16_t id = code_->addFunction(new InterpreterFunction(function, deepness));
    functionIds_.push(id);
  }

  void exitFunction() {
    assert(!functionIds_.empty());
    functionIds_.pop();
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

  InterpreterFunction* functionById(uint16_t id) {
    return code_->functionById(id);
  }

  uint16_t makeStringConstant(const std::string& string) {
    return code_->makeStringConstant(string);
  }

  void declare(AstVar* var) {
    InterpreterFunction* function = functionById(currentFunctionId());
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