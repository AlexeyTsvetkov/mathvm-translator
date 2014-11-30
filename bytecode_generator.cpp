#include "bytecode_generator.hpp"
#include "errors.hpp"
#include "info.hpp"
#include "translation_utils.hpp"
#include "utils.hpp"

#include <iostream>
#include <utility>

#include <cstdlib>
#include <cassert>

namespace mathvm {

Status* BytecodeGenerator::generate() {
  ctx()->addFunction(top_);
  visit(top_);
  return Status::Ok();
}

void BytecodeGenerator::visit(AstFunction* function) {
  ctx()->enterFunction(function);
  
  if (!isTopLevel(function)) { 
    parameters(function);
  } 

  visit(function->node());
  ctx()->exitFunction();
}

void BytecodeGenerator::parameters(AstFunction* function) {
  Scope* scope = function->scope();
  ctx()->enterScope(scope);
  visit(scope);
  
  AstNode* node = function->node();
  int64_t parametersNumber = function->parametersNumber();

  for (int64_t i = parametersNumber - 1; i >= 0; --i) {
    const std::string& name = function->parameterName(i);
    AstVar* param = findVariable(name, scope, node);
    VarInfo* info = getInfo<VarInfo>(param);
    
    Instruction insn;
    switch (param->type()) {
      case VT_INT:    insn = BC_STOREIVAR; break;
      case VT_DOUBLE: insn = BC_STOREDVAR; break;
      default:
        throw TranslationException(node, "Illegal parameter type (int/double expected)");
        return;
    }

    bc()->addInsn(insn);
    bc()->addUInt16(info->localId());
  }

  ctx()->exitScope();
}

void BytecodeGenerator::visit(Scope* scope) {
  Scope::VarIterator varIt(scope);
  while (varIt.hasNext()) {
    AstVar* var = varIt.next();
    ctx()->declare(var);
  }

  // Functions can call other functions defined
  // later in same scope, so add them before visit
  Scope::FunctionIterator addFunIt(scope);
  while (addFunIt.hasNext()) {
    ctx()->addFunction(addFunIt.next()); 
  }

  Scope::FunctionIterator funIt(scope);
  while (funIt.hasNext()) {
    visit(funIt.next()); 
  }
}

void BytecodeGenerator::visit(FunctionNode* function) {
  function->body()->visit(this);
  // If reached from top-level fun, it's OK.
  // Otherwise it means that there's no return in fun,
  // so stop execution anyway.
  bc()->addInsn(BC_STOP); 
}

void BytecodeGenerator::visit(BlockNode* block) {
  Scope* scope = block->scope();
  ctx()->enterScope(scope);
  visit(scope);
  
  for (uint32_t i = 0; i < block->nodes(); ++i) {
    AstNode* statement = block->nodeAt(i);
    statement->visit(this);
    
    if (hasNonEmptyStack(statement)) {
      bc()->addInsn(BC_POP);
    } 
  }

  ctx()->exitScope();
}

void BytecodeGenerator::visit(NativeCallNode* node) { node->visitChildren(this); }
void BytecodeGenerator::visit(ForNode* node) { node->visitChildren(this); }
void BytecodeGenerator::visit(IfNode* node) { node->visitChildren(this); }
void BytecodeGenerator::visit(WhileNode* node) { node->visitChildren(this); }

void BytecodeGenerator::readVarInfo(const AstVar* var, uint16_t& localId, uint16_t& context) {
  VarInfo* info = getInfo<VarInfo>(var);
  uint16_t varFunctionId = info->functionId();
  uint16_t curFunctionId = ctx()->currentFunctionId();
  localId = info->localId();
  context = 0;

  if (varFunctionId != curFunctionId) {
    InterpreterFunction* varFunction = ctx()->functionById(varFunctionId);
    InterpreterFunction* curFunction = ctx()->functionById(curFunctionId);

    int32_t varFunctionDeep = varFunction->deepness();
    int32_t curFunctionDeep = curFunction->deepness();
    // var function deepness always >= current function deepness
    context = static_cast<uint16_t>(curFunctionDeep - varFunctionDeep);
  } 
}

void BytecodeGenerator::visit(LoadNode* node) { 
  uint16_t localId;
  uint16_t context;
  readVarInfo(node->var(), localId, context);
  loadVar(node, localId, context, bc());
  setType(node, node->var()->type());
}

void BytecodeGenerator::visit(StoreNode* node) { 
  node->visitChildren(this); 
  
  const AstVar* var = node->var();
  VarType type = var->type();
  if (!isNumeric(type)) {
    throw TranslationException(node, "Variable can`t have not-numeric type");
  }

  uint16_t localId;
  uint16_t context;
  readVarInfo(var, localId, context);
  bool isInt = type == VT_INT;

  switch (typeOf(node->value())) {
    case VT_INT:
      if (!isInt) {
        bc()->addInsn(BC_D2I);
      };

      break;
    case VT_DOUBLE:
      if (isInt) {
        bc()->addInsn(BC_I2D);
      }

      break;
    default:
      throw TranslationException(node, "Wrong RHS type");
  }

  if (node->op() != tASSIGN) {
    loadVar(node, localId, context, bc());
  }

  switch (node->op()) {
    case tINCRSET: bc()->addInsn(isInt ? BC_IADD : BC_DADD); break;
    case tDECRSET: bc()->addInsn(isInt ? BC_ISUB : BC_DSUB); break;
    default: break; 
  }

  if (context != 0) {
    bc()->addInsn(isInt ? BC_STORECTXIVAR : BC_STORECTXDVAR);
    bc()->addUInt16(context);
  } else {
    bc()->addInsn(isInt ? BC_STOREIVAR : BC_STOREDVAR);
  }

  bc()->addUInt16(localId);
}

void BytecodeGenerator::visit(PrintNode* node) { 
  for (uint32_t i = 0; i < node->operands(); ++i) {
    AstNode* operand = node->operandAt(i);
    operand->visit(this);
    
    Instruction insn;
    switch (typeOf(operand)) {
      case VT_INT:    insn = BC_IPRINT; break;
      case VT_DOUBLE: insn = BC_DPRINT; break;
      case VT_STRING: insn = BC_SPRINT; break;
      default:
        throw TranslationException(node, "Print is only applicable to int, double, string");
    }
    bc()->addInsn(insn);
  }
}

void BytecodeGenerator::visit(ReturnNode* node) { 
  AstNode* returnExpr = node->returnExpr();
  
  if (returnExpr) {
    returnExpr->visit(this);
    cast(returnExpr, ctx()->currentFunction()->returnType(), bc());
  }

  bc()->addInsn(BC_RETURN); 
}

void BytecodeGenerator::visit(CallNode* node) { 
  AstFunction* function = findFunction(node->name(), ctx()->currentScope(), node);
  
  if (node->parametersNumber() != function->parametersNumber()) {
    throw TranslationException(node, "Invocation has wrong argument number");
  }
  
  for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
    AstNode* argument = node->parameterAt(i);
    argument->visit(this);
    cast(argument, function->parameterType(i), bc());
  }  

  bc()->addInsn(BC_CALL);
  bc()->addUInt16(ctx()->getId(function));
  setType(node, function->returnType());
}

void BytecodeGenerator::visit(BinaryOpNode* op) { 
  switch (op->kind()) {
    case tOR:
    case tAND:
      logicalOp(op);
      break;

    case tAOR:
    case tAAND:
    case tAXOR:
      bitwiseOp(op);
      break;

    case tEQ:
    case tNEQ:
    case tGT:
    case tGE:
    case tLT:
    case tLE:
      comparisonOp(op);
      break;

    case tADD:
    case tSUB:
    case tDIV:
    case tMUL:
    case tMOD:
      arithmeticOp(op);
      break;

    default:
      throw TranslationException(op, "Unknown binary operator");
  }
}

void BytecodeGenerator::visit(UnaryOpNode* op) { 
  op->visitChildren(this);
  
  
  switch (op->kind()) {
    case tSUB: negOp(op); break;
    case tNOT: notOp(op); break;
    default:
      throw TranslationException(op, "Unknown unary operator");
  }
}

void BytecodeGenerator::visit(DoubleLiteralNode* floating) {
  bc()->addInsn(BC_DLOAD);
  bc()->addDouble(floating->literal());
  setType(floating, VT_DOUBLE);
}

void BytecodeGenerator::visit(IntLiteralNode* integer) {
  bc()->addInsn(BC_ILOAD);
  bc()->addInt64(integer->literal());
  setType(integer, VT_INT);
}

void BytecodeGenerator::visit(StringLiteralNode* string) {
  uint16_t id = ctx()->makeStringConstant(string->literal());
  bc()->addInsn(BC_SLOAD);
  bc()->addUInt16(id);
  setType(string, VT_STRING);
}

void BytecodeGenerator::negOp(UnaryOpNode* op) {
  switch (typeOf(op->operand())) {
    case VT_INT: 
      bc()->addInsn(BC_INEG);
      setType(op, VT_INT);
      break;
    case VT_DOUBLE: 
      bc()->addInsn(BC_DNEG);
      setType(op, VT_DOUBLE);
      break;
    default:
      throw TranslationException(op, "Unary sub (-) is only applicable to int/double");
  }
}

void BytecodeGenerator::notOp(UnaryOpNode* op) {
  if (typeOf(op->operand()) != VT_INT) {
    throw TranslationException(op, "Unary not (!) is only applicable to int");
  }

  Label setFalse(bc());
  Label end(bc());

  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPNE, setFalse);
  bc()->addInsn(BC_ILOAD1);
  bc()->addBranch(BC_JA, end);
  bc()->bind(setFalse);
  bc()->addInsn(BC_ILOAD0);
  bc()->bind(end);
  setType(op, VT_INT);
}

void BytecodeGenerator::logicalOp(BinaryOpNode* op) {
TokenKind token = op->kind();
  if (token != tAND && token != tOR) {
    throw TranslationException(op, "Unknown logical operator");
  }
  
  bool isAnd = (token == tAND);
  Label evaluateRight(bc());
  Label setTrue(bc());
  Label end(bc());

  op->left()->visit(this);
  
  bc()->addInsn(BC_ILOAD0);
  if (isAnd) {
    bc()->addBranch(BC_IFICMPNE, evaluateRight);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_JA, end);
  } else {
    bc()->addBranch(BC_IFICMPNE, setTrue);
  }

  bc()->bind(evaluateRight);
  op->right()->visit(this);
    
  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    throw TranslationException(op, "Logical operators are only applicable integer operands");
  }

  // only reachable in cases:
  // 1. true AND right
  // 2. false OR right
  // so if right is true, whole is true
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_IFICMPNE, setTrue);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_JA, end);

  bc()->bind(setTrue);
  bc()->addInsn(BC_ILOAD1);
  bc()->bind(end);

  setType(op, VT_INT);
}

void BytecodeGenerator::bitwiseOp(BinaryOpNode* op) {
  op->visitChildren(this);

  
  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    throw TranslationException(op, "Bitwise operator is only applicable to int operands");
  }

  switch (op->kind()) {
    case tAOR:
      bc()->addInsn(BC_IAOR);
      break;
    case tAAND:
      bc()->addInsn(BC_IAAND);
      break;
    case tAXOR:
      bc()->addInsn(BC_IAXOR);
      break;
    default:
      throw TranslationException(op, "Unknown bitwise binary operator");
  } 
}

void BytecodeGenerator::comparisonOp(BinaryOpNode* op) {
  op->visitChildren(this);
  
  VarType operandsCommonType = castOperandsNumeric(op);
  
  Instruction cmpi;
  switch (op->kind()) {
    case tEQ:  cmpi = BC_IFICMPE;  break;
    case tNEQ: cmpi = BC_IFICMPNE; break;
    case tGT:  cmpi = BC_IFICMPG;  break;
    case tGE:  cmpi = BC_IFICMPGE; break;
    case tLT:  cmpi = BC_IFICMPL;  break;
    case tLE:  cmpi = BC_IFICMPLE; break;
    default:
      throw TranslationException(op, "Unknown comparison operator");
  }

  Label setTrue(bc());
  Label end(bc());

  bc()->addInsn(operandsCommonType == VT_INT ? BC_ICMP : BC_DCMP);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(cmpi, setTrue);
  bc()->addInsn(BC_ILOAD0);
  bc()->addBranch(BC_JA, end);
  bc()->bind(setTrue);
  bc()->addInsn(BC_ILOAD1);
  bc()->bind(end);

  setType(op, VT_INT);
}

void BytecodeGenerator::arithmeticOp(BinaryOpNode* op) {
  op->visitChildren(this);
  
  VarType operandsCommonType = castOperandsNumeric(op);
  
  bool isInt = operandsCommonType == VT_INT;
  Instruction insn = BC_INVALID;

  switch (op->kind()) {
    case tADD: insn = isInt ? BC_IADD : BC_DADD; break;
    case tSUB: insn = isInt ? BC_ISUB : BC_DSUB; break;
    case tMUL: insn = isInt ? BC_IMUL : BC_DMUL; break;
    case tDIV: insn = isInt ? BC_IDIV : BC_DDIV; break;
    case tMOD:
      if (isInt) {
        insn = BC_IMOD;
      } else {
        throw TranslationException(op, "Modulo (%) is only applicable to integers");
      }
      break;
    default:
      throw TranslationException(op, "Unknown arithmetic binary operator");
  }

  bc()->addInsn(insn);
  setType(op, operandsCommonType);
}

VarType BytecodeGenerator::castOperandsNumeric(BinaryOpNode* op) {
  VarType tLower = typeOf(op->left());
  VarType tUpper = typeOf(op->right());
  
  if (!isNumeric(tLower) || !isNumeric(tUpper)) {
    throw TranslationException(op, "Operator is only applicable to numbers");
  }

  bool isInt = tLower == VT_INT && tUpper == VT_INT;

  if (!isInt && tLower == VT_INT) {
    bc()->addInsn(BC_SWAP);
    bc()->addInsn(BC_I2D);
    bc()->addInsn(BC_SWAP);
  }

  if (!isInt && tUpper == VT_INT) {
    bc()->addInsn(BC_I2D);
  }

  return isInt ? VT_INT : VT_DOUBLE;
}

} // namespace mathvm