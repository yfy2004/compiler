#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include "koopa.h"

std::string Visit(const koopa_raw_program_t &program);
std::string Visit(const koopa_raw_slice_t &slice);
std::string Visit(const koopa_raw_function_t &func);
std::string Visit(const koopa_raw_basic_block_t &bb);
std::string Visit(const koopa_raw_value_t &value);
std::string Visit(const koopa_raw_return_t &ret);
std::string Visit(const koopa_raw_integer_t &interger);

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
            riscv += Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            riscv += Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            riscv += Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
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
    default:
        assert(false);
    }
    return riscv;
}

std::string Visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t value = ret.value;
    std::string riscv = Visit(value);
    riscv += "  ret\n";
    return riscv;

}

std::string Visit(const koopa_raw_integer_t &interger)
{
    int32_t value = interger.value;
    std::string riscv = "  li a0, " + std::to_string(value) + "\n";
    return riscv;
}