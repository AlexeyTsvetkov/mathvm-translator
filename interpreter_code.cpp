#include "ast.h"

#include "interpreter_code.hpp"

using namespace mathvm;

uint32_t InterpreterFunction::declareLocal(Scope* scope, const std::string& name) {
  LocalScopeMap::iterator nameMapIt = locals_.find(scope);
  if (nameMapIt == locals_.end()) {
    locals_.insert(std::make_pair(scope, LocalNameMap()));
    nameMapIt = locals_.find(scope);
  }

  LocalNameMap& nameMap = nameMapIt->second;
  LocalNameMap::iterator id = nameMap.find(name);
  if (id != nameMap.end()) {
    return id->second;
  }

  uint32_t newId = localsNumber();
  nameMap.insert(std::make_pair(name, newId));
  setLocalsNumber(newId + 1);
  return newId;
}

bool InterpreterFunction::lookupLocal(Scope* scope, const std::string& name, uint32_t& id) {
  LocalScopeMap::iterator nameMapIt = locals_.find(scope);
  if (nameMapIt == locals_.end()) {
    return false;
  }

  LocalNameMap& nameMap = nameMapIt->second;
  LocalNameMap::iterator idIt = nameMap.find(name);
  if (idIt == nameMap.end()) {
    return false;
  }

  id = idIt->second;
  return true;
} 