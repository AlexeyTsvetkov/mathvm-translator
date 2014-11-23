#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"

#include <map>
#include <string>
#include <utility>

#include <stdint.h>

namespace mathvm {

class InterpreterFunction : public BytecodeFunction {
  typedef std::map<std::string, uint32_t> LocalNameMap;
  typedef std::map<Scope*, LocalNameMap> LocalScopeMap;

  LocalScopeMap locals_;
public:
  InterpreterFunction(AstFunction* function)
    : BytecodeFunction(function),
      locals_() {}

  virtual ~InterpreterFunction() {}

  uint32_t declareLocal(Scope* scope, const std::string& name);

  bool lookupLocal(Scope* scope, const std::string& name, uint32_t& id);
};

class InterpreterCodeImpl : public Code {
  public:
    InterpreterCodeImpl() {}
    virtual ~InterpreterCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
      return Status::Error("Not implemented InterpreterCodeImpl::execute");
    }

    InterpreterFunction* functionByName(const string& name) {
      return dynamic_cast<InterpreterFunction*>(Code::functionByName(name));
    }

    InterpreterFunction* functionById(uint16_t id) {
      return dynamic_cast<InterpreterFunction*>(Code::functionById(id));
    }
};

}

#endif 