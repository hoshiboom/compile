#ifndef __TYPE_H__
#define __TYPE_H__
#include <vector>
#include <string>
#include "SymbolTable.h"

class Type
{
private:
    int kind;
    
protected:
    enum {INT, VOID, FLOAT, FUNC, PTR, ARRAY, STRING };
public:
    int size;
    Type(int kind, int size=0) : kind(kind), size(size){};
    virtual ~Type() {};
    virtual std::string toStr() = 0;
    bool isInt() const {return kind == INT;};
    bool isVoid() const {return kind == VOID;};
    bool isFloat() const {return kind == FLOAT;};
    bool isFunc() const { return kind == FUNC; };
    bool isPtr() const { return kind == PTR; };
    bool isArray() const { return kind == ARRAY; };
    bool isString() const { return kind == STRING; };
    int getKind() const { return kind; };
    int getSize() const { return size; };
};

class IntType : public Type
{
private:
    bool constant;
public:
    IntType(int size, bool constant = false)//!
        : Type(Type::INT, size), constant(constant){};
    std::string toStr();
    bool isConst() const { return constant; };
};

class FloatType: public Type
{
public:
    FloatType() : Type(Type::FLOAT){};
    std::string toStr();
};

class VoidType : public Type
{
public:
    VoidType() : Type(Type::VOID){};
    std::string toStr();
};

class FunctionType : public Type
{
private:
    Type *returnType;
    std::vector<Type*> paramsType;
    std::vector<SymbolEntry*> paramsSe;
public:
    FunctionType(Type* returnType,
                 std::vector<Type*> paramsType,
                 std::vector<SymbolEntry*> paramsSe)
        : Type(Type::FUNC),
          returnType(returnType),
          paramsType(paramsType),
          paramsSe(paramsSe){};
    void setParamsType(std::vector<Type*> paramsType) {
        this->paramsType = paramsType;
    };
    std::vector<Type*> getParamsType() { return paramsType; };
    std::vector<SymbolEntry*> getParamsSe() { return paramsSe; };
    Type* getRetType() {return returnType;};
    std::string toStr();
};

class PointerType : public Type
{
private:
    Type *valueType;
public:
    PointerType(Type* valueType) : Type(Type::PTR) {this->valueType = valueType;};
    std::string toStr();
};

class TypeSystem
{
private:
    static IntType commonInt;
    static FloatType commonFloat;
    static IntType commonBool;
    static VoidType commonVoid;
public:
    static Type *intType;
    static Type *floatType;
    static Type *voidType;
    static Type *boolType;
};

#endif
