#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <sstream>
#include <map>
#include <unordered_map>
#include "koopa.h"
#define REG_NUM 15
#define MAX_IMMEDIATE_VAL 2048
#define ZERO_REG_ID 15
#define PARAM_REG_NUM 8

using namespace std;

class StackFrame
{
public:
    StackFrame()
    {
        top = 0;
    }
    void set_stack_size(int size, bool store_ra_, int max_args_num_)
    {
        assert(size % 16 == 0);
        assert(size >= 0);
        stack_size = size;
        top = (max_args_num_ - 8) * 4;
        if (top < 0)
        {
            top = 0;
        }
        store_ra = store_ra_;
    }
    int push()
    {
        top += 4;
        assert(top <= stack_size);
        return top - 4;
    }
    int get_stack_size() const
    {
        return stack_size;
    }
    bool is_store_ra() const
    {
        return store_ra;
    }
private:
    int stack_size;
    int top;
    bool store_ra;
} stack_frame;

class RegManager
{
private:
    unordered_map<int, bool> regs_occupied;
public:
    RegManager()
    {
        for (int i = 0; i < REG_NUM; i++)
        {
            regs_occupied[i] = false;
        }
    }
    void free_regs()
    {
        for (int i = 0; i < REG_NUM; i++)
        {
            regs_occupied[i] = false;
        }
    }
    int alloc_reg()
    {
        int ret = -1;
        for (int i = 0; i < REG_NUM; i++)
        {
            if (regs_occupied[i] == false)
            {
                ret = i;
                regs_occupied[i] = true;
                break;
            }
        }
        assert(ret != -1);
        return ret;
    }
    int alloc_reg(int i)
    {
        assert(regs_occupied[i] == false);
        regs_occupied[i] = true;
        return i;
    }
    void free(int i)
    {
        regs_occupied[i] = false;
    }
} reg_manager;

enum VAR_TYPE{ON_STACK, ON_REG, ON_GLOBAL};

typedef struct{
    VAR_TYPE type;
    int stack_location;
    int reg_id;
    string global_name;
} var_info_t;

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);
void Prologue(const koopa_raw_function_t &func);
void Epilogue();
var_info_t Visit(const koopa_raw_value_t &value);
var_info_t Visit(const koopa_raw_integer_t &interger);
var_info_t Visit(const koopa_raw_binary_t &binary);
var_info_t Visit(const koopa_raw_load_t &load);
var_info_t Visit(const koopa_raw_call_t &call, bool is_ret);
var_info_t Visit(const koopa_raw_global_alloc_t &global_alloc);
string gen_reg(int id);
void GenLoadStoreInst(string op, string reg1, int imm, string reg2);

static const string regs[REG_NUM + 1] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "x0"};
static map<koopa_raw_value_t, var_info_t> is_visited;
static map<koopa_raw_binary_op_t, string> op_names = {{KOOPA_RBO_GT, "sgt"}, {KOOPA_RBO_LT, "slt"}, {KOOPA_RBO_ADD, "add"}, {KOOPA_RBO_SUB, "sub"}, {KOOPA_RBO_MUL, "mul"}, {KOOPA_RBO_DIV, "div"}, {KOOPA_RBO_MOD, "rem"}, {KOOPA_RBO_AND, "and"}, {KOOPA_RBO_OR, "or"}};
static int global_count = 0;

void Visit(const koopa_raw_program_t &program)
{
    Visit(program.values);
    cout << "  .text" << endl;
    Visit(program.funcs);
}

void Visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; i++)
    {
        auto ptr = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
}

void Visit(const koopa_raw_function_t &func)
{
    if (func->bbs.len == 0)
    {
        return;
    }
    string func_name = string(func->name + 1);
    cout << "  .globl " << func_name << endl;
    cout << func_name << ":" << endl;
    Prologue(func);
    Visit(func->bbs);
    cout << endl;
}

void Visit(const koopa_raw_basic_block_t &bb)
{
    cout << bb->name + 1 << ":" << endl;
    Visit(bb->insts);
}

void Visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t value = ret.value;
    cout << endl << "  # ret" << endl;
    if (value)
    {
        var_info_t var = Visit(value);
        assert(var.type == VAR_TYPE::ON_REG);
        cout << "  mv a0, " << gen_reg(var.reg_id) << endl;
    }
    Epilogue();
    cout << "  ret" << endl;
}

void Visit(const koopa_raw_store_t &store)
{
    cout << endl << "  # store" << endl;
    koopa_raw_value_t dst = store.dest;
    assert(is_visited.find(dst) != is_visited.end());
    var_info_t src_var = Visit(store.value);
    assert(src_var.type == VAR_TYPE::ON_REG);
    var_info_t dst_var = is_visited[dst];
    if(dst->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        int reg_id = reg_manager.alloc_reg();
        cout << "  la " << gen_reg(reg_id) << ", " << dst_var.global_name << endl;
        cout << "  sw " << gen_reg(src_var.reg_id) << ", 0(" << gen_reg(reg_id) << ")" << endl;
    }
    else
    {
        GenLoadStoreInst("sw", gen_reg(src_var.reg_id), dst_var.stack_location, "sp");
    }
}

void Visit(const koopa_raw_branch_t &branch)
{
    cout << endl << "  # branch" << endl;
    string label_true = branch.true_bb->name + 1;
    string label_false = branch.false_bb->name + 1;
    var_info_t var = Visit(branch.cond);
    reg_manager.free_regs();
    string var_name;
    if (var.type == VAR_TYPE::ON_REG)
    {
        var_name = gen_reg(var.reg_id);
    }
    else
    {
        var_name = to_string(var.stack_location) + "(sp)";
    }
    cout << "  bnez  " << var_name << ", " << label_true << endl;
    cout << "  j     " << label_false << endl;
}

void Visit(const koopa_raw_jump_t &jump)
{
    cout << endl << "  # jump" << endl;
    string label_target = jump.target->name + 1;
    cout << "  j     " << label_target << endl;
    reg_manager.free_regs();
}

void Prologue(const koopa_raw_function_t &func)
{
    cout << endl << "  # prologue" << endl;
    int stack_size = 0;
    bool store_ra = false;
    int max_args_num = 0;
    for (uint32_t i = 0; i < func->bbs.len; i++)
    {
        koopa_raw_basic_block_t bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        for (uint32_t j = 0; j < bb->insts.len; ++j)
        {
            koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag != KOOPA_RTT_UNIT)
            {
                stack_size = stack_size + 4;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL)
            {
                store_ra = true;
                int args_num = inst->kind.data.call.args.len;
                if (args_num > max_args_num)
                {
                    max_args_num = args_num;
                }
            }
        }
    }
    if (store_ra)
    {
        stack_size += 4;
    }
    if (max_args_num > 8)
    {
        stack_size += (max_args_num - 8) * 4;
    }
    stack_size = (stack_size + 15) & (~15);
    stack_frame.set_stack_size(stack_size, store_ra, max_args_num);
    if (stack_size < MAX_IMMEDIATE_VAL)
    {
        cout << "  addi sp, sp, " << -stack_size << endl;
    }
    else
    {
        cout << "  li t0, " << -stack_size << endl;
        cout << "  add sp, sp, t0" << endl;
    }
    if(store_ra)
    {
        GenLoadStoreInst("sw", "ra", stack_size-4, "sp");
    }
    for (int i = 0; i < func->params.len; i++)
    {
        koopa_raw_value_t param = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);
        var_info_t param_info;
        if (i < PARAM_REG_NUM)
        {
            param_info.type = VAR_TYPE::ON_REG;
            param_info.reg_id = i + 7;
        }
        else
        {
            param_info.type = VAR_TYPE::ON_STACK;
            param_info.stack_location = stack_size + (i - 8) * 4;
        }
        is_visited[param] = param_info;
    }
}

void Epilogue()
{
    cout << endl << "  # epilogue" << endl;
    int stack_size = stack_frame.get_stack_size();
    bool store_ra = stack_frame.is_store_ra();
    if (store_ra)
    {
        GenLoadStoreInst("lw", "ra", stack_size - 4, "sp");
    }
    if (stack_size < MAX_IMMEDIATE_VAL)
    {
        cout << "  addi sp, sp, " << stack_size << endl;
    }
    else
    {
        cout << "  li t0, " << stack_size << endl;
        cout << "  add sp, sp, t0" << endl;
    }
}

var_info_t Visit(const koopa_raw_value_t &value)
{
    if (is_visited.find(value) != is_visited.end())
    {
        var_info_t info = is_visited[value];
        if (info.type == VAR_TYPE::ON_REG)
        {
            return info;
        }
        else if(info.type == VAR_TYPE::ON_STACK)
        {
            int location = info.stack_location;
            assert(location >= 0);
            int reg_id = reg_manager.alloc_reg();
            GenLoadStoreInst("lw", gen_reg(reg_id), location, "sp");
            info.type = VAR_TYPE::ON_REG;
            info.reg_id = reg_id;
            return info;
        }
        else if (info.type == VAR_TYPE::ON_GLOBAL)
        {
            int reg_id = reg_manager.alloc_reg();
            cout << "  la " << gen_reg(reg_id) << ", " << is_visited[value].global_name << endl;
            cout << "  lw " << gen_reg(reg_id) << ", 0(" << gen_reg(reg_id) << ")" << endl;
            info.type = VAR_TYPE::ON_REG;
            info.reg_id = reg_id;
            return info;
        }
    }
    const auto &kind = value->kind;
    var_info_t vinfo;
    bool is_ret = (value->ty->tag != KOOPA_RTT_UNIT);
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        Visit(kind.data.ret);
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_INTEGER:
        vinfo = Visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        vinfo = Visit(kind.data.binary);
        is_visited[value] = vinfo;
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_ALLOC:
        cout << endl << "  # alloc" << endl;
        vinfo.type = VAR_TYPE::ON_STACK;
        vinfo.stack_location = stack_frame.push();
        is_visited[value] = vinfo;
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_BRANCH:
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        Visit(kind.data.jump);
        break;
    case KOOPA_RVT_STORE:
        Visit(kind.data.store);
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_LOAD:
        vinfo = Visit(kind.data.load);
        is_visited[value] = vinfo;
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_CALL:
        vinfo = Visit(kind.data.call, is_ret);
        is_visited[value] = vinfo;
        reg_manager.free_regs();
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        vinfo = Visit(kind.data.global_alloc);
        assert(vinfo.type == VAR_TYPE::ON_GLOBAL);
        is_visited[value] = vinfo;
        break;
    default:
        assert(false);
    }
    return vinfo;
}

var_info_t Visit(const koopa_raw_integer_t &interger)
{
    int32_t value = interger.value;
    var_info_t vinfo;
    vinfo.type = VAR_TYPE::ON_REG;
    if (value == 0)
    {
        vinfo.reg_id = ZERO_REG_ID;
        return vinfo;
    }
    int new_reg_id = reg_manager.alloc_reg();
    vinfo.reg_id = new_reg_id;
    string reg = gen_reg(new_reg_id);
    cout << "  li " << reg << ", " << to_string(value) << endl;
    return vinfo;
}

var_info_t Visit(const koopa_raw_binary_t &binary)
{
    cout << endl << "  # binary" << endl;
    var_info_t lvar = Visit(binary.lhs);
    var_info_t rvar = Visit(binary.rhs);
    if (lvar.type == VAR_TYPE::ON_STACK)
    {
        lvar.type = VAR_TYPE::ON_REG;
        lvar.reg_id = reg_manager.alloc_reg();
        GenLoadStoreInst("lw", gen_reg(lvar.reg_id), lvar.stack_location, "sp");
    }
    if (rvar.type == VAR_TYPE::ON_STACK)
    {
        rvar.type = VAR_TYPE::ON_REG;
        rvar.reg_id = reg_manager.alloc_reg();
        GenLoadStoreInst("lw", gen_reg(rvar.reg_id), rvar.stack_location, "sp");
    }
    var_info_t tmp_result;
    tmp_result.type = VAR_TYPE::ON_REG;
    tmp_result.reg_id = reg_manager.alloc_reg();
    string new_reg = gen_reg(tmp_result.reg_id), l_reg = gen_reg(lvar.reg_id), r_reg = gen_reg(rvar.reg_id);
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
        cout << "  " << op_names[op] << " " << new_reg << ", " << l_reg << ", " << r_reg << endl;
        break;
    case KOOPA_RBO_EQ:
        cout << "  xor " << new_reg << ", " << l_reg << ", " << r_reg << endl;
        cout << "  seqz " << new_reg << ", " << new_reg << endl;
        break;
    case KOOPA_RBO_NOT_EQ:
        cout << "  xor " << new_reg << ", " << l_reg << ", " << r_reg << endl;
        cout << "  snez " << new_reg << ", " << new_reg << endl;;
        break;
    case KOOPA_RBO_LE:  
        cout << "  sgt " << new_reg << ", " << l_reg << ", " << r_reg << endl;
        cout << "  xori " << new_reg << ", " << new_reg << ", 1" << endl;
        break;
    case KOOPA_RBO_GE:
        cout << "  slt " << new_reg << ", " << l_reg << ", " << r_reg << endl;
        cout << "  xori " << new_reg << ", " << new_reg << ", 1" << endl;
    }
    var_info_t res;
    res.type = VAR_TYPE::ON_STACK;
    res.stack_location = stack_frame.push();
    GenLoadStoreInst("sw", new_reg, res.stack_location,"sp");
    return res;
}

var_info_t Visit(const koopa_raw_load_t &load)
{
    cout << endl << "  # load" << endl;
    var_info_t src_var = Visit(load.src);
    assert(src_var.type == VAR_TYPE::ON_REG);
    var_info_t dst_var;
    dst_var.type = VAR_TYPE::ON_STACK;
    dst_var.stack_location = stack_frame.push();
    GenLoadStoreInst("sw", gen_reg(src_var.reg_id), dst_var.stack_location, "sp");
    return dst_var;
}

var_info_t Visit(const koopa_raw_call_t &call, bool is_ret)
{
    cout << endl << "  # func" << endl;
    reg_manager.free_regs();
    for (int i = 0; i < call.args.len; i++)
    {
        koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        var_info_t info = Visit(arg);
        if (i < PARAM_REG_NUM)
        {
            if (i + 7 != info.reg_id)
            {
                reg_manager.alloc_reg(i + 7);
                cout << "  mv " << gen_reg(i + 7) << ", " << gen_reg(info.reg_id) << endl;
                reg_manager.free(info.reg_id);
            }
        }
        else
        {   
            GenLoadStoreInst("sw", gen_reg(info.reg_id), (i-8) * 4, "sp");
            reg_manager.free(info.reg_id);
        }
    }
    cout << "  call " << call.callee->name + 1 << endl;
    var_info_t info;
    if (is_ret)
    {
        info.type = VAR_TYPE::ON_STACK;
        info.stack_location = stack_frame.push();
        GenLoadStoreInst("sw", "a0", info.stack_location, "sp");
    }
    return info;
}

var_info_t Visit(const koopa_raw_global_alloc_t &global_alloc)
{
    string gname = "g_" + to_string(global_count++);
    cout << endl << "  # global alloc" << endl;
    cout << "  .data" << endl;
    cout << "  .globl " << gname << endl;
    cout << gname << ":" << endl;
    const auto &kind = global_alloc.init->kind.tag;
    switch (kind)
    {
        case KOOPA_RVT_ZERO_INIT:
            cout << "  .zero 4" << endl;
            break;
        case KOOPA_RVT_INTEGER:
            cout << "  .word " << global_alloc.init->kind.data.integer.value << endl;
            break;
        default:
            assert(false);
    }
    var_info_t vinfo;
    vinfo.type = VAR_TYPE::ON_GLOBAL;
    vinfo.global_name = gname;
    cout << endl;
    return vinfo;
}

string gen_reg(int id)
{
    if (id <= REG_NUM)
    {
        return regs[id];
    }
    assert(false);
}

void GenLoadStoreInst(string op, string reg1, int imm, string reg2)
{
    if (imm < MAX_IMMEDIATE_VAL)
    {
        cout << "  " << op << " " << reg1 << ", " << imm << "(" << reg2 << ")" << endl;
    }
    else
    {
        int reg_id = reg_manager.alloc_reg();
        string reg_tmp = gen_reg(reg_id);
        cout << "  li " << reg_tmp << ", " << imm << endl;
        cout << "  add " << reg_tmp << ", " << reg_tmp << ", " << reg2 << endl;
        cout << "  " << op << " " << reg1 << ", " << 0 << "(" << reg_tmp << ")" << endl;
    }
}