#include "SymbolTable.h"
#include "Type.h"
#include <iostream>
#include <sstream>

SymbolEntry::SymbolEntry(Type *type, int kind) 
{
    this->type = type;
    this->kind = kind;
}

bool SymbolEntry::setNext(SymbolEntry* se) {
    SymbolEntry* s = this;
    long unsigned int cnt =
        ((FunctionType*)(se->getType()))->getParamsType().size();
    if (cnt == ((FunctionType*)(s->getType()))->getParamsType().size())
    {
        std::vector<Type*> pv1=((FunctionType*)(se->getType()))->getParamsType();
        std::vector<Type*> pv2=((FunctionType*)(s->getType()))->getParamsType();
        int temp=0;
        for(int i=0;i<int(cnt);i++)
        {
            if(pv1[i]!=pv2[i])
                temp=1;
        }
        if(temp==0)
            {
                fprintf(stderr,"fuction overloading wrong,already have funcion %s\n",se->toStr().c_str());
                return false;//要依次判断参数类型，不要直接return
            }
    }
    while (s->getNext()) {
        if (cnt == ((FunctionType*)(s->getType()))->getParamsType().size())
        {
            std::vector<Type*> pv1=((FunctionType*)(se->getType()))->getParamsType();
            std::vector<Type*> pv2=((FunctionType*)(s->getType()))->getParamsType();
            int temp=0;
            for(int i=0;i<int(cnt);i++)
            {
                if(pv1[i]!=pv2[i])
                    temp=1;
            }
            if(temp==0)
            {
                fprintf(stderr,"fuction overloading wrong,already have funcion %s\n",se->toStr().c_str());
                return false;//要依次判断参数类型，不要直接return 重载
            }
        }
        s = s->getNext();
    }
    if (s == this) {
        this->next = se;
    } else {
        s->setNext(se);
    }
    return true;
}

ConstantSymbolEntry::ConstantSymbolEntry(Type *type, int value) : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}

std::string ConstantSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}

IdentifierSymbolEntry::IdentifierSymbolEntry(Type *type, std::string name, int scope) : SymbolEntry(type, SymbolEntry::VARIABLE), name(name)
{
    this->scope = scope;
    addr = nullptr;
}

std::string IdentifierSymbolEntry::toStr()
{
    return "@" + name;
}

TemporarySymbolEntry::TemporarySymbolEntry(Type *type, int label) : SymbolEntry(type, SymbolEntry::TEMPORARY)
{
    this->label = label;
}

std::string TemporarySymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << "%t" << label;
    return buffer.str();
}

SymbolTable::SymbolTable()
{
    prev = nullptr;
    level = 0;
}

SymbolTable::SymbolTable(SymbolTable *prev)
{
    this->prev = prev;
    this->level = prev->level + 1;
}

/*
    Description: lookup the symbol entry of an identifier in the symbol table
    Parameters: 
        name: identifier name
    Return: pointer to the symbol entry of the identifier

    hint:
    1. The symbol table is a stack. The top of the stack contains symbol entries in the current scope.
    2. Search the entry in the current symbol table at first.
    3. If it's not in the current table, search it in previous ones(along the 'prev' link).
    4. If you find the entry, return it.
    5. If you can't find it in all symbol tables, return nullptr.
*/
SymbolEntry* SymbolTable::lookup(std::string name)
{
    SymbolTable* temp_point = this;
    do {
        std::map<std::string, SymbolEntry*>::iterator it;
        it = temp_point->symbolTable.find(name);
        if (it != temp_point->symbolTable.end())
            return it->second;
        else
            temp_point = temp_point->prev;
    } while (temp_point != nullptr);
    return nullptr;
}

// install the entry into current symbol table.
bool SymbolTable::install(std::string name, SymbolEntry* entry) {
    if (this->symbolTable.find(name) != this->symbolTable.end()) {
        SymbolEntry* se = this->symbolTable[name];
        if (se->getType()->isFunc())
            return se->setNext(entry);
        if(!se->getType()->isFunc())
            fprintf(stderr, "identifier \"%s\" is redefined under one scope\n", name.c_str());        
        return false;
    } else {
        symbolTable[name] = entry;
        return true;
    }
}

int SymbolTable::counter = 0;
static SymbolTable t;
SymbolTable *identifiers = &t;
SymbolTable *globals = &t;
