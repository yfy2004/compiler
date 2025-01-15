#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include "koopa.h"
#define REG_NUM 15

std::string Visit(const koopa_raw_program_t &program);
std::string Visit(const koopa_raw_slice_t &slice);
std::string Visit(const koopa_raw_function_t &func);
std::string Visit(const koopa_raw_basic_block_t &bb);
std::string Visit(const koopa_raw_value_t &value);
std::string Visit(const koopa_raw_return_t &ret);
std::string Visit(const koopa_raw_integer_t &interger);
std::string Visit(const koopa_raw_binary_t &binary);
std::string get_instructions(std::string str);
std::string get_reg(std::string str);
std::string gen_reg(int id);

const std::string regs[REG_NUM] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
std::map<koopa_raw_value_t, std::string> is_visited;
std::map<koopa_raw_binary_op_t, std::string> op_names = {{KOOPA_RBO_GT, "sgt"}, {KOOPA_RBO_LT, "slt"}, {KOOPA_RBO_ADD, "add"}, {KOOPA_RBO_SUB, "sub"}, {KOOPA_RBO_MUL, "mul"}, {KOOPA_RBO_DIV, "div"}, {KOOPA_RBO_MOD, "rem"}, {KOOPA_RBO_AND, "and"}, {KOOPA_RBO_OR, "or"}};

int temp_reg_count = 0;
int reg_count = 0;

std::string Visit(const koopa_raw_program_t &program)
{
    std::string riscv = "  .text\n";
    riscv += Visit(program.values);
    riscv += Visit(program.funcs);
    return riscv;
}

std::string Visit(const koopa_raw_slice_t &slice)
{
    std::string riscv = "";
    for (size_t i = 0; i < slice.len; i++)
    {
        auto ptr = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            riscv += get_instructions(Visit(reinterpret_cast<koopa_raw_function_t>(ptr)));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            riscv += get_instructions(Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr)));
            break;
        case KOOPA_RSIK_VALUE:
            riscv += get_instructions(Visit(reinterpret_cast<koopa_raw_value_t>(ptr)));
            break;
        default:
            assert(false);
        }
    }
    return riscv;
}

std::string Visit(const koopa_raw_function_t &func)
{
    std::string func_name = std::string(func->name + 1);
    std::string riscv = "  .globl " + func_name + "\n";
    riscv += func_name + ":\n";
    riscv += Visit(func->bbs);
    return riscv;
}

std::string Visit(const koopa_raw_basic_block_t &bb)
{
    return Visit(bb->insts);
}

std::string Visit(const koopa_raw_value_t &value)
{
    if (is_visited.find(value) != is_visited.end())
    {
        return is_visited[value];
    }
    std::string riscv = "";
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        riscv += Visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        riscv += Visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        riscv += Visit(kind.data.binary);
        break;
    default:
        assert(false);
    }
    is_visited[value] = get_reg(riscv);
    return riscv;
}

std::string Visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t value = ret.value;
    std::string temp = Visit(value);
    std::string reg = get_reg(temp);
    std::string instructions = get_instructions(temp);
    std::string riscv = "a0\n" + instructions + "  mv a0, " + reg + "\n";
    riscv += "  ret\n";
    return riscv;

}

std::string Visit(const koopa_raw_integer_t &interger)
{
    int32_t value = interger.value;
    if (value == 0)
    {
        return "x0\n";
    }
    std::string reg = gen_reg(temp_reg_count++);
    std::string riscv = reg + "\n" "  li " + reg + ", " + std::to_string(value) + "\n";
    return riscv;
}

std::string Visit(const koopa_raw_binary_t &binary)
{
    temp_reg_count = reg_count;    
    std::string lhs = Visit(binary.lhs);
    std::string lhs_instructions = get_instructions(lhs);
    std::string lhs_reg = get_reg(lhs);
    std::string rhs = Visit(binary.rhs);
    std::string rhs_instructions = get_instructions(rhs);
    std::string rhs_reg = get_reg(rhs);
    temp_reg_count = reg_count;
    std::string new_reg = gen_reg(reg_count++);
    std::string riscv = new_reg + "\n" + lhs_instructions + rhs_instructions;
    std::string op_instructions = "";
    koopa_raw_binary_op_t op = binary.op;
    switch (op)
    {
    case KOOPA_RBO_GT:
    case KOOPA_RBO_LT:
    case KOOPA_RBO_ADD:
    case KOOPA_RBO_SUB:
    case KOOPA_RBO_MUL:
    case KOOPA_RBO_DIV:
    case KOOPA_RBO_MOD:
    case KOOPA_RBO_AND:
    case KOOPA_RBO_OR:
        op_instructions = op_names[op];
        riscv += "  " + op_instructions + " " + new_reg + ", " + lhs_reg + ", " + rhs_reg + "\n";
        break;
    case KOOPA_RBO_EQ:
        riscv += "  xor " + new_reg + ", " + lhs_reg + ", " + rhs_reg + "\n" + "  seqz " + new_reg + ", " + new_reg + "\n";
        break;
    case KOOPA_RBO_NOT_EQ:
        riscv += "  xor " + new_reg + ", " + lhs_reg + ", " + rhs_reg + "\n" + "  snez " + new_reg + ", " + new_reg + "\n";
        break;
    case KOOPA_RBO_LE:  
        riscv += "  sgt " + new_reg + ", " + lhs_reg + ", " + rhs_reg + "\n" + "  xori " + new_reg + ", " + new_reg + ", 1\n";
        break;
    case KOOPA_RBO_GE:
        riscv += "  slt " + new_reg + ", " + lhs_reg + ", " + rhs_reg + "\n" + "  xori " + new_reg + ", " + new_reg + ", 1\n";
    }
    return riscv;
}

std::string get_instructions(std::string str)
{
    std::istringstream iss(str);
    std::string var;
    std::getline(iss, var);
    std::string riscv;
    std::string line;
    if(var[0] == ' ')
    {
        riscv = var + "\n";
    }
    while (std::getline(iss, line))
    {
        riscv += line + "\n";
    }
    return riscv;
}

std::string get_reg(std::string str)
{
    std::istringstream iss(str);
    std::string var;
    std::getline(iss, var);
    return var;
}

std::string gen_reg(int id)
{
    if (id < REG_NUM)
    {
        return regs[id];
    }
    return "a" + std::to_string(id);
}