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
