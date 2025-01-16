#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include "table.h"

class BaseAST;
class VecAST;
class ExpVecAST;
class BaseExpAST;
class CompUnitAST;
class FuncDefAST;
class TypeAST;
class BlockAST;
class StmtAST;
class OpenStmtAST;
class ClosedStmtAST;
class SimpleStmtAST;

class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class MulExpAST;
class AddExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;

class DeclAST;
class ConstDeclAST;
class ConstDefAST;
class ConstInitValAST;
class BlockItemAST;
class LValAST;
class ConstExpAST;
class VarDeclAST;
class VarDefAST;
class InitVal;
class FuncFParamAST;

enum PrimaryExpType{EXP, NUMBER, LVAL};
enum UnaryExpType{PRIMARY, UNARY, CALL};
enum BianryOPExpType{INHERIT, EXPAND};
enum DeclType{CONST_DECL, VAR_DECL};
enum VarDefType{VAR, VAR_ASSIGN};
enum BlockItemType{BLK_DECL, BLK_STMT};
enum StmtType{STMT_OPEN, STMT_CLOSED};
enum OpenStmtType{OSTMT_CLOSED, OSTMT_OPEN, OSTMT_ELSE, OSTMT_WHILE};
enum ClosedStmtType{CSTMT_SIMPLE, CSTMT_ELSE, CSTMT_WHILE};
enum SimpleStmtType{SSTMT_ASSIGN, SSTMT_EMPTY_RET, SSTMT_RETURN, SSTMT_EMPTY_EXP, SSTMT_EXP, SSTMT_BLK, SSTMT_BREAK, SSTMT_CONTINUE};

static int symbol_count = 0;
static int label_count = 0;
static std::map<std::string, std::string> names = {{"!=", "ne"}, {"==", "eq"}, {">", "gt"}, {"<", "lt"}, {">=", "ge"}, {"<=", "le"}, {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"}, {"&&", "and"}, {"||", "or"}};
static bool is_ret = false;
static int alloc_tmp = 0;
static std::vector<int> while_stack;
static std::string current_func;

class BaseAST
{
public:
    std::string ident = "";
    bool is_global = false;
    virtual void DumpIR() = 0;
    virtual ~BaseAST() = default;
};

class VecAST
{
public:
    std::vector<std::unique_ptr<BaseAST> > vec;
    void push_back(std::unique_ptr<BaseAST> &ast)
    {
        vec.push_back(std::move(ast));
    }
};

class ExpVecAST
{
public:
    std::vector<std::unique_ptr<BaseExpAST> > vec;
    void push_back(std::unique_ptr<BaseExpAST> &ast)
    {
        vec.push_back(std::move(ast));
    }
};

class BaseExpAST: public BaseAST
{
public:
    virtual void Eval() = 0;
    int value = -1;
    bool is_const = false;
    bool is_evaled = false;
    bool is_left = false;
    void Copy(std::unique_ptr<BaseExpAST> &exp)
    {
        value = exp->value;
        ident = exp->ident;
        is_const = exp->is_const;
        is_evaled = exp->is_evaled;
    }
};

class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<VecAST> comp_units;
    void DumpIR() override
    { 
        symbol_table_stack.PushScope();
        initSysyRuntimeLib();
        for (auto &item:comp_units->vec)
        {
            item->is_global = true;
            item->DumpIR();
        }
        symbol_table_stack.PopScope();
    }
};

class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<VecAST> func_fparams;
    void DumpIR() override
    {
        current_func = ident;
        func_type->DumpIR();
        symbol_count = 0;
        assert(func_map.find(ident) == func_map.end());
        func_map[ident] = func_type->ident;
        symbol_table_stack.PushScope();
        std::vector<std::string> params;
        std::cout << "fun @" << ident << "(";
        int count = 0;
        for (auto &param: func_fparams->vec)
        {
            if (count != 0)
            {
                std::cout << ", ";
            }
            param->DumpIR();
            params.push_back(param->ident);
            count++;
        }
        std::cout << ")";
        if (func_type->ident == "i32")
        {
            std::cout << ": " << func_type->ident;
        }
        std::cout << " {" << std::endl;
        std::cout << "%entry_" << ident << ":" << std::endl;
        for (auto &param: params)
        {
            symbol_table_stack.Insert(param, "%" + param);
            symbol_info_t *info = symbol_table_stack.LookUp(param);
            std::cout << "  " << info->ir_name << " = alloc i32" << std::endl;
            std::cout << "  store @" << param << ", " << info->ir_name << std::endl;
        }
        block->DumpIR();
        if (is_ret == false)
        {
            if (func_type->ident == "i32")
            {
                std::cout << "  ret 0" << std::endl;
            }
            else if (func_type->ident == "void")
                std::cout << "  ret" << std::endl;
        }
        std::cout << "}" << std::endl;
        symbol_table_stack.PopScope();
        is_ret = false;
    }
};

class TypeAST : public BaseAST
{
public:
    std::string type;
    void DumpIR() override
    {   
        if (type == "int")
        {
            ident = "i32";
        }
        else if (type == "void")
        {
            ident="void";
        }
        else
        {
            assert(false);
        }
    }
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<VecAST> items;
    void DumpIR() override
    {
        for (auto &item:items->vec)
        {  
            if (is_ret == true)
            {
               break;
            }
            item->DumpIR();
        }
    }
};

class StmtAST : public BaseAST
{
public:
    StmtType type;
    std::unique_ptr<BaseAST> open_stmt;
    std::unique_ptr<BaseAST> closed_stmt;
    void DumpIR() override
    {
        if (type == StmtType::STMT_CLOSED)
        {
            closed_stmt->DumpIR();
        }
        else if (type == StmtType::STMT_OPEN)
        {
            open_stmt->DumpIR();
        }
        else
        {
            assert(false);
        }
    }
};

class OpenStmtAST: public BaseAST
{
public:
    OpenStmtType type;
    std::unique_ptr<BaseExpAST> exp;
    std::unique_ptr<BaseAST> open_stmt;
    std::unique_ptr<BaseAST> closed_stmt;
    void DumpIR() override
    {
        std::string label_then = "%then_" + std::to_string(label_count);
        std::string label_else = "%else_" + std::to_string(label_count);
        std::string label_end = "%end_" + std::to_string(label_count);
        std::string label_while_entry = "%while_entry_" + std::to_string(label_count);
        std::string label_while_body = "%while_body_" + std::to_string(label_count++);
        if (type == OpenStmtType::OSTMT_CLOSED)
        {
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_then << ", " << label_end << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            is_ret = false;
            closed_stmt->DumpIR();
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_end << ":" << std::endl;
            is_ret = false;
        }
        else if (type == OpenStmtType::OSTMT_OPEN)
        {
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_then << ", " << label_end << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            is_ret = false;
            open_stmt->DumpIR();
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_end << ":" << std::endl;
            is_ret = false;
        }
        else if (type == OpenStmtType::OSTMT_ELSE)
        {
            bool total_ret = true;
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_then << ", " << label_else << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            is_ret = false;
            closed_stmt->DumpIR();
            total_ret = total_ret & is_ret;
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_else << ":" << std::endl;
            is_ret = false;
            open_stmt->DumpIR();
            total_ret = total_ret & is_ret;
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            if (total_ret == false)
            {
                std::cout << label_end << ":" << std::endl;
            }
            is_ret = total_ret;
        }
        else if (type == OpenStmtType::OSTMT_WHILE)
        {
            while_stack.push_back(label_count - 1);
            std::cout << "  jump " << label_while_entry << std::endl << std::endl;
            std::cout << label_while_entry << ":" << std::endl;
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_while_body << ", " << label_end << std::endl << std::endl;
            std::cout << label_while_body << ":" << std::endl;
            is_ret = false;
            open_stmt->DumpIR();
            if (is_ret == false)
            {
                std::cout << "  jump " << label_while_entry << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_end << ":" << std::endl;
            is_ret = false;
            while_stack.pop_back();
        }
        else
        {
            assert(false);
        }
    }
};

class ClosedStmtAST: public BaseAST
{
public:
    ClosedStmtType type;
    std::unique_ptr<BaseExpAST> exp;
    std::unique_ptr<BaseAST> closed_stmt1;
    std::unique_ptr<BaseAST> closed_stmt2;
    std::unique_ptr<BaseAST> simple_stmt;
    void DumpIR() override
    {
        if (type == ClosedStmtType::CSTMT_SIMPLE)
        {
            simple_stmt->DumpIR();
        }
        else if (type == ClosedStmtType::CSTMT_ELSE)
        {
            std::string label_then = "%then_" + std::to_string(label_count);
            std::string label_else = "%else_" + std::to_string(label_count);
            std::string label_end = "%end_" + std::to_string(label_count++);
            bool total_ret = true;
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_then << ", " << label_else << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            is_ret = false;
            closed_stmt1->DumpIR();
            total_ret = total_ret & is_ret;
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_else << ":" << std::endl;
            is_ret = false;
            closed_stmt2->DumpIR();
            total_ret = total_ret & is_ret;
            if (is_ret == false)
            {
                std::cout << "  jump " << label_end << std::endl;
            }
            std::cout << std::endl;
            if (total_ret == false)
            {
                std::cout << label_end << ":" << std::endl;
            }
            is_ret = total_ret;
        }
        else if (type == ClosedStmtType::CSTMT_WHILE)
        {
            std::string label_end = "%end_" + std::to_string(label_count);
            std::string label_while_entry = "%while_entry_" + std::to_string(label_count);
            std::string label_while_body = "%while_body_" + std::to_string(label_count);
            while_stack.push_back(label_count++);
            std::cout << "  jump " << label_while_entry << std::endl << std::endl;
            std::cout << label_while_entry << ":" << std::endl;
            exp->Eval();
            std::cout << "  br " << exp->ident << ", " << label_while_body << ", " << label_end << std::endl << std::endl;
            std::cout << label_while_body << ":" << std::endl;
            is_ret = false;
            closed_stmt1->DumpIR();
            if (is_ret == false)
            {
                std::cout << "  jump " << label_while_entry << std::endl;
            }
            std::cout << std::endl;
            std::cout << label_end << ":" << std::endl;
            is_ret = false;
            while_stack.pop_back();
        }
        else
        {
            assert(false);
        }
    }
};

class SimpleStmtAST : public BaseAST
{
public:
    SimpleStmtType type;
    std::unique_ptr<BaseExpAST> lval;
    std::unique_ptr<BaseExpAST> exp;
    std::unique_ptr<BaseAST> block;
    void DumpIR() override
    {
        if (type == SimpleStmtType::SSTMT_RETURN)
        {
            exp->Eval();
            std::cout << "  ret " << exp->ident << std::endl;
            is_ret = true;
        }
        else if (type == SimpleStmtType::SSTMT_EMPTY_RET)
        {
            std::string ret_type = func_map[current_func];
            if (ret_type == "i32")
            {
                std::cout << "  ret 0" << std::endl;
            }
            else
            {
                std::cout << "  ret" << std::endl;
            }
            is_ret = true;
        }
        else if (type == SimpleStmtType::SSTMT_ASSIGN)
        {
            exp->Eval();
            lval->is_left = true;
            lval->Eval();
            assert(!lval->is_const);
            exp->DumpIR();
            symbol_info_t *info = symbol_table_stack.LookUp(lval->ident);
            std::cout << "  store " << exp->ident << ", " << info->ir_name << std::endl;
        }
        else if (type == SimpleStmtType::SSTMT_BLK)
        {
            symbol_table_stack.PushScope();
            block->DumpIR();
            symbol_table_stack.PopScope();
        }
        else if (type == SimpleStmtType::SSTMT_EMPTY_EXP)
        {
        }
        else if (type == SimpleStmtType::SSTMT_EXP)
        {
            exp->Eval();
            exp->DumpIR();
        }
        else if (type == SimpleStmtType::SSTMT_BREAK)
        {
            assert(!while_stack.empty());
            int label_num = while_stack.back();
            std::cout << "  jump %end_" << std::to_string(label_num) << std::endl << std::endl;
            is_ret = true;
        }
        else if (type == SimpleStmtType::SSTMT_CONTINUE)
        {
            assert(!while_stack.empty());
            int label_num = while_stack.back();
            std::cout << "  jump %while_entry_" << std::to_string(label_num) << std::endl << std::endl;
            is_ret = true;
        }
        else
        {
            assert(false);
        }
    }
};

class ExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseExpAST> exp;
    void DumpIR() override
    {
        exp->DumpIR();
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        exp->Eval();
        Copy(exp);
        is_evaled = true;
    }
};

class PrimaryExpAST : public BaseExpAST
{
public:
    PrimaryExpType type;
    int num;
    std::unique_ptr<BaseExpAST> exp;
    std::unique_ptr<BaseExpAST> lval;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == PrimaryExpType::EXP)
        {
            exp->Eval();
            Copy(exp);
        }
        else if (type == PrimaryExpType::NUMBER)
        {
            value = num;
            is_const = true;
            ident = std::to_string(value);
        }
        else if (type == PrimaryExpType::LVAL)
        {
            lval->Eval();
            Copy(lval);
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class UnaryExpAST : public BaseExpAST
{
public:
    UnaryExpType type;
    std::unique_ptr<BaseExpAST> unary_exp;
    std::unique_ptr<BaseExpAST> primary_exp;
    std::unique_ptr<ExpVecAST> func_rparams;
    std::string op;
    std::string func_name;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == UnaryExpType::PRIMARY)
        {
            primary_exp->Eval();
            Copy(primary_exp);
        }
        else if (type == UnaryExpType::UNARY)
        {
            unary_exp->Eval();
            Copy(unary_exp);
            if (unary_exp->is_const)
            {
                if (op == "+")
                {
                    value = unary_exp->value;
                }
                else if (op == "-")
                {
                    value = -unary_exp->value;
                }
                else if (op == "!")
                {
                    value = !unary_exp->value;
                }
                else
                {
                    assert(false);
                }
                ident = std::to_string(value);
            }
            else
            {   
                if (op == "-")
                {
                    ident = "%" + std::to_string(symbol_count++);
                    std::cout << "  " << ident << " = sub 0, " << unary_exp->ident << std::endl;
                }
                else if (op == "!")
                {
                    ident = "%" + std::to_string(symbol_count++);
                    std::cout << "  " << ident << " = eq " << unary_exp->ident << ", 0" << std::endl;
                }
            }
            is_evaled = true;
        }
        else if (type == UnaryExpType::CALL)
        {
            for (auto &param: func_rparams->vec)
            {
                param->Eval();
            }
            assert(func_map.find(func_name) != func_map.end());
            std::string ret_type = func_map[func_name];
            if (ret_type == "i32")
            {
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = ";
            }
            else if (ret_type == "void")
            {
                std::cout << "  ";
            }
            else
            {
                assert(false);
            }
            std::cout << "call @" << func_name << "(";
            int count = 0;
            for (auto &param : func_rparams->vec)
            {
                if (count != 0)
                {
                    std::cout << ", ";
                }
                std::cout << param->ident;
                count++;
            }
            std::cout << ")" << std::endl;
        }
    }
};

class MulExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> mul_exp;
    std::unique_ptr<BaseExpAST> unary_exp;
    std::string op;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            unary_exp->Eval();
            Copy(unary_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            mul_exp->Eval();
            unary_exp->Eval();
            is_const = mul_exp->is_const && unary_exp->is_const;
            if (is_const)
            {
                int val1 = mul_exp->value;
                int val2 = unary_exp->value;
                if (op == "*")
                {
                    value = val1 * val2;
                }
                else if (op == "/")
                {
                    value = val1 / val2;
                }
                else if (op == "%")
                {
                    value = val1 % val2;
                }
                else
                {
                    assert(false);
                }
                ident = std::to_string(value);
            }
            else
            {
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = " << names[op] << " " << mul_exp->ident << ", " << unary_exp->ident << std::endl;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class AddExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> add_exp;
    std::unique_ptr<BaseExpAST> mul_exp;
    std::string op;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            mul_exp->Eval();
            Copy(mul_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            add_exp->Eval();
            mul_exp->Eval();
            is_const = add_exp->is_const && mul_exp->is_const;
            if (is_const)
            {
                int val1 = add_exp->value;
                int val2 = mul_exp->value;
                if (op == "+")
                {
                    value = val1 + val2;
                }
                else if (op == "-")
                {
                    value = val1 - val2;
                }
                else
                {
                    assert(false);
                }
                ident = std::to_string(value);
            }
            else
            {
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = " << names[op] << " " << add_exp->ident << ", " << mul_exp->ident << std::endl;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class RelExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> rel_exp;
    std::unique_ptr<BaseExpAST> add_exp;
    std::string op;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            add_exp->Eval();
            Copy(add_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            rel_exp->Eval();
            add_exp->Eval();
            is_const = rel_exp->is_const && add_exp->is_const;
            if (is_const)
            {
                int val1 = rel_exp->value;
                int val2 = add_exp->value;
                if (op == "<")
                {
                    value = (val1 < val2);
                }
                else if (op == ">")
                {
                    value = (val1 > val2);
                }
                else if (op == "<=")
                {
                    value = (val1 <= val2);
                }
                else if (op == ">=")
                {
                    value = (val1 >= val2);
                }
                else
                {
                    assert(false);
                }
                ident = std::to_string(value);
            }
            else{
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = " << names[op] << " " << rel_exp->ident << ", " << add_exp->ident << std::endl;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class EqExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> eq_exp;
    std::unique_ptr<BaseExpAST> rel_exp;
    std::string op;
    void DumpIR()override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            rel_exp->Eval();
            Copy(rel_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            eq_exp->Eval();
            rel_exp->Eval();
            is_const = eq_exp->is_const && rel_exp->is_const;
            if (is_const)
            {
                int val1 = eq_exp->value;
                int val2 = rel_exp->value;
                if (op == "==")
                {
                    value = (val1 == val2);
                }
                else if (op == "!=")
                {
                    value = (val1 != val2);
                }
                else
                {
                    assert(false);
                }
                ident = std::to_string(value);
            }
            else
            {
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = " << names[op] << " " << eq_exp->ident << ", " << rel_exp->ident << std::endl;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class LAndExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> land_exp;
    std::unique_ptr<BaseExpAST> eq_exp;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            eq_exp->Eval();
            Copy(eq_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            land_exp->Eval();
            if (land_exp->is_const && land_exp->value == 0)
            {
                value = land_exp->value;
                ident = std::to_string(value);
                is_const = true;
                is_evaled = true;
                return;
            }
            std::string label_then = "%then_" + std::to_string(label_count);
            std::string label_else = "%else_" + std::to_string(label_count);
            std::string label_end = "%end_" + std::to_string(label_count++);
            ident = "t" + std::to_string(alloc_tmp++);
            std::string ir_name = "@" + ident;
            symbol_table_stack.Insert(ident, ir_name);
            std::cout << "  " << ir_name << " = alloc i32" << std::endl;
            std::string tmp_var1 = "%"+std::to_string(symbol_count++);
            std::cout << "  " << tmp_var1 << " = ne " << land_exp->ident << ", 0" << std::endl;
            std::cout << "  br " << tmp_var1 << ", " << label_then << ", " << label_else << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            eq_exp->Eval();
            std::string tmp_var2 = "%" + std::to_string(symbol_count++);
            std::cout << "  " << tmp_var2 << " = ne " << eq_exp->ident << ", 0" << std::endl;
            std::cout << "  store " << tmp_var2 << ", " << ir_name << std::endl;
            std::cout << "  jump " << label_end << std::endl << std::endl;
            std::cout << label_else << ":" << std::endl;
            std::cout << "  store 0, " << ir_name << std::endl;
            std::cout << "  jump " << label_end << std::endl << std::endl;
            std::cout << label_end << ":" << std::endl;
            ident = "%" + std::to_string(symbol_count++);
            std::cout << "  " << ident << " = load " << ir_name << std::endl;
            if (land_exp->is_const && eq_exp->is_const)
            {
                value = land_exp->value && eq_exp->value;
                ident = std::to_string(value);
                is_const = true;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class LOrExpAST : public BaseExpAST
{
public:
    BianryOPExpType type;
    std::unique_ptr<BaseExpAST> lor_exp;
    std::unique_ptr<BaseExpAST> land_exp;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        if (type == BianryOPExpType::INHERIT)
        {
            land_exp->Eval();
            Copy(land_exp);
        }
        else if (type == BianryOPExpType::EXPAND)
        {
            lor_exp->Eval();
            if (lor_exp->is_const && lor_exp->value == 1)
            {
                value = lor_exp->value;
                ident = std::to_string(value);
                is_const = true;
                is_evaled = true;
                return;
            }
            std::string label_then = "%then_" + std::to_string(label_count);
            std::string label_else = "%else_" + std::to_string(label_count);
            std::string label_end = "%end_" + std::to_string(label_count++);
            std::string ir_name = "@t" + std::to_string(alloc_tmp++);
            symbol_table_stack.Insert(ident, ir_name);
            std::cout << "  " << ir_name << " = alloc i32" << std::endl;
            std::string tmp_var1 = "%" + std::to_string(symbol_count++);
            std::cout << "  " << tmp_var1 << " = eq " << lor_exp->ident << ", 0" << std::endl;
            std::cout << "  br " << tmp_var1 << ", " << label_then << ", " << label_else << std::endl << std::endl;
            std::cout << label_then << ":" << std::endl;
            land_exp->Eval();
            std::string tmp_var2 = "%" + std::to_string(symbol_count++);
            std::cout << "  " << tmp_var2 << " = ne " << land_exp->ident << ", 0" << std::endl;
            std::cout << "  store " << tmp_var2 << ", " << ir_name << std::endl;
            std::cout << "  jump " << label_end << std::endl << std::endl;
            std::cout << label_else << ":" << std::endl;
            std::cout << "  store 1, " << ir_name << std::endl;
            std::cout << "  jump " << label_end << std::endl << std::endl;
            std::cout << label_end << ":" << std::endl;
            ident = "%" + std::to_string(symbol_count++);
            std::cout << "  " << ident << " = load " << ir_name << std::endl;
            if (land_exp->is_const && lor_exp->is_const)
            {
                value = land_exp->value || lor_exp->value;
                ident = std::to_string(value);
                is_const = true;
            }
        }
        else
        {
            assert(false);
        }
        is_evaled = true;
    }
};

class DeclAST: public BaseAST
{
public:
    DeclType type;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;
    void DumpIR() override
    {
        if (type == DeclType::CONST_DECL)
        {
            const_decl->is_global = is_global;
            const_decl->DumpIR();
        }
        else if (type == DeclType::VAR_DECL)
        {
            var_decl->is_global = is_global;
            var_decl->DumpIR();
        }
        else{
            assert(false);
        }
    }
};

class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<VecAST> const_defs;
    void DumpIR() override
    {
        for (auto &def: const_defs->vec)
        {
            def->is_global = is_global;
            def->DumpIR();
        }
    }
};

class ConstDefAST: public BaseAST
{
public:
    std::unique_ptr<BaseExpAST> const_init_val;
    void DumpIR() override
    {
        const_init_val->Eval();
        symbol_table_stack.Insert(ident, const_init_val->value);
    }
};

class ConstInitValAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseExpAST> const_exp;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        const_exp->Eval();
        Copy(const_exp);
        is_evaled = true;
    }
};

class BlockItemAST : public BaseAST
{
public:
    BlockItemType type;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;
    void DumpIR() override
    {
        if (type == BlockItemType::BLK_DECL)
        {
            decl->DumpIR();
        }
        else if (type == BlockItemType::BLK_STMT)
        {
            stmt->DumpIR();
        }
    }
};

class LValAST : public BaseExpAST
{
public:
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        symbol_info_t *info = symbol_table_stack.LookUp(ident);
        assert(info != nullptr);
        if (!is_left)
        {
            if (info->type == SYMBOL_TYPE::CONST_SYMBOL)
            {
                value = info->value;
                ident = std::to_string(value);
                is_const = true;
            }
            else if (info->type == SYMBOL_TYPE::VAR_SYMBOL)
            {
                ident = "%" + std::to_string(symbol_count++);
                std::cout << "  " << ident << " = load " << info->ir_name << std::endl;
            }
        }
        is_evaled = true;
    }
};

class ConstExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseExpAST> exp;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        exp->Eval();
        Copy(exp);
        is_evaled = true;
    }
};

class VarDeclAST: public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<VecAST> var_defs;
    void DumpIR() override
    {
        for (auto &def: var_defs->vec)
        {
            def->is_global = is_global;
            def->DumpIR();
        }
    }
};

class VarDefAST: public BaseAST
{
public:
    VarDefType type;
    std::unique_ptr<BaseExpAST> init_val;
    void DumpIR() override
    {
        if (is_global)
        {
            std::cout << "global ";
        }
        std::string ir_name = "@" + ident;
        ir_name = symbol_table_stack.Insert(ident, ir_name);
        if (!is_global)
        {
            std::cout << "  ";
        }
        std::cout << ir_name << " = alloc i32";
        if (!is_global)
        {
            std::cout << std::endl;
        }
        if (type == VarDefType::VAR_ASSIGN)
        {
            init_val->Eval();
            if (is_global)
            {
                std::cout << ", " << init_val->ident << std::endl;
            }
            else
            {
                std::cout << "  " << "store " << init_val->ident << ", " << ir_name << std::endl;
            }
        }
        else
        {
            if (is_global)
            {
                std::cout << ", zeroinit" << std::endl;
            }
        }
    }
};

class InitValAST: public BaseExpAST
{
public:
    std::unique_ptr<BaseExpAST> exp;
    void DumpIR() override
    {
    }
    void Eval() override
    {
        if (is_evaled)
        {
            return;
        }
        exp->Eval();
        Copy(exp);
        is_evaled = true;
    }
};

class FuncFParamAST: public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    void DumpIR() override
    {
        btype->DumpIR();
        std::cout << "@" << ident << ": " << btype->ident;
    }
};