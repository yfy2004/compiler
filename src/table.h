#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

enum SYMBOL_TYPE{CONST_SYMBOL, VAR_SYMBOL};
typedef struct
{
    SYMBOL_TYPE type;
    int value;
    int addr;
    std::string ir_name;
} symbol_info_t;

class SymbolTable;
class SymbolTableStack;
class SymbolTable
{
private:
    int stack_depth;
    int id;
    int child_cnt;
    std::string name;
    SymbolTable *parent;
    SymbolTable *child;
    std::unordered_map<std::string,symbol_info_t*> symbol_table;
public:
    SymbolTable()
    {
        stack_depth = 0;
        id = 0;
        child_cnt = 0;
        name = "";
        parent = nullptr;
        child = nullptr;
    }
    SymbolTable(int depth, int id) : stack_depth(depth), id(id), child_cnt(0)
    {
    }
    inline bool Exist(std::string symbol);
    inline std::string Insert(std::string symbol, int value);
    inline std::string Insert(std::string symbol, std::string ir_name);
    inline symbol_info_t *LookUp(std::string symbol);
    inline SymbolTable *PopScope();
    inline SymbolTable *PushScope();
    ~SymbolTable()
    {
        for(auto &item: symbol_table)
        {
            delete item.second;
        }
        symbol_table.clear();
    }
};

class SymbolTableStack
{
private:
    SymbolTable *current_symtab;
public:
    SymbolTableStack()
    {
        current_symtab = new SymbolTable();
    }
    bool Exist(std::string symbol)
    {
        return current_symtab->Exist(symbol);
    }
    std::string Insert(std::string symbol, int value)
    {
        return current_symtab->Insert(symbol, value);
    }
    std::string Insert(std::string symbol, std::string ir_name)
    {
        return current_symtab->Insert(symbol, ir_name);
    }
    symbol_info_t *LookUp(std::string symbol)
    {
        return current_symtab->LookUp(symbol);
    }
    void PopScope()
    {
        SymbolTable *old = current_symtab;
        current_symtab = current_symtab->PopScope();
        delete old;
    }
    void PushScope()
    {
        current_symtab = current_symtab->PushScope();
    }
};

inline SymbolTableStack symbol_table_stack;
inline std::unordered_map<std::string,std::string> func_map;

bool SymbolTable::Exist(std::string symbol)
{
    return (symbol_table.find(symbol) != symbol_table.end());
}

std::string SymbolTable::Insert(std::string symbol, int value)
{
    if (Exist(symbol))
    {
        return "";
    }
    symbol_info_t* info = new symbol_info_t;
    info->value = value;
    info->type = SYMBOL_TYPE::CONST_SYMBOL;
    this->symbol_table[symbol] = info;
    return "";
}

std::string SymbolTable::Insert(std::string symbol, std::string ir_name)
{
    if (Exist(symbol))
    {
        return "";
    }
    symbol_info_t *info = new symbol_info_t;
    info->type = SYMBOL_TYPE::VAR_SYMBOL;
    info->ir_name = ir_name + name;
    this->symbol_table[symbol] = info;
    return info->ir_name;
}

symbol_info_t *SymbolTable::LookUp(std::string symbol)
{
    if (Exist(symbol))
    {
        return symbol_table[symbol];
    }
    if (parent!=nullptr)
    {
        return parent->LookUp(symbol);
    }
    else
    {
        return nullptr;
    }
}

SymbolTable *SymbolTable::PushScope()
{
    SymbolTable *sym_tab = new SymbolTable(stack_depth + 1, child_cnt++);
    sym_tab->name = name + "_" + std::to_string(sym_tab->id);
    sym_tab->parent = this;
    sym_tab->child = nullptr;
    this->child = sym_tab;
    return sym_tab;
}
SymbolTable *SymbolTable::PopScope()
{
    SymbolTable *sym_tab = this->parent;
    return sym_tab;
}

inline void initSysyRuntimeLib()
{
    func_map["getint"] = "i32";
    func_map["getch"] = "i32";
    func_map["getarray"] = "i32";
    func_map["putint"] = "void";
    func_map["putch"] = "void";
    func_map["putarray"] = "void";
    func_map["starttime"] = "void";
    func_map["stoptime"] = "void";
    std::cout << "decl @getint(): i32" << std::endl;
    std::cout << "decl @getch(): i32" << std::endl;
    std::cout << "decl @getarray(*i32): i32" << std::endl;
    std::cout << "decl @putint(i32)" << std::endl;
    std::cout << "decl @putch(i32)" << std::endl;
    std::cout << "decl @putarray(i32, *i32)" << std::endl;
    std::cout << "decl @starttime()" << std::endl;
    std::cout << "decl @stoptime()" << std::endl;
}