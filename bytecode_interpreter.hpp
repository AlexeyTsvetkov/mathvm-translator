#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include "mathvm.h"
#include "interpreter_code.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <cassert>
#include <cstddef>

namespace mathvm {

namespace constants {
  const int64_t MAX_STACK_SIZE = 128000000;
}

class BytecodeInterpreter {
  static const int64_t VAL_SIZE = 8;

  InterpreterCodeImpl* code_;
  BytecodeFunction* function_;
  char* stack_;
  uint32_t ins_;
  int64_t sp_;

public:
  BytecodeInterpreter(Code* code): ins_(0), sp_(0) {
    code_ = dynamic_cast<InterpreterCodeImpl*>(code);
    assert(code_ != NULL);
    function_ = loadFunction(0);
    stack_ = new char[constants::MAX_STACK_SIZE];
  }

  ~BytecodeInterpreter() {
    delete [] stack_;
  }

  void execute();

private:
  BytecodeFunction* loadFunction(uint16_t id) {
    return code_->functionById(id);
  } 

  Bytecode* bc() {
    return function_->bytecode();
  }

  void remove() {
    sp_ -= VAL_SIZE;
  }

  template<typename T>
  T pop() {
    sp_ -= VAL_SIZE;
    return *((T*) (stack_ + sp_));
  }

  template<typename T>
  void push(T val) {
    *((T*) (stack_ + sp_)) = val; 
    sp_ += VAL_SIZE;
  }

  template<typename T>
  void load() {
    T val = readFromBc<T>();
    ins_ += sizeof(T);
    push<T>(val);
  }

  template<typename T>
  T readFromBc() {
    T val = bc()->getTyped<T>(ins_);
    return val;
  }

  void swap() {
    int64_t upper = pop<int64_t>();
    int64_t lower = pop<int64_t>();
    push(upper);
    push(lower);
  }
};

} // namespace mathvm
#endif