#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "interpreter_code.hpp"
#include "error_reporter.hpp"
#include "abstract_expression.hpp"

#include <map>
#include <stack>
#include <string>

namespace mathvm {

  class BytecodeGenerator : public AstBaseVisitor {
    typedef std::stack<uint16_t> FunctionIdStack;
    typedef std::stack<Scope*> ScopeStack;
    typedef std::map<Scope*, uint16_t> ScopeIdMap;

    AstFunction* top_;
    InterpreterCodeImpl* code_;
    FunctionIdStack functionIds_;
    ScopeStack scopes_;
    ScopeIdMap functionIdByScope_;
    ErrorReporter reporter_;
    ExpressionType expr_;

  public:
    BytecodeGenerator(AstFunction* top, InterpreterCodeImpl* code)
     : top_(top), 
       code_(code),
       reporter_(),
       expr_(reporter_) {} 

    Status* generate();

    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitBlockNode(BlockNode* node);
    //virtual void visitNativeCallNode(NativeCallNode* node);
    //virtual void visitForNode(ForNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitWhileNode(WhileNode* node);
    //virtual void visitLoadNode(LoadNode* node);
    //virtual void visitStoreNode(StoreNode* node);
    virtual void visitPrintNode(PrintNode* node);
    //virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);

  private:
    void visitAstFunction(AstFunction* function);
    void visitScope(Scope* scope);
    void negOp(AstNode* operand);
    void notOp(AstNode* operand);
    void logicalOp(BinaryOpNode* node);
    void logicalAnd(BinaryOpNode* node);
    void logicalOr(BinaryOpNode* node);
    void bitwiseOp(BinaryOpNode* node);
    void comparisonOp(BinaryOpNode* node);
    void arithmeticOp(BinaryOpNode* node);
    bool operands(BinaryOpNode* node);
    void swap();

    Scope* currentScope() {
      assert(!scopes_.empty());
      return scopes_.top();
    }

    void enterScope(Scope* scope) {
      uint16_t id = currentFunctionId();
      functionIdByScope_.insert(std::make_pair(scope, id));
      scopes_.push(scope);
    }

    void exitScope() {
      assert(!scopes_.empty());
      scopes_.pop();
    }

    uint16_t currentFunctionId() {
      assert(!functionIds_.empty());
      return functionIds_.top();
    }

    BytecodeFunction* functionByName(const string& name) {
      return code_->functionByName(name);
    }

    uint16_t functionIdByScope(Scope* scope) {
      ScopeIdMap::iterator it = functionIdByScope_.find(scope);
      assert(it != functionIdByScope_.end());
      return it->second;
    }

    Bytecode* bc() {
      BytecodeFunction* function = code_->functionById(currentFunctionId());
      assert(function != NULL);
      return function->bytecode();
    }

    ExpressionType& expr() { return expr_; }

    ErrorReporter& status() { return reporter_; }
  };
}