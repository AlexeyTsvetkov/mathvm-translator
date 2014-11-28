#include "bytecode_generator.hpp"
#include "info.hpp"
#include "utils.hpp"

#include <iostream>
#include <utility>

#include <cstdlib>
#include <cassert>

namespace mathvm {

Status* BytecodeGenerator::generate() {
  visit(top_);
  return report()->release();
}

void BytecodeGenerator::visit(AstFunction* function) {
  ctx()->enterFunction(function);
  
  if (!isTopLevel(function)) { 
    visit(function->scope());
  }

  visit(function->node());
  ctx()->exitFunction();
}

void BytecodeGenerator::visit(Scope* scope) {
  ctx()->enterScope(scope);

  Scope::VarIterator varIt(scope);
  while (varIt.hasNext()) {
    AstVar* var = varIt.next();
    ctx()->declare(var);
  }

  Scope::FunctionIterator funIt(scope);
  while (funIt.hasNext()) {
    visit(funIt.next()); 
  }

  ctx()->exitScope();
}

void BytecodeGenerator::visit(FunctionNode* function) {
  function->body()->visit(this);

  if (isTopLevel(function)) {
    bc()->addInsn(BC_STOP);
  } else {
    bc()->addInsn(BC_RETURN);
  }
}

void BytecodeGenerator::visit(BlockNode* block) {
  visit(block->scope());
  block->visitChildren(this);
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

void BytecodeGenerator::loadVar(VarType type, uint16_t localId, uint16_t context, AstNode* node) {
  switch (type) {
    case VT_INT:
      if (context != 0) {
        bc()->addInsn(BC_LOADCTXIVAR);
        bc()->addUInt16(context);
      } else {
        bc()->addInsn(BC_LOADIVAR);
      }
      
      bc()->addUInt16(localId);
      break;
    case VT_DOUBLE:
      if (context != 0) {
        bc()->addInsn(BC_LOADCTXDVAR);
        bc()->addUInt16(context);
      } else {
        bc()->addInsn(BC_LOADDVAR);
      }
      
      bc()->addUInt16(localId);
      break;
    default:
      report()->error("Wrong var reference type (only numbers are supported)", node);
  }
}

void BytecodeGenerator::visit(LoadNode* node) { 
  const AstVar* var = node->var();
  uint16_t localId;
  uint16_t context;
  readVarInfo(var, localId, context);
  loadVar(var->type(), localId, context, node);
  setType(node, var->type());
}

void BytecodeGenerator::visit(StoreNode* node) { 
  node->visitChildren(this); 
  if (report()->isError()) return;

  const AstVar* var = node->var();
  VarType type = var->type();
  if (!isNumeric(type)) {
    report()->error("Variable can`t have not-numeric type", node);
    return;
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
      report()->error("Wrong RHS type", node);
      return;
  }

  if (node->op() != tASSIGN) {
    loadVar(type, localId, context, node);
    if (report()->isError()) return;
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
    if (report()->isError()) return;

    Instruction insn;
    switch (typeOf(operand)) {
      case VT_INT:    insn = BC_IPRINT; break;
      case VT_DOUBLE: insn = BC_DPRINT; break;
      case VT_STRING: insn = BC_SPRINT; break;
      default:
        report()->error("Print is only applicable to int, double, string", operand);
        return;
    }
    bc()->addInsn(insn);
  }
}

void BytecodeGenerator::visit(ReturnNode* node) { node->visitChildren(this); }
void BytecodeGenerator::visit(CallNode* node) { node->visitChildren(this); }

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
      report()->error("Unknown binary operator", op);
  }
}

void BytecodeGenerator::visit(UnaryOpNode* op) { 
  op->visitChildren(this);
  
  if (report()->isError()) return;

  switch (op->kind()) {
    case tSUB: negOp(op); break;
    case tNOT: notOp(op); break;
    default:
      report()->error("Unknown unary operator", op);
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
      report()->error("Unary sub (-) is only applicable to int/double", op);
  }
}

void BytecodeGenerator::notOp(UnaryOpNode* op) {
  if (typeOf(op->operand()) != VT_INT) {
    report()->error("Unary not (!) is only applicable to int", op);
    return;
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
    report()->error("Unknown logical operator", op);
    return;
  }
  
  bool isAnd = (token == tAND);
  Label evaluateRight(bc());
  Label setTrue(bc());
  Label end(bc());

  op->left()->visit(this);
  if (report()->isError()) return;

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
  if (report()->isError()) return;
  
  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    report()->error("Logical operators are only applicable integer operands", op);
    return;
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

  if (report()->isError()) return;

  if (typeOf(op->left()) != VT_INT || typeOf(op->right()) != VT_INT) {
    report()->error("Bitwise operator is only applicable to int operands", op);
    return;
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
      report()->error("Unknown bitwise binary operator", op);
  } 
}

void BytecodeGenerator::comparisonOp(BinaryOpNode* op) {
  op->visitChildren(this);
  if (report()->isError()) return;

  VarType operandsCommonType = castOperandsNumeric(op);
  if (report()->isError()) return;

  Instruction cmpi;
  switch (op->kind()) {
    case tEQ:  cmpi = BC_IFICMPE;  break;
    case tNEQ: cmpi = BC_IFICMPNE; break;
    case tGT:  cmpi = BC_IFICMPG;  break;
    case tGE:  cmpi = BC_IFICMPGE; break;
    case tLT:  cmpi = BC_IFICMPL;  break;
    case tLE:  cmpi = BC_IFICMPLE; break;
    default:
      report()->error("Unknown comparison operator", op);
      return;
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
  if (report()->isError()) return;

  VarType operandsCommonType = castOperandsNumeric(op);
  if (report()->isError()) return;

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
        report()->error("Modulo (%) is only applicable to integers", op);
      }
      break;
    default:
      report()->error("Unknown arithmetic binary operator", op);
  }

  bc()->addInsn(insn);
  setType(op, operandsCommonType);
}

VarType BytecodeGenerator::castOperandsNumeric(BinaryOpNode* op) {
  VarType tLower = typeOf(op->left());
  VarType tUpper = typeOf(op->right());
  
  if (!isNumeric(tLower) || !isNumeric(tUpper)) {
    report()->error("Operator is only applicable to numbers", op);
    return VT_INVALID;
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