#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

enum SYMBOL_TYPE{CONST_SYMBOL, VAR_SYMBOL};
typedef struct
{
    SYMBOL_TYPE type;
    int value;
    int addr;
    string ir_name;
} symbol_info_t;

class SymbolTable
{
private:
    int stack_depth;
    int id;
    int child_count;
    string name;
    SymbolTable *parent;
    SymbolTable *child;
    unordered_map<string, symbol_info_t*> symbol_table;
public:
    SymbolTable()
    {
        stack_depth = 0;
        id = 0;
        child_count = 0;
        name = "";
        parent = nullptr;
        child = nullptr;
    }
    SymbolTable(int depth, int id) : stack_depth(depth), id(id), child_count(0)
    {
    }
    inline bool Exist(string symbol);
    inline string Insert(string symbol, int value);
    inline string Insert(string symbol, string ir_name);
    inline symbol_info_t *LookUp(string symbol);
    inline SymbolTable *PopScope();
    inline SymbolTable *PushScope();
    ~SymbolTable()
    {
        for (auto &item: symbol_table)
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
    bool Exist(string symbol)
    {
        return current_symtab->Exist(symbol);
    }
    string Insert(string symbol, int value)
    {
        return current_symtab->Insert(symbol, value);
    }
    string Insert(string symbol, string ir_name)
    {
        return current_symtab->Insert(symbol, ir_name);
    }
    symbol_info_t *LookUp(string symbol)
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
inline unordered_map<string, string> func_map;

bool SymbolTable::Exist(string symbol)
{
    return (symbol_table.find(symbol) != symbol_table.end());
}

string SymbolTable::Insert(string symbol, int value)
{
    if (Exist(symbol))
    {
        return "";
    }
    symbol_info_t *info = new symbol_info_t;
    info->value = value;
    info->type = SYMBOL_TYPE::CONST_SYMBOL;
    this->symbol_table[symbol] = info;
    return "";
}

string SymbolTable::Insert(string symbol, string ir_name)
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

symbol_info_t *SymbolTable::LookUp(string symbol)
{
    if (Exist(symbol))
    {
        return symbol_table[symbol];
    }
    if (parent != nullptr)
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
    SymbolTable *sym_tab = new SymbolTable(stack_depth + 1, child_count++);
    sym_tab->name = name + "_" + to_string(sym_tab->id);
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
    cout << "decl @getint(): i32" << endl;
    cout << "decl @getch(): i32" << endl;
    cout << "decl @getarray(*i32): i32" << endl;
    cout << "decl @putint(i32)" << endl;
    cout << "decl @putch(i32)" << endl;
    cout << "decl @putarray(i32, *i32)" << endl;
    cout << "decl @starttime()" << endl;
    cout << "decl @stoptime()" << endl;
}