#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include "Type.h"

extern FILE *yyout;
int Node::counter = 0;
IRBuilder* Node::builder = nullptr;

Node::Node()
{
    seq = counter++;
    next=nullptr;
}

void Node::setNext(Node* node) {
    Node* n = this;
    while (n->getNext()) 
        n = n->getNext();//????β
    if (n == this) {
        this->next = node;
    } else {
        n->setNext(node);
    }
}


void Node::backPatch(std::vector<Instruction*> &list, BasicBlock*bb)
{
    for(auto &inst:list)
    {
        if(inst->isCond())
            dynamic_cast<CondBrInstruction*>(inst)->setTrueBranch(bb);
        else if(inst->isUncond())
            dynamic_cast<UncondBrInstruction*>(inst)->setBranch(bb);
    }
}

std::vector<Instruction*> Node::merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2)
{
    std::vector<Instruction*> res(list1);
    res.insert(res.end(), list2.begin(), list2.end());
    return res;
}

FunctionUse::FunctionUse(SymbolEntry *se,ExprNode* expr):ExprNode(se), expr(expr)
{
    //?????functionuse?????????ж??????????
    dst = nullptr;
    SymbolEntry* s = se;
    int paramCnt = 0;//?????????????θ???
    ExprNode* temp = expr;
    while (temp) {
        paramCnt++;
        temp = (ExprNode*)(temp->getNext());
    }
    while (s) {
        Type* type = s->getType();   //?????????entry
        std::vector<Type*> params = ((FunctionType*)type)->getParamsType();   //?βε?vector
        int t=0;
        if ((long unsigned int)paramCnt == params.size()) {
            ExprNode* temp = expr;
            for(int i=0;i<paramCnt;i++)
            {
                if(temp->getType()!=params[i])
                {
                    t=1;
                    break;
                }
                temp = (ExprNode*)(temp->getNext());
            }
            if(t==0)
            {
                this->symbolEntry = s;
                break;
            }
        }//????????????type
        s = s->getNext();//???????????
    }
    if (symbolEntry) {
        Type* type = symbolEntry->getType();
        this->type = ((FunctionType*)type)->getRetType();
        if (this->type != TypeSystem::voidType) {
            SymbolEntry* se =
                new TemporarySymbolEntry(this->type, SymbolTable::getLabel());
            dst = new Operand(se);
        }
        std::vector<Type*> params = ((FunctionType*)type)->getParamsType();//?β?
        ExprNode* temp = expr;//???
        for (auto it = params.begin(); it != params.end(); it++) {
            if (temp == nullptr) {
                fprintf(stderr, "too few arguments to function %s %s\n",
                        symbolEntry->toStr().c_str(), type->toStr().c_str());
                break;
            } 
            else if ((*it)->getKind() != temp->getType()->getKind())
                fprintf(stderr, "parameter's type %s can't convert to %s\n",
                        temp->getType()->toStr().c_str(),
                        (*it)->toStr().c_str());
            temp = (ExprNode*)(temp->getNext());
        }
        if (temp != nullptr) {
            fprintf(stderr, "too many arguments to function %s %s\n",
                    symbolEntry->toStr().c_str(), type->toStr().c_str());
        }
    }
}

void Ast::genCode(Unit *unit)
{
    IRBuilder *builder = new IRBuilder(unit);
    Node::setIRBuilder(builder);
    root->genCode();
}

void FunctionDef::genCode()
{
    Unit *unit = builder->getUnit();
    Function *func = new Function(unit, se);
    BasicBlock *entry = func->getEntry();
    // set the insert point to the entry basicblock of this function.
    builder->setInsertBB(entry);

    if(decl){
        decl->genCode();
    }
    if(stmt){
        stmt->genCode();
    }       

    /**
     * Construct control flow graph. You need do set successors and predecessors for each basic block.
     * Todo
    */
   for(auto block = func->begin(); block != func->end(); block++){
        //获取该块的最后一条指令
        Instruction* i = (*block)->begin();
        Instruction* last = (*block)->rbegin();
        //基本块中不能跳转，不应包含条件型语句，删掉，另外直接删除有风险，新建块？？？
        while (i != last) {
            if (i->isCond() || i->isUncond()) {
                (*block)->remove(i);
            }
            i = i->getNext();
        }
        //连接每个块的前后块，分情况：条件跳转获取真假分支，回填，无条件跳转直接获取跳转的目标块连接
        if (last->isCond()) {
            BasicBlock *truebranch, *falsebranch;
            truebranch =
                dynamic_cast<CondBrInstruction*>(last)->getTrueBranch();
            falsebranch =
                dynamic_cast<CondBrInstruction*>(last)->getFalseBranch();
                
            
            (*block)->addSucc(truebranch);
            (*block)->addSucc(falsebranch);
            truebranch->addPred(*block);
            falsebranch->addPred(*block);
        } 
        else if (last->isUncond())
        {
            BasicBlock* dst =
                dynamic_cast<UncondBrInstruction*>(last)->getBranch();
            (*block)->addSucc(dst);            
            dst->addPred(*block);
            //目标块为空插入return
            if (dst->empty()) {
                if (((FunctionType*)(se->getType()))->getRetType() ==
                    TypeSystem::intType)
                    new RetInstruction(new Operand(new ConstantSymbolEntry(
                                           TypeSystem::intType, 0)),
                                       dst);
                else if (((FunctionType*)(se->getType()))->getRetType() ==
                         TypeSystem::voidType)
                    new RetInstruction(nullptr, dst);
            }
        }
        //没有返回或跳转的语句插入空返回指令
        else if ((!last->isRet())&&
((FunctionType*)(se->getType()))->getRetType() ==TypeSystem::voidType) {
                new RetInstruction(nullptr, *block);
            }
   }


}

void BinaryExpr::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Function *func = bb->getParent();
    if (op == AND)
    {
        BasicBlock *trueBB = new BasicBlock(func);  // if the result of lhs is true, jump to the trueBB.
        expr1->genCode();
        backPatch(expr1->trueList(), trueBB);
        builder->setInsertBB(trueBB);               // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
        expr2->genCode();
        true_list = expr2->trueList();
        false_list = merge(expr1->falseList(), expr2->falseList());
    }
    else if(op == OR)
    {
        // Todo
        BasicBlock* trueBB = new BasicBlock(func);
        expr1->genCode();
        backPatch(expr1->falseList(), trueBB);//只有当expr1为假时才去判断expr2，也就是跳转到trueBB，因此此处回填的时expr1的falselist
        builder->setInsertBB(trueBB);
        expr2->genCode();
        true_list = merge(expr1->trueList(), expr2->trueList());//有一个真就行
        false_list = expr2->falseList();
    }
    else if(op >= LESS && op <= MORE)
    {
        // Todo
        expr1->genCode();
        expr2->genCode();
        Operand* src1 = expr1->getOperand();
        Operand* src2 = expr2->getOperand();
        //0扩展 将i1转换为i32  关系比较_i32  
        if (src1->getType()->getSize() == 1) {
            Operand* dst = new Operand(new TemporarySymbolEntry(
                TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(dst, src1, bb);
            src1 = dst;
        }
        if (src2->getType()->getSize() == 1) {
            Operand* dst = new Operand(new TemporarySymbolEntry(
                TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(dst, src2, bb);
            src2 = dst;
        }
        int cmpopcode = -1;
        switch (op) {
            case LESS:
                cmpopcode = CmpInstruction::L;
                break;
            case EQULESS:
                cmpopcode = CmpInstruction::LE;
                break;
            case MORE:
                cmpopcode = CmpInstruction::G;
                break;
            case EQUMORE:
                cmpopcode = CmpInstruction::GE;
                break;
            case EQUAL:
                cmpopcode = CmpInstruction::E;
                break;
            case NOTEQUAL:
                cmpopcode = CmpInstruction::NE;
                break;
        }
        new CmpInstruction(cmpopcode, dst, src1, src2, bb);
        //
        BasicBlock *truebb, *falsebb, *tempbb;
        //临时假块
        truebb = new BasicBlock(func);
        falsebb = new BasicBlock(func);
        tempbb = new BasicBlock(func);

        true_list.push_back(new CondBrInstruction(truebb, tempbb, dst, bb));

        false_list.push_back(new UncondBrInstruction(falsebb, tempbb));
    }
    else if(op >= ADD && op <= SUB)
    {
        expr1->genCode();
        expr2->genCode();
        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();
        int opcode;
        switch (op)
        {
        case ADD:
            opcode = BinaryInstruction::ADD;
            break;
        case SUB:
            opcode = BinaryInstruction::SUB;
            break;
        }
        new BinaryInstruction(opcode, dst, src1, src2, bb);
    }
}

void Constant::genCode()
{
    // we don't need to generate code.
}

void Id::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
    new LoadInstruction(dst, addr, bb);
}

void IfStmt::genCode()
{
    Function *func;
    BasicBlock *then_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(end_bb);
}

void IfElseStmt::genCode()
{
    // Todo
}

void CompoundStmt::genCode()
{
    // Todo
}

void SeqNode::genCode()
{
    // Todo
}

void DeclStmt::genCode()
{
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymbolEntry());
    if(se->isGlobal())
    {
        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new IdentifierSymbolEntry(*se);
        addr_se->setType(new PointerType(se->getType()));
        addr = new Operand(addr_se);
        se->setAddr(addr);
    }
    else if(se->isLocal())
    {
        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Operand *addr;
        SymbolEntry *addr_se;
        Type *type;
        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new Operand(addr_se);
        alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.
        entry->insertFront(alloca);                                 // allocate instructions should be inserted into the begin of the entry block.
        se->setAddr(addr);                                          // set the addr operand in symbol entry so that we can use it in subsequent code generation.
    }
}

void ReturnStmt::genCode()
{
    // Todo
}

void AssignStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    expr->genCode();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(lval->getSymbolEntry())->getAddr();
    Operand *src = expr->getOperand();
    /***
     * We haven't implemented array yet, the lval can only be ID. So we just store the result of the `expr` to the addr of the id.
     * If you want to implement array, you have to caculate the address first and then store the result into it.
     */
    new StoreInstruction(addr, src, bb);
}

bool Ast::typeCheck(Type* retType)
{
    if(root != nullptr)
        root->typeCheck();
    return false;
}

bool FunctionDef::typeCheck(Type* retType)
{
    // Todo
    SymbolEntry* se = this->getSymbolEntry();
    Type* ret = ((FunctionType*)(se->getType()))->getRetType();
    StmtNode* stmt = this->stmt;
    if (stmt == nullptr) {
        if (ret != TypeSystem::voidType)
            fprintf(stderr, "non-void function does not return a value\n");
        return false;
    }
    if (!stmt->typeCheck(ret)) {
        fprintf(stderr, "function does not have a return statement\n");
        return false;
    }
    return false;
}

bool BinaryExpr::typeCheck(Type* retType)
{
    // Todo
    return false;
}

bool Constant::typeCheck(Type* retType)
{
    // Todo
    return false;
}

bool Id::typeCheck(Type* retType)
{
    // Todo
    return false;
}

bool IfStmt::typeCheck(Type* retType)
{
    // Todo
    if (thenStmt)   //if?????????ret
        return thenStmt->typeCheck(retType);
    return false;
}

bool IfElseStmt::typeCheck(Type* retType)
{
    // Todo
    bool temp1 = false, temp2 = false;
    if (thenStmt)
        temp1 = thenStmt->typeCheck(retType);
    if (elseStmt)
        temp2 = elseStmt->typeCheck(retType);
    return temp1 || temp2;
}

bool CompoundStmt::typeCheck(Type* retType)
{
    // Todo
    if (stmt)
        return stmt->typeCheck(retType);
    return false;
}

bool SeqNode::typeCheck(Type* retType)
{
    // Todo
    bool temp1 = false, temp2 = false;
    if (stmt1)
        temp1 = stmt1->typeCheck(retType);
    if (stmt2)
        temp2 = stmt2->typeCheck(retType);
    return temp1 || temp2;
}

bool DeclStmt::typeCheck(Type* retType)
{
    // Todo
    return false;
}

bool ReturnStmt::typeCheck(Type* retType)
{ 
    // Todo ???ExprNode *retValue??type??????type??????
    // retValue->getSymbolEntry()->getType()
    if (!retType) {
        fprintf(stderr, "[wrong]ret stmt not in function\n");
        return true;
    }
    if (!retValue && !retType->isVoid()) {
        fprintf(
            stderr,
            "return-statement with no value, but function return type is \'%s\'\n",
            retType->toStr().c_str());
        return true;
    }
    if (retValue && retType->isVoid()) {
        fprintf(
            stderr,
            "return-statement with a value, in function return type is \'void\'\n");
        return true;
    }
    if ((!retValue || !retValue->getSymbolEntry()))//? && retType->isVoid()
        return true;
    Type* type = retValue->getType();
    if (type != retType) {
        fprintf(stderr,
                "cannot initialize return object of type \'%s\' with an rvalue "
                "of type \'%s\'\n",
                retType->toStr().c_str(), type->toStr().c_str());//?????????
        fprintf(stderr,"fuctionret and ret value have different types\n");
        fprintf(stderr,"function ret type %s\n",retType->toStr().c_str());
        fprintf(stderr,"value type%s\n",type->toStr().c_str());
        //value???type??????
    }
    return true;
}

bool AssignStmt::typeCheck(Type* retType)
{
    // Todo
    return false;
}

bool ExprStmt::typeCheck(Type* retType)
{
    return false;
}

bool FunctionUse::typeCheck(Type* retType)
{
    return false;
}


void Ast::output()
{
    fprintf(yyout, "program\n");
    if(root != nullptr)
        root->output(4);
}

void ExprNode::output(int level)
{
    std::string name, type;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    fprintf(yyout, "%*cconst string\ttype:%s\t%s\n", level, ' ', type.c_str(), name.c_str());
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case ADD:
            op_str = "add";
            break;
        case SUB:
            op_str = "sub";
            break;
        case MUL:
            op_str = "mul";
            break;
        case DIV:
            op_str = "div";
            break;
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case LESS:
            op_str = "less";
            break;
        case MORE:
            op_str = "more";
            break;
        case MOD:
            op_str = "mod";
            break;
        case EQUMORE:
            op_str = "equmore";
            break;
        case EQULESS:
            op_str = "equless";
        case EQUAL:
            op_str = "equal"; 
        case NOTEQUAL:
            op_str = "notequal";   
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

void UnaryExpr::output(int level) 
{
    std::string op_str;
    switch (op) {
        case NOT:
            op_str = "not";
            break;
        case SUB:
            op_str = "minus";
            break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\t\n", level, ' ',
            op_str.c_str());
    expr->output(level + 4);
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    stmt->output(level + 4);
}

void SeqNode::output(int level)
{
    fprintf(yyout, "%*cSequence\n", level, ' ');
    stmt1->output(level + 4);
    stmt2->output(level + 4);
}

void ExprStmt::output(int level)
{
    fprintf(yyout, "%*cExprStmt\n", level, ' ');
    expr->output(level + 4);
}

void DeclStmt::output(int level)
{
    fprintf(yyout, "%*cDeclStmt\n", level, ' ');
    id->output(level + 4);
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);//cond?exprnode*?????????exprnode????????????
    thenStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void WhileStmt::output(int level) {
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    stmt->output(level + 4);
}

void BreakStmt::output(int level) {
    fprintf(yyout, "%*cBreakStmt\n", level, ' ');
}

void ContinueStmt::output(int level) {
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}
void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    if (retValue != nullptr)
        retValue->output(level + 4);
}

void BlankStmt::output(int level)
{
    fprintf(yyout, "%*cBlankStmt\n", level, ' ');
}
void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}

void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine\tfunction name: %s\ttype: %s\n", level,
            ' ', name.c_str(), type.c_str());
    if (decl) {
        decl->output(level + 4);
    }
    stmt->output(level + 4);
}


void FunctionUse::output(int level)
{
    std::string name, type;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    fprintf(yyout, "%*cFunctionUse function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
    if(expr!=nullptr)
    {expr->output(level + 4);}
}

void ImplictCastExpr::output(int level)
{
    fprintf(yyout, "%*cImplictCastExpr\ttype: %s to %s\n", level, ' ',
            expr->getType()->toStr().c_str(), type->toStr().c_str());
    this->expr->output(level + 4);
}

void ImplictCastExpr::genCode()
{
    expr->genCode();
    BasicBlock* bb = builder->getInsertBB();
    Function* func = bb->getParent();
    BasicBlock* trueBB = new BasicBlock(func);
    BasicBlock* tempbb = new BasicBlock(func);
    BasicBlock* falseBB = new BasicBlock(func);

    new CmpInstruction(
        CmpInstruction::NE, this->dst, this->expr->getOperand(),
        new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)), bb);
    this->trueList().push_back(
        new CondBrInstruction(trueBB, tempbb, this->dst, bb));
    this->falseList().push_back(new UncondBrInstruction(falseBB, tempbb));
}

BinaryExpr::BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2)
{
    //??????void???????????????
    if(expr1->getType()->isVoid()){
        fprintf(stderr, "void cannot participate in binary operations\n");

    }
    if(expr2->getType()->isVoid()){
        fprintf(stderr, "void cannot participate in binary operations\n");
    }
    dst = new Operand(se);
    //????cond?????????
    if (op >= BinaryExpr::AND && op <= BinaryExpr::NOTEQUAL)//???????
    {
        type = TypeSystem::boolType;
        if (op == BinaryExpr::AND || op == BinaryExpr::OR) {
            if (expr1->getType()->isInt() &&
                expr1->getType()->getSize() == 32) {
                ImplictCastExpr* temp = new ImplictCastExpr(expr1);
                this->expr1 = temp;
            }
            if (expr2->getType()->isInt() &&
                expr2->getType()->getSize() == 32) {
                ImplictCastExpr* temp = new ImplictCastExpr(expr2);
                this->expr2 = temp;
            }
            fprintf(stderr,"Implicit type conversion in AND/OR BinaryExpr\n");
        }
    } 
    else
        type = TypeSystem::intType;
};

IfStmt::IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt)
{
    if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ImplictCastExpr* temp = new ImplictCastExpr(cond);
            this->cond = temp;
            fprintf(stderr,"Implicit type conversion in Ifstmt's cond\n");
        }
};

WhileStmt::WhileStmt(ExprNode* cond, StmtNode* stmt): cond(cond), stmt(stmt)
{
    if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ImplictCastExpr* temp = new ImplictCastExpr(cond);
            this->cond = temp;
            fprintf(stderr,"Implicit type conversion in Whilestmt's cond\n");
        }
};

IfElseStmt::IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) 
{
    if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ImplictCastExpr* temp = new ImplictCastExpr(cond);
            this->cond = temp;
            fprintf(stderr,"Implicit type conversion in IfElsestmt's cond\n");
        }
};