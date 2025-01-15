#pragma once
#include <memory>
#include <string>
#include <iostream>

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;
class NumberAST;

class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;
    void Dump() const override
    {
        std::cout << "CompUnitAST {";
        func_def->Dump();
        std::cout << "}\n";
    }
    std::string DumpIR() const override
    { 
        return func_def->DumpIR();
    }
};

class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void Dump() const override
    {
        std::cout << "FuncDefAST {";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return "fun @" + ident+ "(): " + func_type->DumpIR() + " {\n" + block->DumpIR() + "\n}\n";
    }
};

class FuncTypeAST:public BaseAST
{
public:
    std::string type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST {";
        std::cout << type;
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return "i32";
    }
};

class BlockAST: public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override
    {
        std::cout << "BlockAST {";
        stmt->Dump();
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return "%entry:\n  " + stmt->DumpIR() + "\n";
    }
};

class StmtAST: public BaseAST
{
public:
    std::unique_ptr<BaseAST> number;
    void Dump() const override
    {
        std::cout << "StmtAST {";
        number->Dump();
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return "ret " + number->DumpIR();
    }
};

class NumberAST: public BaseAST
{
public:
    int const_int;
    void Dump() const override
    {
        std::cout << "NumberAST {";
        std::cout << const_int;
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return std::to_string(const_int);
    }
};
