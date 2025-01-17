# 编译原理课程实践报告：可怜的编译器（或其它名字）

数学科学学院 2100010785 叶飞越

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 由SysY语言程序得到抽象语法树
2. 由抽象语法树生成中间代码 Koopa IR
3. 由中间代码 Koopa IR 生成目标代码 (RISCV)

### 1.2 主要特点

本编译器的主要特点是
- 生成 Koopa IR 的过程只需要通过嵌套的 AST 进行一次遍历即可
- 生成目标代码的过程只依赖前端程序得到的 Koopa IR , 不依赖前端程序的中间结果
- Koopa IR 中的局部变量都直接保存在栈上, 寄存器只存储运算过程中的中间结果

## 二、编译器设计

### 2.1 主要模块组成

编译器由 4 个主要模块组成: ```sysy.l``` 和 ```sysy.y``` 负责词法分析和语法分析, 得到 ```ast.h``` 中定义的抽象语法树; ```ast.h``` 负责递归遍历抽象语法树, 编译生成 Koopa IR; ```main.cpp``` 和 ```riscv.h``` 负责将 Koopa IR 转换为内存形式, 再通过扫描内存形式的 IR 生成目标代码; ```table.h``` 负责维护编译过程中的符号表. 

### 2.2 主要数据结构

本编译器主要数据结构是 AST 树, 所有 AST 类都是基类 ```class BaseAST``` 的衍生类, 实例 ```class CompUnitAST``` 是这棵树的根, 函数定义则由 ```class FunDefAST``` 表示, 等等. 其中基类 ```class BaseAST``` 的定义为: 

```c
class BaseAST
{
public:
    string ident = "";
    bool is_global = false;
    virtual void DumpIR() = 0;
    virtual ~BaseAST() = default;
};
```

其中 ```ident``` 用来记录当前节点的信息, 辅助生成 Koopa IR ; ```is_global``` 用来记录函数或者变量是否是全局的; ```DumpIR()``` 递归地生成中间代码. 

对于一般的 AST 节点, 我们根据产生式给出定义; 如果该节点对应的非终结符有多条生成规则, 则定义相应的 ```enum``` 来区分不同的规则: 

```c
enum PrimaryExpType{EXP, NUMBER, LVAL};
class PrimaryExpAST : public BaseExpAST
{
public:
    PrimaryExpType type;
    int num;
    unique_ptr<BaseExpAST> exp;
    unique_ptr<BaseExpAST> lval;
    void DumpIR() override;
    void Eval() override;
}
```

其中 ```Eval()``` 函数用来进行编译期间的计算. 

对于 ```if/else``` 语句的二义性问题, 按照课上讲过的方法对生成规则进行扩充: 

```
Stmt :: = OpenStmt |  ClosedStmt;
OpenStmt :: = IF '(' Exp ')' ClosedStmt |  IF '(' Exp ')' OpenStmt | 
              IF '(' Exp ')' ClosedStmt ELSE OpenStmt | WHILE '(' Exp ')' OpenStmt
ClosedStmt :: = SimpleStmt | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt
SimpleStmt :: = LVal "=" Exp ";" | [Exp] ";" | Block | "return" [Exp] ";";
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑
符号表采用 ```unordered_map``` 实现, 其中 ```key``` 是变量名, ```value``` 包含以下信息:

- ```SYMBOL_TYPE type``` 表示符号种类, 包括常量和变量 (目前只实现到 lv8 )
- ```int value``` 用来维护编译期间计算出的值
- ```string ir_name``` 表示变量在 Koopa IR 中的变量名

变量作用域则用 ```SymbolTableStack``` 来实现, 其作用原理为: 进入代码块时, 在这个结构里新建一个符号表 ```SymbolTable``` , 这个符号表就代表当前的符号表; 退出代码块时, 删除刚刚创建的符号表, 进入代码块之前的那个符号表就代表当前的符号表. 

#### 2.3.2 寄存器分配策略
将所有局部变量分配在栈上, 维护了栈帧 ```StackFrame``` , 在函数的开始扫描整个函数计算出栈帧的大小, 扫描到变量时, 将变量直接压入栈, 在函数的结尾将栈恢复到初始状态. 寄存器只用来存储运算过程中的中间结果, 维护 ```RegManager``` 来分配中间结果需要的寄存器. 

#### 2.3.3 采用的优化策略
未采取优化. 

#### 2.3.4 其它补充设计考虑
暂无. 

## 三、编译器实现

### 3.1 各阶段编码细节

介绍各阶段实现的时候自己认为有价值的信息，本部分内容**不做特别要求和限制**。可按如下所示的各阶段组织。

#### Lv1. main函数和Lv2. 初试目标代码生成

中间代码生成部分按照 AST 树的结构递归处理; 目标代码生成部分只需处理 ```ret``` 和 ```integer```

#### Lv3. 表达式

这部分的主要内容是利用 AST 的层次结构处理优先级, 优先级较低的运算在抽象语法树中层次较高. 这部分抽象语法树的 AST 类优先级从上到下为: 

```c
class ExpAST : public BaseAST;
class LOrExpAST : public BaseAST;
class LAndExpAST : public BaseAST;
class EqExpAST : public BaseAST;
class RelExpAST : public BaseAST;
class AddExpAST : public BaseAST;
class MulExpAST : public BaseAST;
class UnaryExpAST : public BaseAST;
class PrimaryExpAST : public BaseAST;
```

#### Lv4. 常量和变量

引入了常量和变量的实现, 因此添加了符号表, 程序中用 ```table.h``` 来维护符号表. 

#### Lv5. 语句块和作用域

引入了作用域的实现, 因此将符号表拓展成了符号表栈.

#### Lv6. if语句

为了处理 ```if/else``` 语句的二义性问题, 将生成规则进行了修改. 同时也实现了跳转指令. 

#### Lv7. while语句

跟 ```if``` 语句类似, 将 ```while+break/continue``` 的语句转换成 ```jump``` 形式即可. 

#### Lv8. 函数和全局变量

调用函数时, 将传入的前 8 个参数存入对应的寄存器中, 剩下的存在栈上. 对于库函数, 将所有 SysY 库函数的定义放在代码开头. 

#### Lv9. 数组

这部分目前暂未实现. 

### 3.2 工具软件介绍（若未使用特殊软件或库，则本部分可略过）
1. Flex/Bison: 进行词法分析和语法分析. 
2. LibKoopa: 用于生成 Koopa IR 中间代码的结构, 以便 RISC-V 目标代码的生成. 

### 3.3 测试情况说明（如果进行过额外的测试，可增加此部分内容）

暂无. 

## 四、实习总结

这次 lab 完成的比较仓促, 未能完成全部内容, 写出一个能真正编译 SysY 程序的任何代码的编译器. 主要参考了开源代码 https://github.com/NovaPigeon/sysy-compiler 的实现. 