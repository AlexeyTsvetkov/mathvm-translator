#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include "mathvm.h"
#include "interpreter_code.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <cassert>
#include <cstddef>

#include <algorithm>
#include <vector>

namespace mathvm {

typedef int32_t mem_t;

namespace constants {
  const mem_t MAX_STACK_SIZE = 1024*1024*1024;
  const mem_t VAL_SIZE = std::max(sizeof(int64_t), sizeof(double));
}

class StackFrame {
  uint16_t function_;
  uint32_t instruction_;
  mem_t parentFrame_;
  mem_t variablesOffset_;

public:
  StackFrame(uint16_t function, uint32_t instruction, mem_t parentFrame, mem_t variablesOffset)
    : function_(function),
      instruction_(instruction),
      parentFrame_(parentFrame),
      variablesOffset_(variablesOffset) {}

  StackFrame(const StackFrame& other) 
    : function_(other.function_),
      instruction_(other.instruction_),
      parentFrame_(other.parentFrame_),
      variablesOffset_(other.variablesOffset_) {}

  StackFrame& operator=(const StackFrame& other) {
    function_ = other.function_;
    instruction_ = other.instruction_;
    parentFrame_ = other.parentFrame_;
    variablesOffset_ = other.variablesOffset_;
    return *this;
  }

  uint16_t function() const { return function_; }
  uint32_t instruction() const { return instruction_; }
  mem_t parentFrame() const { return parentFrame_; }
  mem_t variablesOffset() const { return variablesOffset_; }
};

class BytecodeInterpreter {
  char* stack_;
  std::vector<StackFrame> frames_;

  InterpreterCodeImpl* code_;
  BytecodeFunction* function_;
  uint32_t instructionPointer_;
  mem_t operandsOffset_;
  mem_t variablesOffset_;

public:
  BytecodeInterpreter(Code* code);
  ~BytecodeInterpreter();
  void execute();

private:
  void allocFrame(uint16_t functionId, uint32_t localsNumber, int64_t context);
  void callFunction(uint16_t id);
  void returnFunction();

  template<typename T>
  T* findVar(uint16_t id, uint16_t context) {
    mem_t variables;

    if (frames_.empty()) {
      variables = variablesOffset_;
    } else {
      mem_t frame = static_cast<mem_t>(frames_.size()) - 1;

      for (uint16_t i = 0; i < context; ++i) {
        frame = frames_.at(frame).parentFrame();
      }

      variables = frames_.at(frame).variablesOffset();
    }
    
    return reinterpret_cast<T*> (stack_ + variables + id * constants::VAL_SIZE);
  }

  template<typename T>
  void loadVar(uint16_t id, uint16_t context) {
    push(*findVar<T>(id, context));
  }

  template<typename T>
  void loadLocalVar() {
    uint16_t ctx = 0;
    uint16_t id = readFromBcAndShift<uint16_t>();
    loadVar<T>(id, ctx); 
  }

  template<typename T>
  void loadClosureVar() {
    uint16_t ctx = readFromBcAndShift<uint16_t>();
    uint16_t id = readFromBcAndShift<uint16_t>();
    loadVar<T>(id, ctx); 
  }

  template<typename T>
  void storeVar(uint16_t id, uint16_t context, T val) {
    *findVar<T>(id, context) = val;
  }

  template<typename T>
  void storeLocalVar() {
    uint16_t ctx = 0;
    uint16_t id = readFromBcAndShift<uint16_t>();
    T value = pop<T>();
    storeVar<T>(id, ctx, value); 
  }

  template<typename T>
  void storeClosureVar() {
    uint16_t ctx = readFromBcAndShift<uint16_t>();
    uint16_t id = readFromBcAndShift<uint16_t>();
    T value = pop<T>();
    storeVar<T>(id, ctx, value); 
  }


  void remove() {
    operandsOffset_ -= constants::VAL_SIZE;
  }

  template<typename T>
  T* operand() {
    return reinterpret_cast<T*> (stack_ + operandsOffset_);
  }

  template<typename T>
  T pop() {
    operandsOffset_ -= constants::VAL_SIZE;
    return *operand<T>();
  }

  template<typename T>
  void push(T val) {
    *operand<T>() = val; 
    operandsOffset_ += constants::VAL_SIZE;
  }

  template<typename T>
  void load() {
    T val = readFromBc<T>();
    instructionPointer_ += sizeof(T);
    push<T>(val);
  }

  void swap() {
    int64_t upper = pop<int64_t>();
    int64_t lower = pop<int64_t>();
    push(upper);
    push(lower);
  }


  Bytecode* bc() {
    return function_->bytecode();
  }

  template<typename T>
  T readFromBc() {
    T val = bc()->getTyped<T>(instructionPointer_);
    return val;
  }

  template<typename T>
  T readFromBcAndShift() {
    T val = bc()->getTyped<T>(instructionPointer_);
    instructionPointer_ += sizeof(T);
    return val;
  }
};

} // namespace mathvm
#endif