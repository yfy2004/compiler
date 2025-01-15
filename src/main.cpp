#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include "ast.h"
#include "riscv.h"
#include "koopa.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

FILE *fout;

int main(int argc, const char *argv[])
{
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    yyin = fopen(input, "r");
    assert(yyin);
    fout = fopen(output, "w");
    assert(fout);

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    std::string IR = ast->DumpIR();

    if (strcmp(mode, "-koopa") == 0)
    {
        fprintf(fout, "%s\n", IR.c_str());
        cout << IR;
    }
    else if(strcmp(mode,"-riscv") == 0)
    {
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string(IR.c_str(), &program);
        assert(ret == KOOPA_EC_SUCCESS); 
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);

        std::string riscv = Visit(raw);
        fprintf(fout, "%s\n", riscv.c_str());
        cout << riscv;

        koopa_delete_raw_program_builder(builder);
    }

    return 0;
}