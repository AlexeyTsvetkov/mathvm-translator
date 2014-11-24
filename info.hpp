#ifndef INFO_HPP
#define INFO_HPP 

#include "ast.h"
#include "mathvm.h"

#include <cassert>
#include <cstdlib>

namespace mathvm {

class TypeInfo {
public:
  TypeInfo(VarType type): type_(type) {};
  TypeInfo(): type_(VT_INVALID) {};
  ~TypeInfo() {}

  VarType getType() { return type_; }
  void setType(VarType type) { type_ = type; }

private:
  VarType type_;
};

template<typename InfoT>
InfoT* getInfo(CustomDataHolder* dataHolder) {
  return dynamic_cast<InfoT*> dataHolder->info();
}

#endif