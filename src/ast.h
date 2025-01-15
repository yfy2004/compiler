#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <iostream>
#include <map>
#include <sstream>

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;

class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class MulExpAST;
class AddExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;

enum PrimaryExpType{EXP, NUMBER};
enum UnaryExpType{PRIMARY, UNARY};
enum BianryOPExpType{INHERIT, EXPAND};

static int symbol_count = 0;
static std::string get_IR(std::string str);
static std::string get_var(std::string str);
static std::map<std::string, std::string> names = {{"!=", "ne"}, {"==", "eq"}, {">", "gt"}, {"<", "lt"}, {">=", "ge"}, {"<=", "le"}, {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"}, {"&&", "and"}, {"||", "or"}};

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

class FuncTypeAST : public BaseAST
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

class BlockAST : public BaseAST
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

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "StmtAST {";
        exp->Dump();
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = exp->DumpIR();
        return get_IR(ans) + " ret " + get_var(ans);
    }
};

class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "ExpAST {";
        exp->Dump();
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        return exp->DumpIR();
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    PrimaryExpType type;
    int num;
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "PrimaryExpAST {";
        if (type == PrimaryExpType::EXP)
        {
            exp->Dump();
        }
        else if (type == PrimaryExpType::NUMBER)
        {
            std::cout << num;
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        if (type == PrimaryExpType::EXP)
        {
            return exp->DumpIR();
        }
        else if (type == PrimaryExpType::NUMBER)
        {
            return std::to_string(num);
        }
        else
        {
            return "";
        }
    }
};

class UnaryExpAST : public BaseAST
{
public:
    UnaryExpType type;
    std::unique_ptr<BaseAST> unary_exp;
    std::unique_ptr<BaseAST> primary_exp;
    std::string op;
    void Dump() const override
    {
        std::cout << "UnaryExpAST {";
        if (type == UnaryExpType::PRIMARY)
        {
            primary_exp->Dump();
        }
        else if (type == UnaryExpType::UNARY)
        {
            std::cout << op;
            unary_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == UnaryExpType::PRIMARY)
        {
            ans = primary_exp->DumpIR();
        }
        else if (type == UnaryExpType::UNARY)
        {
            std::string unary_IR = unary_exp->DumpIR();
            std::string IR = get_IR(unary_IR);
            std::string var = get_var(unary_IR);
            if (op == "+")
            {
                ans = unary_IR;
            }
            else if (op == "-")
            {
                ans = "%" + std::to_string(symbol_count) + "\n" + IR + "  %" + std::to_string(symbol_count) + " = sub 0, " + var + "\n";
                symbol_count++;
            }
            else if (op == "!")
            {
                ans = "%" + std::to_string(symbol_count) + "\n" + IR + "  %" + std::to_string(symbol_count) + " = eq " + var + ", 0\n";
                symbol_count++;
            }
        }
        return ans;
    }
};

class MulExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;
    std::string op;
    void Dump() const override
    {
        std::cout << "MulExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            unary_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            mul_exp->Dump();
            std::cout << " " << op << " ";
            unary_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = unary_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = mul_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = unary_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count++);
            ans = var + "\n" + lhs_IR + rhs_IR + "  " + var + " = " + names[op] + " " + lhs_var + ", " + rhs_var + "\n";
        }
        else
        {
            assert(false);
        }
        return ans;
    }
};

class AddExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    void Dump() const override
    {
        std::cout << "AddExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            mul_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            add_exp->Dump();
            std::cout << " " << op << " ";
            mul_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = mul_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = add_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = mul_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count++);
            ans = var + "\n" + lhs_IR + rhs_IR + "  " + var + " = " + names[op] + " " + lhs_var + ", " + rhs_var + "\n";
        }
        else
        {
            assert(false);
        }
        return ans;
    }
};

class RelExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    void Dump() const override
    {
        std::cout << "RelExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            add_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            rel_exp->Dump();
            std::cout << " " << op << " ";
            add_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = add_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = rel_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = add_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count++);
            ans = var + "\n" + lhs_IR + rhs_IR + "  " + var + " = " + names[op] + " " + lhs_var + ", " + rhs_var + "\n";
        }
        else
        {
            assert(false);
        }
        return ans;
    }
};

class EqExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    void Dump() const override
    {
        std::cout << "EqExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            rel_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            eq_exp->Dump();
            std::cout << " " << op << " ";
            rel_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = rel_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = eq_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = rel_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count++);
            ans = var + "\n" + lhs_IR + rhs_IR + "  " + var + " = " + names[op] + " " + lhs_var + ", " + rhs_var + "\n";
        }
        else
        {
            assert(false);
        }
        return ans;
    }
};
// LAndExp :: = EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;
    void Dump() const override
    {
        std::cout << "LAndExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            eq_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            land_exp->Dump();
            std::cout << " && ";
            eq_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = eq_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = land_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = eq_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count+2);
            ans = var + "\n" + lhs_IR + rhs_IR + "  %" + std::to_string(symbol_count) + " = ne " + lhs_var + ", 0\n" + "  %" + std::to_string(symbol_count + 1) + " = ne " + rhs_var + ", 0\n" + "  " + var + " = " + names["&&"] + " %" + std::to_string(symbol_count) + ", %" + std::to_string(symbol_count + 1) + "\n";
            symbol_count += 3;
        }
        else
            assert(false);
        return ans;
    }
};

class LOrExpAST : public BaseAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;
    void Dump() const override
    {
        std::cout << "LOrExpAST {";
        if (type == BianryOPExpType::INHERIT)
        {
            land_exp->Dump();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            lor_exp->Dump();
            std::cout << " || ";
            land_exp->Dump();
        }
        else
        {
            assert(false);
        }
        std::cout << "}";
    }
    std::string DumpIR() const override
    {
        std::string ans = "";
        if (type == BianryOPExpType::INHERIT)
        {
            ans = land_exp->DumpIR();
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            std::string lhs = lor_exp->DumpIR();
            std::string lhs_IR = get_IR(lhs);
            std::string lhs_var = get_var(lhs);
            std::string rhs = land_exp->DumpIR();
            std::string rhs_IR = get_IR(rhs);
            std::string rhs_var = get_var(rhs);
            std::string var = "%" + std::to_string(symbol_count + 2);
            ans = var + "\n" + lhs_IR + rhs_IR + "  %" + std::to_string(symbol_count) + " = ne " + lhs_var + ", 0\n" + "  %" + std::to_string(symbol_count + 1) + " = ne " + rhs_var + ", 0\n" + "  " + var + " = " + names["||"] + " %" + std::to_string(symbol_count) + ", %" + std::to_string(symbol_count + 1) + "\n";
            symbol_count += 3;
        }
        else
        {
            assert(false);
        }
        return ans;
    }
};

std::string get_IR(std::string str)
{
    std::istringstream iss(str);
    std::string t;
    std::getline(iss, t);
    std::string line;
    std::string IR;
    while (std::getline(iss, line))
    {
        IR += line + "\n";
    }
    return IR;
}

std::string get_var(std::string str)
{
    std::istringstream iss(str);
    std::string var;
    std::getline(iss, var);
    return var;
}