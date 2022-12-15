#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include "Type.h"
#include "Operand.h"
#include <stack>
class SymbolEntry;
class Unit;
class Function;
class BasicBlock;
class Instruction;
class IRBuilder;

class Node
{
private:
    static int counter;
    Node* next;//??????????????????????????????????
    int seq;
protected:
    std::vector<Instruction*> true_list;
    std::vector<Instruction*> false_list;
    static IRBuilder *builder;
    void backPatch(std::vector<Instruction*> &list, BasicBlock*bb);
    std::vector<Instruction*> merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2);

public:
    Node();
    int getSeq() const {return seq;};
    void setNext(Node* node);
    Node* getNext() { return next; }
    static void setIRBuilder(IRBuilder*ib) {builder = ib;};
    virtual void output(int level) = 0;
    virtual void typeCheck() = 0 ;// =0?
    virtual void genCode() = 0;
    std::vector<Instruction*>& trueList() {return true_list;}
    std::vector<Instruction*>& falseList() {return false_list;}
};

class ExprNode : public Node
{
protected:
    SymbolEntry *symbolEntry;
    Operand *dst;   // The result of the subtree is stored into dst.
    Type* type;
public:
    ExprNode(SymbolEntry *symbolEntry) : symbolEntry(symbolEntry){};
    Operand* getOperand() {return dst;};
    //SymbolEntry* getSymPtr() {return symbolEntry;};
    void output(int level);//???
    virtual Type* getType() {return type;};
    virtual int getValue() {return -1;};
    SymbolEntry* getSymbolEntry() { return symbolEntry; };
};

class BinaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1, *expr2;
public:
    enum {ADD, SUB, MUL, DIV, AND, OR, LESS, MORE, MOD, EQUMORE, EQULESS, EQUAL, NOTEQUAL};
    BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2){};
    void output(int level);
    void genCode();
    void typeCheck();
};

class UnaryExpr : public ExprNode {
private:
    int op;
    ExprNode* expr;

public:
    enum { NOT, SUB };
    UnaryExpr(SymbolEntry* se, int op_, ExprNode* e): ExprNode(se), op(op_), expr(e){
        if(op==UnaryExpr::NOT){
            type = TypeSystem::intType;
            UnaryExpr* ue = (UnaryExpr*)expr;
            if (ue->getOp() == UnaryExpr::NOT) {
                if (ue->getType() == TypeSystem::intType)
                    ue->setType(TypeSystem::boolType);
                // type = TypeSystem::intType;
            }
        }
        else if(op==UnaryExpr::SUB){
            type = TypeSystem::intType;
            
            UnaryExpr* ue = (UnaryExpr*)expr;
            if (ue->getOp() == UnaryExpr::NOT)
                if (ue->getType() == TypeSystem::intType)
                    ue->setType(TypeSystem::boolType);
        }

    };
    void genCode(){};
    void typeCheck(){};
    void output(int level);
    int getOp() const { return op; };
    void setType(Type* type) { this->type = type; }
};

class FunctionUse : public ExprNode 
{
private:
    ExprNode* expr;
public:
    FunctionUse(SymbolEntry *se,ExprNode* expr=nullptr ) : ExprNode(se), expr(expr){};
    void output(int level);
    void typeCheck(){};
    void genCode(){};
};

class Constant : public ExprNode
{
public:
    Constant(SymbolEntry *se) : ExprNode(se){dst = new Operand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class Id : public ExprNode
{
public:
    Id(SymbolEntry *se) : ExprNode(se){SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel()); dst = new Operand(temp);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class StmtNode : public Node
{
private:
    int kind;
protected:
    enum{IF,IFELSE,WHILE,COMPOUND,RETURN};

};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;
public:
    CompoundStmt(StmtNode *stmt) : stmt(stmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class SeqNode : public StmtNode
{
private:
    StmtNode *stmt1, *stmt2;
public:
    SeqNode(StmtNode *stmt1, StmtNode *stmt2) : stmt1(stmt1), stmt2(stmt2){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class DeclStmt : public StmtNode
{
private:
    Id *id;
    ExprNode* expr;
public:
    DeclStmt(Id *id, ExprNode* expr = nullptr) : id(id){
        if (expr) {
            this->expr = expr;            
        }
    };
    void output(int level);
    Id* getId() { return id; };
    void typeCheck();
    void genCode();
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfElseStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
    StmtNode *elseStmt;
public:
    IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};
class WhileStmt : public StmtNode//?????????exp-cond??stmt-stmt
{
private:
    ExprNode* cond;
    StmtNode* stmt;
public:
    WhileStmt(ExprNode* cond, StmtNode* stmt=nullptr): cond(cond), stmt(stmt){}
    void output(int level);
    void setStmt(StmtNode* s){this->stmt=s;};//while??stmt???§á????????????????????????while????????setStmt
};

class BreakStmt : public StmtNode
{//break????????while?????????§à??????StmtNode* whileStmt???????????????????????????
private:
    StmtNode* whilestmt;
public:
    BreakStmt(StmtNode* ws){this->whilestmt=ws;};
    void output(int level);
};

class ContinueStmt : public StmtNode
{
private:
    StmtNode* whilestmt;
public:
    ContinueStmt(StmtNode* ws){this->whilestmt=ws;};
    void output(int level);
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
public:
    ReturnStmt(ExprNode*retValue = nullptr) : retValue(retValue) {};
    void output(int level);
    void typeCheck();
    void genCode();
};
class BlankStmt : public StmtNode
{
public:
    BlankStmt(){};
    void output(int level);
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    ExprNode *expr;
public:
    AssignStmt(ExprNode *lval, ExprNode *expr) : lval(lval), expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ExprStmt : public StmtNode {
   private:
    ExprNode* expr;

   public:
    ExprStmt(ExprNode* expr) : expr(expr){};
    void output(int level);
    bool typeCheck(Type* retType = nullptr);
    void genCode();
};

class FunctionDef : public StmtNode {
   private:
    SymbolEntry* se;
    DeclStmt* decl;
    StmtNode* stmt;

   public:
    FunctionDef(SymbolEntry* se, DeclStmt* decl, StmtNode* stmt)
        : se(se), decl(decl), stmt(stmt){};
    void output(int level);
    //void typeCheck(Type* retType = nullptr);
    void typeCheck();
    void genCode();
    SymbolEntry* getSymbolEntry() { return se; };
};

class Ast
{
private:
    Node* root;
public:
    Ast() {root = nullptr;}
    void setRoot(Node*n) {root = n;}
    void output();
    void typeCheck();
    void genCode(Unit *unit);
};

#endif
