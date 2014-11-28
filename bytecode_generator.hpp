#ifndef BYTECODE_GENERATOR_HPP
#define BYTECODE_GENERATOR_HPP 

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "interpreter_code.hpp"
#include "context.hpp"

#include <map>
#include <stack>
#include <string>

namespace mathvm {

  class BytecodeGenerator : public AstVisitor {
    AstFunction* top_;
    Context context_;

  public:
    BytecodeGenerator(AstFunction* top, InterpreterCodeImpl* code)
     : top_(top), 
       context_(code) {} 

    Status* generate();

  #define VISITOR_FUNCTION(type, name)     \
    void visit(type* node);                \
    virtual void visit##type(type* node) { \
      if (shouldVisit(node)) {             \
        visit(node);                       \
      }                                    \
    }                                      \

    FOR_NODES(VISITOR_FUNCTION)
  #undef VISITOR_FUNCTION

  private:
    void visit(Scope* scope);
    void visit(AstFunction* function);
    void negOp(UnaryOpNode* op);
    void notOp(UnaryOpNode* op);
    void logicalOp(BinaryOpNode* op);
    void bitwiseOp(BinaryOpNode* op);
    void comparisonOp(BinaryOpNode* op);
    void arithmeticOp(BinaryOpNode* op);
    VarType castOperandsNumeric(BinaryOpNode* op);
    void readVarInfo(const AstVar* var, uint16_t& localId, uint16_t& context);
    void loadVar(VarType type, uint16_t localId, uint16_t context, AstNode* node);
    void cast(VarType from, VarType to, AstNode* node);

    bool shouldVisit(AstNode* node) {
      return !report()->isError();
    }

    Bytecode* bc() {
      uint16_t id = ctx()->currentFunctionId();
      Bytecode* bytecode = ctx()->bytecodeByFunctionId(id);
      assert(bytecode != NULL);
      return bytecode;
    }

    Context* ctx() {
      return &context_;
    }

    Report* report() {
      return ctx()->report();
    }
  };
}

#endif 