%code requires {
    #include <memory>
    #include <string>
    #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
    std::string *str_val;
    int int_val;
    BaseAST *ast_val;
    BaseExpAST *exp_val;
    VecAST *vec_val;
    ExpVecAST *exp_vec_val;
}

// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> LE GE EQ NEQ AND OR

%type <ast_val> FuncDef Type Block Stmt
%type <ast_val> Decl ConstDecl ConstDef BlockItem VarDef VarDecl
%type <ast_val> OpenStmt ClosedStmt SimpleStmt
%type <ast_val> FuncFParam
%type <exp_val> Exp PrimaryExp UnaryExp MulExp 
%type <exp_val> AddExp RelExp EqExp LAndExp LOrExp
%type <exp_val> ConstInitVal LVal ConstExp InitVal
%type <vec_val> BlockItems ConstDefs VarDefs
%type <vec_val> FuncFParams CompUnits
%type <exp_vec_val> FuncRParams
%type <str_val> UnaryOP MulOP AddOP RelOP EqOP
%type <int_val> Number

%%

CompUnit
    : CompUnits {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->comp_units = unique_ptr<VecAST>($1);
        ast = move(comp_unit);
    }
    ;

CompUnits 
    : FuncDef {
        auto comp_units = new VecAST();
        auto func_def = unique_ptr<BaseAST>($1);
        comp_units->push_back(func_def);
        $$ = comp_units;
    }
    | Decl {
        auto comp_units = new VecAST();
        auto decl = unique_ptr<BaseAST>($1);
        comp_units->push_back(decl);
        $$ = comp_units;
    }
    | CompUnits FuncDef {
        auto comp_units = ($1);
        auto func_def = unique_ptr<BaseAST>($2);
        comp_units->push_back(func_def);
        $$ = comp_units;
    }
    | CompUnits Decl {
        auto comp_units = ($1);
        auto decl = unique_ptr<BaseAST>($2);
        comp_units->push_back(decl);
        $$ = comp_units;
    }
    ;

FuncDef
    : Type IDENT '(' ')' Block {
        auto funcdef = new FuncDefAST();
        funcdef->func_type = unique_ptr<BaseAST>($1);
        funcdef->ident = *unique_ptr<string>($2);
        funcdef->block = unique_ptr<BaseAST>($5);
        funcdef->func_fparams=unique_ptr<VecAST>(new VecAST());
        $$ = funcdef;
    }
    | Type IDENT '(' FuncFParams ')' Block {
        auto funcdef = new FuncDefAST();
        funcdef->func_type = unique_ptr<BaseAST>($1);
        funcdef->ident = *unique_ptr<string>($2);
        funcdef->block = unique_ptr<BaseAST>($6);
        funcdef->func_fparams=unique_ptr<VecAST>($4);
        $$ = funcdef;
    }
    ;

FuncFParams
    : FuncFParam {
        auto params = new VecAST();
        auto func_fparam = unique_ptr<BaseAST>($1);
        params->push_back(func_fparam);
        $$ = params;
    }
    | FuncFParams ',' FuncFParam {
        auto params = ($1);
        auto func_fparam = unique_ptr<BaseAST>($3);
        params->push_back(func_fparam);
        $$ = params;
    }
    ;

FuncFParam
    : Type IDENT {
        auto func_fparam = new FuncFParamAST();
        func_fparam->btype = unique_ptr<BaseAST>($1);
        func_fparam->ident = *unique_ptr<string>($2);
        $$ = func_fparam;
    }
    ;

Type
    : INT {
        auto functype = new TypeAST();
        functype->type = "int";
        $$ = functype;
    }
    | VOID {
        auto functype = new TypeAST();
        functype->type = "void";
        $$ = functype;
    }
    ;

Block
    : '{' BlockItems '}' {
        auto block = new BlockAST();
        block->items = unique_ptr<VecAST>($2);
        $$ = block;
    }
    | '{' '}' {
        auto block = new BlockAST();
        block->items = unique_ptr<VecAST>(new VecAST());
        $$ = block;
    }
    ;

BlockItems
    : BlockItems BlockItem {
        auto items = ($1);
        auto block_item = unique_ptr<BaseAST>($2);
        items->push_back(block_item);
        $$ = items;
    }
    | BlockItem {
        auto items = new VecAST();
        auto block_item = unique_ptr<BaseAST>($1);
        items->push_back(block_item);
        $$ = items;
    }
    ;

BlockItem
    : Decl {
        auto block_item = new BlockItemAST();
        block_item->decl = unique_ptr<BaseAST>($1);
        block_item->type = BlockItemType::BLK_DECL;
        $$ = block_item;
    }
    | Stmt {
        auto block_item = new BlockItemAST();
        block_item->stmt = unique_ptr<BaseAST>($1);;
        block_item->type = BlockItemType::BLK_STMT;
        $$ = block_item;
    }
    ;

Stmt
    : OpenStmt {
        auto stmt = new StmtAST();
        stmt->type = StmtType::STMT_OPEN;
        stmt->open_stmt = unique_ptr<BaseAST>($1);
        $$ = stmt;
    }
    | ClosedStmt {
        auto stmt = new StmtAST();
        stmt->type = StmtType::STMT_CLOSED;
        stmt->closed_stmt = unique_ptr<BaseAST>($1);
        $$ = stmt;
    }
    ;

SimpleStmt
    : RETURN Exp ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_RETURN;
        stmt->exp = unique_ptr<BaseExpAST>($2);
        $$ = stmt;
    }
    | LVal '=' Exp ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_ASSIGN;
        stmt->lval = unique_ptr<BaseExpAST>($1);
        stmt->exp = unique_ptr<BaseExpAST>($3);
        $$ = stmt;
    }
    | ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_EMPTY_EXP;
        $$ = stmt;
    }
    | Exp ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_EXP;
        stmt->exp = unique_ptr<BaseExpAST>($1);
        $$ = stmt;
    }
    | Block {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_BLK;
        stmt->block = unique_ptr<BaseAST>($1);
        $$ = stmt;
    }
    | RETURN ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_EMPTY_RET;
        $$ = stmt;
    }
    | BREAK ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_BREAK;
        $$ = stmt;
    }
    | CONTINUE ';' {
        auto stmt = new SimpleStmtAST();
        stmt->type = SimpleStmtType::SSTMT_CONTINUE;
        $$ = stmt;
    }
    ;

Exp
    : LOrExp {
        auto exp = new ExpAST();
        exp->exp = unique_ptr<BaseExpAST>($1);
        $$=exp;
    }
    ;

PrimaryExp
    : '(' Exp ')' {
        auto primary_exp = new PrimaryExpAST();
        primary_exp->type = PrimaryExpType::EXP;
        primary_exp->exp = unique_ptr<BaseExpAST>($2);
        $$ = primary_exp;
    }
    | Number {
        auto primary_exp = new PrimaryExpAST();
        primary_exp->type = PrimaryExpType::NUMBER;
        primary_exp->num = ($1);
        $$ = primary_exp;
    }
    | LVal {
        auto primary_exp = new PrimaryExpAST();
        primary_exp->lval = unique_ptr<BaseExpAST>($1);
        primary_exp->type = PrimaryExpType::LVAL;
        $$ = primary_exp;
    }
    ;

UnaryExp
    : PrimaryExp {
        auto unary_exp = new UnaryExpAST();
        unary_exp->type = UnaryExpType::PRIMARY;
        unary_exp->primary_exp = unique_ptr<BaseExpAST>($1);
        $$ = unary_exp;
    }
    | UnaryOP UnaryExp {
        auto unary_exp = new UnaryExpAST();
        unary_exp->type = UnaryExpType::UNARY;
        unary_exp->op = *unique_ptr<string>($1);
        unary_exp->unary_exp = unique_ptr<BaseExpAST>($2);
        $$ = unary_exp;
    }
    | IDENT '(' ')' {
        auto unary_exp = new UnaryExpAST();
        unary_exp->func_name = *unique_ptr<string>($1);
        unary_exp->func_rparams = unique_ptr<ExpVecAST>(new ExpVecAST());
        unary_exp->type = UnaryExpType::CALL;
        $$ = unary_exp;
    }
    | IDENT '(' FuncRParams ')' {
        auto unary_exp = new UnaryExpAST();
        unary_exp->func_name = *unique_ptr<string>($1);
        unary_exp->func_rparams = unique_ptr<ExpVecAST>($3);
        unary_exp->type = UnaryExpType::CALL;
        $$ = unary_exp;
    }
    ;

FuncRParams
    : FuncRParams ',' Exp {
        auto params = ($1);
        auto exp = unique_ptr<BaseExpAST>($3);
        params->push_back(exp);
        $$ = params;
    }
    | Exp {
        auto params = new ExpVecAST();
        auto exp = unique_ptr<BaseExpAST>($1);
        params->push_back(exp);
        $$ = params;
    }
    ;

MulExp
    : UnaryExp {
        auto mul_exp = new MulExpAST();
        mul_exp->type = BianryOPExpType::INHERIT;
        mul_exp->unary_exp = unique_ptr<BaseExpAST>($1);
        $$ = mul_exp;
    }
    | MulExp MulOP UnaryExp {
        auto mul_exp = new MulExpAST();
        mul_exp->type = BianryOPExpType::EXPAND;
        mul_exp->mul_exp = unique_ptr<BaseExpAST>($1);
        mul_exp->op = *unique_ptr<string>($2);
        mul_exp->unary_exp = unique_ptr<BaseExpAST>($3);
        $$ = mul_exp;
    }
    ;

AddExp
    : MulExp {
        auto add_exp = new AddExpAST();
        add_exp->type = BianryOPExpType::INHERIT;
        add_exp->mul_exp = unique_ptr<BaseExpAST>($1);
        $$ = add_exp;
    }
    | AddExp AddOP MulExp {
        auto add_exp = new AddExpAST();
        add_exp->type = BianryOPExpType::EXPAND;
        add_exp->add_exp = unique_ptr<BaseExpAST>($1);
        add_exp->op = *unique_ptr<string>($2);
        add_exp->mul_exp = unique_ptr<BaseExpAST>($3);
        $$ = add_exp;
    }
    ;

RelExp
    : AddExp {
        auto rel_exp = new RelExpAST();
        rel_exp->type = BianryOPExpType::INHERIT;
        rel_exp->add_exp = unique_ptr<BaseExpAST>($1);
        $$ = rel_exp;
    }
    | RelExp RelOP AddExp {
        auto rel_exp = new RelExpAST();
        rel_exp->type = BianryOPExpType::EXPAND;
        rel_exp->rel_exp = unique_ptr<BaseExpAST>($1);
        rel_exp->op = *unique_ptr<string>($2);
        rel_exp->add_exp = unique_ptr<BaseExpAST>($3);
        $$ = rel_exp;
    }
    ;

EqExp
    : RelExp {
        auto eq_exp = new EqExpAST();
        eq_exp->type = BianryOPExpType::INHERIT;
        eq_exp->rel_exp = unique_ptr<BaseExpAST>($1);
        $$ = eq_exp;
    }
    | EqExp EqOP RelExp {
        auto eq_exp = new EqExpAST();
        eq_exp->type = BianryOPExpType::EXPAND;
        eq_exp->eq_exp = unique_ptr<BaseExpAST>($1);
        eq_exp->op = *unique_ptr<string>($2);
        eq_exp->rel_exp = unique_ptr<BaseExpAST>($3);
        $$ = eq_exp;
    }
    ;

LAndExp
    : EqExp {
        auto land_exp = new LAndExpAST();
        land_exp->type = BianryOPExpType::INHERIT;
        land_exp->eq_exp = unique_ptr<BaseExpAST>($1);
        $$ = land_exp;
    }
    | LAndExp AND EqExp {
        auto land_exp = new LAndExpAST();
        land_exp->type = BianryOPExpType::EXPAND;
        land_exp->land_exp = unique_ptr<BaseExpAST>($1);
        land_exp->eq_exp = unique_ptr<BaseExpAST>($3);
        $$ = land_exp;
    }
    ;

LOrExp
    : LAndExp {
        auto lor_exp = new LOrExpAST();
        lor_exp->type = BianryOPExpType::INHERIT;
        lor_exp->land_exp = unique_ptr<BaseExpAST>($1);
        $$ = lor_exp;
    }
    | LOrExp OR LAndExp {
        auto lor_exp = new LOrExpAST();
        lor_exp->type = BianryOPExpType::EXPAND;
        lor_exp->lor_exp = unique_ptr<BaseExpAST>($1);
        lor_exp->land_exp = unique_ptr<BaseExpAST>($3);
        $$ = lor_exp;
    }
    ;

UnaryOP
    : '+' {
        string *op = new string("+");
        $$ = op;
    }
    | '-' {
        string *op = new string("-");
        $$ = op;
    }
    | '!' {
        string *op = new string("!");
        $$ = op;
    }
    ;

MulOP
    : '*' {
        string *op = new string("*");
        $$ = op;
    }
    | '/' {
        string *op = new string("/");
        $$ = op;
    }
    | '%' {
        string *op = new string("%");
        $$ = op;
    }
    ;

AddOP
    : '+' {
        string *op = new string("+");
        $$ = op;
    }
    | '-' {
        string *op = new string("-");
        $$ = op;
    }
    ;

RelOP
    : '<' {
        string *op = new string("<");
        $$ = op;
    }
    | '>' {
        string *op = new string(">");
        $$ = op;
    }
    | LE {
        string *op = new string("<=");
        $$ = op;
    }
    | GE {
        string *op = new string(">=");
        $$ = op;
    }
    ;

EqOP
    : EQ {
        string *op = new string("==");
        $$ = op;
    }
    | NEQ {
        string *op = new string("!=");
        $$ = op;
    }
    ;

Number
    : INT_CONST {
        $$ = ($1);
    }
    ;

Decl
    : ConstDecl {
        auto decl = new DeclAST();
        decl->type = DeclType::CONST_DECL;
        decl->const_decl = unique_ptr<BaseAST>($1);
        $$ = decl;
    }
    | VarDecl {
        auto decl = new DeclAST();
        decl->type = DeclType::VAR_DECL;
        decl->var_decl = unique_ptr<BaseAST>($1);
        $$ = decl;
    }
    ;

ConstDecl
    : CONST Type ConstDefs ';' {
        auto const_decl = new ConstDeclAST();
        const_decl->btype = unique_ptr<BaseAST>($2);
        const_decl->const_defs = unique_ptr<VecAST>($3);
        $$ = const_decl;
    }
    ;

ConstDefs
    : ConstDef {
        auto const_defs = new VecAST();
        auto const_def = unique_ptr<BaseAST>($1);
        const_defs->push_back(const_def);
        $$ = const_defs;
    } 
    | ConstDefs ',' ConstDef {
        auto const_defs = $1;
        auto const_def = unique_ptr<BaseAST>($3);
        const_defs->push_back(const_def);
        $$ = const_defs;
    }
    ;

ConstDef
    : IDENT '=' ConstInitVal {
        auto const_def = new ConstDefAST();
        const_def->ident = *unique_ptr<string>($1);
        const_def->const_init_val = unique_ptr<BaseExpAST>($3);
        $$ = const_def;
    }
    ;

ConstInitVal
    : ConstExp {
        auto const_init_val = new ConstInitValAST();
        const_init_val->const_exp = unique_ptr<BaseExpAST>($1);
        $$ = const_init_val;
    }
    ;

ConstExp
    : Exp {
        auto const_exp = new ConstExpAST();
        const_exp->exp = unique_ptr<BaseExpAST>($1);
        $$ = const_exp;
    }
    ;

LVal
    : IDENT {
        auto lval = new LValAST();
        lval->ident = *unique_ptr<string>($1);
        $$ = lval;
    }
    ;

VarDecl
    : Type VarDefs ';' {
        auto var_decl = new VarDeclAST();
        var_decl->btype = unique_ptr<BaseAST>($1);
        var_decl->var_defs = unique_ptr<VecAST>($2);
        $$ = var_decl;
    }
    ;

VarDefs
    : VarDef {
        auto var_def = unique_ptr<BaseAST>($1);
        auto var_defs = new VecAST();
        var_defs->push_back(var_def);
        $$ = var_defs;
    }
    | VarDefs ',' VarDef {
        auto var_defs = $1;
        auto var_def = unique_ptr<BaseAST>($3);
        var_defs->push_back(var_def);
        $$ = var_defs;
    }
    ;

VarDef
    : IDENT {
        auto var_def = new VarDefAST();
        var_def->type = VarDefType::VAR;
        var_def->ident = *unique_ptr<string>($1);
        $$ = var_def;
    }
    | IDENT '=' InitVal {
        auto var_def = new VarDefAST();
        var_def->type = VarDefType::VAR_ASSIGN;
        var_def->ident = *unique_ptr<string>($1);
        var_def->init_val = unique_ptr<BaseExpAST>($3);
        $$ = var_def;
    }
    ;

InitVal
    : Exp {
        auto init_val = new InitValAST();
        init_val->exp = unique_ptr<BaseExpAST>($1);
        $$ = init_val;
    }
    ;

OpenStmt
    : IF '(' Exp ')' ClosedStmt {
        auto open_stmt = new OpenStmtAST();
        open_stmt->type = OpenStmtType::OSTMT_CLOSED;
        open_stmt->exp = unique_ptr<BaseExpAST>($3);
        open_stmt->closed_stmt = unique_ptr<BaseAST>($5);
        $$ = open_stmt;
    }
    | IF '(' Exp ')' OpenStmt {
        auto open_stmt = new OpenStmtAST();
        open_stmt->type = OpenStmtType::OSTMT_OPEN;
        open_stmt->exp = unique_ptr<BaseExpAST>($3);
        open_stmt->open_stmt = unique_ptr<BaseAST>($5);
        $$ = open_stmt;
    }
    | IF '(' Exp ')' ClosedStmt ELSE OpenStmt{
        auto open_stmt = new OpenStmtAST();
        open_stmt->type = OpenStmtType::OSTMT_ELSE;
        open_stmt->exp = unique_ptr<BaseExpAST>($3);
        open_stmt->closed_stmt = unique_ptr<BaseAST>($5);
        open_stmt->open_stmt = unique_ptr<BaseAST>($7);
        $$ = open_stmt;
    }
    | WHILE '(' Exp ')' OpenStmt {
        auto open_stmt = new OpenStmtAST();
        open_stmt->type = OpenStmtType::OSTMT_WHILE;
        open_stmt->exp = unique_ptr<BaseExpAST>($3);
        open_stmt->open_stmt = unique_ptr<BaseAST>($5);
        $$ = open_stmt;
    }
    ;

ClosedStmt
    : SimpleStmt {
        auto closed_stmt = new ClosedStmtAST();
        closed_stmt->type = ClosedStmtType::CSTMT_SIMPLE;
        closed_stmt->simple_stmt = unique_ptr<BaseAST>($1);
        $$ = closed_stmt;
    }
    | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
        auto closed_stmt = new ClosedStmtAST();
        closed_stmt->type = ClosedStmtType::CSTMT_ELSE;
        closed_stmt->exp = unique_ptr<BaseExpAST>($3);
        closed_stmt->closed_stmt1 = unique_ptr<BaseAST>($5);
        closed_stmt->closed_stmt2 = unique_ptr<BaseAST>($7);
        $$ = closed_stmt;
    }
    | WHILE '(' Exp ')' ClosedStmt {
        auto closed_stmt = new ClosedStmtAST();
        closed_stmt->type = ClosedStmtType::CSTMT_WHILE;
        closed_stmt->exp = unique_ptr<BaseExpAST>($3);
        closed_stmt->closed_stmt1 = unique_ptr<BaseAST>($5);
        $$ = closed_stmt;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}