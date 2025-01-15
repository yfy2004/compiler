#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <fstream>
#include "ast.h"
#include "riscv.h"
#include "koopa.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[])
{
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    yyin = fopen(input, "r");
    assert(yyin);
    ofstream fout(output);
    assert(fout);

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    streambuf *old_cout = std::cout.rdbuf(fout.rdbuf());

    if (strcmp(mode, "-koopa") == 0)
    {
        ast->DumpIR();
    }
    else if (strcmp(mode, "-riscv") == 0)
    {
        std::stringstream ss;
        cout.rdbuf(ss.rdbuf());
        ast->DumpIR();
        cout.rdbuf(fout.rdbuf());
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string((ss.str().c_str()), &program);
        assert(ret == KOOPA_EC_SUCCESS); 
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);

        Visit(raw);

        koopa_delete_raw_program_builder(builder);
    }

    cout.rdbuf(old_cout);
    fout.close();

    return 0;
}