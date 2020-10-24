 // SPDX-License-Identifier: GPL-3.0-only
%{
#include <stdio.h>
#include <c_compiler/ast.h>
#include <c.tab.h>

// typedef struct ast_node *YYSTYPE;

const char *lex_ident;
long long int lex_int;

extern int yylex();
extern FILE *yyin;

void yyerror(struct ast_node **res, const char *str)
{
	(void)res;
	fprintf(stderr, "error: %s\n", str);
}

int yywrap()
{
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc < 2) return 1;
	yyin = fopen(argv[2], "r");
	struct ast_node *n;
	yyparse(&n);
	fclose(yyin);

	if (strcmp(argv[1], "ast") == 0) {
		ast_fprint(stdout, n, 0);
	} else {
		return EXIT_FAILURE;
	}
}
%}

%parse-param {struct ast_node **res}

%union {
	struct ast_node *n;
	enum ast_unary_kind u;
	enum ast_bin_kind b;
	struct vec argument_expr_list; /* struct vec<ast_node *> */
	enum ast_type_kind type_kind;
	struct ast_type type;
	int k;
}

%token ALIGNOF AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE ENUM
%token EXTERN FLOAT FOR GOTO IF INLINE INT LONG REGISTER RESTRICT RETURN SHORT
%token SIGNED SIZEOF STATIC STRUCT SWITCH TYPEDEF UNION UNSIGNED VOID VOLATILE
%token WHILE U_ALIGNAS U_ATOMIC U_BOOL U_COMPLEX U_GENERIC U_IMAGINARY
%token U_NORETURN U_STATIC_ASSERT U_THREAD_LOCAL

%token IDENT
%token INTEGER
%token STRING CHARACTER_CONSTANT
%token LSQUARE RSQUARE LCURLY RCURLY LROUND RROUND DOT ARROW INCR DECR
%token AND STAR PLUS MINUS NOT NOTB DIV MOD LSHIFT RSHIFT
%token LT GT LEQ GEQ EQB NEQ XOR OR ANDB ORB QMARK COLON SEMI ELLIPSIS EQ
%token COMMA

%type <n> identifier string_literal
%type <n> start primary_expression postfix_expression unary_expression cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression xor_expression or_expression andb_expression orb_expression conditional_expression assignment_expression expression
%type <n> declaration storage_class_specifier struct_or_union_specifier struct_or_union struct_declaration_list struct_declaration specifier_qualifier_list struct_declarator_list struct_declarator enum_specifier enumerator_list enumerator atomic_type_specifier type_qualifier function_specifier alignment_specifier type_qualifier_list parameter_type_list parameter_list parameter_declaration identifier_list type_name abstract_declarator direct_abstract_declarator typedef_name initializer initializer_list designation designator_list designator static_assertion_declaration
%type <n> statement labeled_statement compound_statement block_item_list block_item expression_statement selection_statement iteration_statement jump_statement

%type <u> unary_operator
%type <b> assigment_operator
%type <argument_expr_list> argument_expression_list
%type <type_kind> type_specifier declaration_specifiers
%type <type> init_declarator_list init_declarator direct_declarator declarator
%type <k> pointer

%start start

%%

start : compound_statement { *res = $1; } ;

string_literal : STRING { $$ = ast_string(lex_ident); } ;
identifier : IDENT { $$ = ast_ident(lex_ident); } ;
enumeration_constant : identifier ;

 /* A.2.1 EXPRESSIONS */

primary_expression : identifier
		   | INTEGER { $$ = ast_integer(lex_int); }
		   | CHARACTER_CONSTANT { $$ = ast_character_constant(lex_int); }
		   | string_literal
		   | LROUND expression RROUND { $$ = $2; }
		   ;

postfix_expression : primary_expression { $$ = $1; }
		   | postfix_expression LSQUARE expression RSQUARE { $$ = ast_index($1, $3); }
		   | postfix_expression LROUND RROUND { $$ = ast_call($1, vec_new_empty(sizeof(struct ast_node *))); }
		   | postfix_expression LROUND argument_expression_list RROUND { $$ = ast_call($1, $3); }
		   | postfix_expression DOT IDENT { $$ = ast_member($1, lex_ident); }
		   | postfix_expression ARROW IDENT { $$ = ast_member_deref($1, lex_ident); }
		   | postfix_expression INCR { $$ = ast_unary($1, AST_POST_INCR); }
		   | postfix_expression DECR { $$ = ast_unary($1, AST_POST_DECR); }
		   ;

argument_expression_list : assignment_expression { $$ = vec_new_empty(sizeof(struct ast_node *)); vec_append(&($$), &($1)); }
			 | argument_expression_list COMMA assignment_expression { $$ = $1; vec_append(&($$), &($3)); }
			 ;

unary_expression : postfix_expression { $$ = $1; }
		 | INCR unary_expression { $$ = ast_unary($2, AST_PRE_INCR); }
		 | DECR unary_expression { $$ = ast_unary($2, AST_PRE_DECR); }
		 | unary_operator cast_expression { $$ = ast_unary($2, $1); }
		 | SIZEOF unary_expression { $$ = ast_unary($2, AST_UNARY_SIZEOF); }
		 | SIZEOF LROUND type_name RROUND
		 | ALIGNOF LROUND type_name RROUND
		 ;

unary_operator : AND { $$ = AST_UNARY_REF; }
	       | STAR { $$ = AST_UNARY_DEREF; }
	       | PLUS { $$ = AST_UNARY_PLUS; }
	       | MINUS { $$ = AST_UNARY_MINUS; }
	       | NOT { $$ = AST_UNARY_NOT; }
	       | NOTB { $$ = AST_UNARY_NOTB; }
	       ;

cast_expression : unary_expression { $$ = $1; }
		| LROUND type_name RROUND cast_expression
		;

multiplicative_expression : cast_expression { $$ = $1; }
			  | multiplicative_expression STAR cast_expression { $$ = ast_bin($1, $3, AST_BIN_MUL); }
			  | multiplicative_expression DIV cast_expression { $$ = ast_bin($1, $3, AST_BIN_DIV); }
			  | multiplicative_expression MOD cast_expression { $$ = ast_bin($1, $3, AST_BIN_MOD); }
			  ;

additive_expression : multiplicative_expression { $$ = $1; }
		    | additive_expression PLUS multiplicative_expression { $$ = ast_bin($1, $3, AST_BIN_ADD); }
		    | additive_expression MINUS multiplicative_expression { $$ = ast_bin($1, $3, AST_BIN_SUB); }
		    ;

shift_expression : additive_expression { $$ = $1; }
		 | shift_expression LSHIFT additive_expression { $$ = ast_bin($1, $3, AST_BIN_LSHIFT); }
		 | shift_expression RSHIFT additive_expression { $$ = ast_bin($1, $3, AST_BIN_RSHIFT); }
		 ;

relational_expression : shift_expression { $$ = $1; }
		      | relational_expression LT shift_expression { $$ = ast_bin($1, $3, AST_BIN_LT); }
		      | relational_expression GT shift_expression { $$ = ast_bin($1, $3, AST_BIN_GT); }
		      | relational_expression LEQ shift_expression { $$ = ast_bin($1, $3, AST_BIN_LEQ); }
		      | relational_expression GEQ shift_expression { $$ = ast_bin($1, $3, AST_BIN_GEQ); }
		      ;

equality_expression : relational_expression { $$ = $1; }
		    | equality_expression EQB relational_expression { $$ = ast_bin($1, $3, AST_BIN_EQB); }
		    | equality_expression NEQ relational_expression { $$ = ast_bin($1, $3, AST_BIN_NEQ); }
		    ;

and_expression : equality_expression { $$ = $1; }
	       | and_expression AND equality_expression { $$ = ast_bin($1, $3, AST_BIN_AND); }
	       ;

xor_expression : and_expression { $$ = $1; }
	       | xor_expression XOR and_expression { $$ = ast_bin($1, $3, AST_BIN_XOR); }
	       ;

or_expression : xor_expression { $$ = $1; }
	      | or_expression OR xor_expression { $$ = ast_bin($1, $3, AST_BIN_OR); }
	      ;

andb_expression : or_expression { $$ = $1; }
		| andb_expression ANDB or_expression { $$ = ast_bin($1, $3, AST_BIN_ANDB); }
		;

orb_expression : andb_expression { $$ = $1; }
	       | orb_expression ORB andb_expression { $$ = ast_bin($1, $3, AST_BIN_ORB); }
	       ;

conditional_expression : orb_expression { $$ = $1; }
		       | orb_expression QMARK expression COLON conditional_expression
		       ;

assignment_expression : conditional_expression { $$ = $1; }
		      | unary_expression assigment_operator assignment_expression { $$ = ast_bin($1, $3, $2); }
		      ;

assigment_operator : EQ { $$ = AST_BIN_ASSIGN; }
		   ;

expression : assignment_expression { $$ = $1; }
	   | expression COMMA assignment_expression { $$ = ast_bin($1, $3, AST_BIN_COMMA); }
	   ;

constant_expression : conditional_expression
		    ;

 /* A.2.2 DECLARATIONS */

declaration : declaration_specifiers SEMI
	    | declaration_specifiers init_declarator_list SEMI { $2.kind = $1; $$ = ast_decl($2); }
	    | static_assertion_declaration
	    ;

declaration_specifiers : storage_class_specifier
		       | storage_class_specifier declaration_specifiers
		       | type_specifier { $$ = $1; }
		       | type_specifier declaration_specifiers
		       | type_qualifier
		       | type_qualifier declaration_specifiers
		       | function_specifier
		       | function_specifier declaration_specifiers
		       | alignment_specifier
		       | alignment_specifier declaration_specifiers
		       ;

init_declarator_list : init_declarator
		     // | init_declarator_list COMMA init_declarator
		     ;

init_declarator : declarator { $$ = $1; }
		// | declarator EQ initializer
		;

storage_class_specifier : TYPEDEF | EXTERN | STATIC | U_THREAD_LOCAL | AUTO | REGISTER
			;

type_specifier : VOID
	       | CHAR { $$ = AST_TYPE_CHAR; }
	       | SHORT
	       | INT { $$ = AST_TYPE_INT; }
	       | LONG
	       | FLOAT
	       | DOUBLE
	       | SIGNED
	       | UNSIGNED
	       | U_BOOL
	       | U_COMPLEX
	       | atomic_type_specifier
	       | struct_or_union_specifier
	       | enum_specifier
	       // | typedef_name
	       ;

struct_or_union_specifier : struct_or_union LCURLY struct_declaration_list RCURLY
			  | struct_or_union identifier LCURLY struct_declaration_list RCURLY
			  | struct_or_union identifier
			  ;

struct_or_union : STRUCT | UNION
		;

struct_declaration_list : struct_declaration
			| struct_declaration_list struct_declaration
			;

struct_declaration : specifier_qualifier_list SEMI
		   | specifier_qualifier_list struct_declarator_list SEMI
		   | static_assertion_declaration
		   ;

specifier_qualifier_list : type_specifier
			 | type_specifier specifier_qualifier_list
			 | type_qualifier specifier_qualifier_list
			 ;

struct_declarator_list : struct_declarator
		       | struct_declarator_list COMMA struct_declarator
		       ;

struct_declarator : declarator
		  | COLON constant_expression
		  | declarator COLON constant_expression
		  ;

enum_specifier : ENUM LCURLY enumerator_list RCURLY
	       | ENUM identifier LCURLY enumerator_list RCURLY
	       | ENUM LCURLY enumerator_list COMMA RCURLY
	       | ENUM identifier LCURLY enumerator_list COMMA RCURLY
	       | ENUM identifier
	       ;

enumerator_list : enumerator
		| enumerator_list COMMA enumerator
		;

enumerator : enumeration_constant
	   | enumeration_constant EQ constant_expression
	   ;

atomic_type_specifier : U_ATOMIC LROUND type_name RROUND
		      ;

type_qualifier : CONST | RESTRICT | VOLATILE | U_ATOMIC
	       ;

function_specifier : INLINE | U_NORETURN
		   ;

alignment_specifier : U_ALIGNAS LROUND type_name RROUND
		   | U_ALIGNAS LROUND constant_expression RROUND
		   ;

declarator : direct_declarator { $$ = $1; }
	   | pointer direct_declarator { $$ = $2; ($$).pointer = $1; }
	   ;

direct_declarator : identifier { $$ = (struct ast_type){ .ident = $1 }; }
		  | LROUND identifier RROUND { $$ = (struct ast_type){ .ident = $2 }; }
		  ;
direct_declarator : // direct_declarator LSQUARE RSQUARE
		  // | direct_declarator LSQUARE type_qualifier_list RSQUARE
		  | direct_declarator LSQUARE assignment_expression RSQUARE { $$ = $1; ($$).array = $3; }
		  | direct_declarator LSQUARE type_qualifier_list assignment_expression RSQUARE { $$ = $1; ($$).array = $4; }
		  ;
 /* direct_declarator : direct_declarator LSQUARE STATIC assignment_expression RSQUARE
		  | direct_declarator LSQUARE STATIC type_qualifier_list assignment_expression RSQUARE
		  ;
direct_declarator : direct_declarator LSQUARE type_qualifier_list STATIC assignment_expression RSQUARE
		  ;
direct_declarator : direct_declarator LSQUARE STAR RSQUARE
		  | direct_declarator LSQUARE type_qualifier_list STAR RSQUARE
		  ;
direct_declarator : direct_declarator LROUND parameter_type_list RROUND
		  ;
direct_declarator : direct_declarator LROUND RROUND
		  | direct_declarator LROUND identifier_list RROUND
		  ; */

pointer : STAR { $$ = 1; }
	| STAR type_qualifier_list { $$ = 1; }
	| STAR pointer { $$ = $2 + 1; }
	| STAR type_qualifier_list pointer { $$ = $3; }
	;

type_qualifier_list : type_qualifier
		    | type_qualifier_list type_qualifier
		    ;

parameter_type_list : parameter_list
		    | parameter_list COMMA ELLIPSIS
		    ;

parameter_list : parameter_declaration
	       | parameter_list COMMA parameter_declaration
	       ;

parameter_declaration : declaration_specifiers declarator
		      | declaration_specifiers
		      | declaration_specifiers abstract_declarator
		      ;

identifier_list : identifier
		| identifier_list COMMA identifier
		;

type_name : specifier_qualifier_list
	  | specifier_qualifier_list abstract_declarator
	  ;

abstract_declarator : pointer
		    | direct_abstract_declarator
		    | pointer direct_abstract_declarator
		    ;

opt_direct_astract_declarator : | direct_abstract_declarator ;
opt_type_qualifier_list : | type_qualifier_list ;
opt_assignment_expression : | assignment_expression ;
opt_parameter_type_list : | parameter_type_list ;
direct_abstract_declarator : LROUND abstract_declarator RROUND
			   | opt_direct_astract_declarator LSQUARE opt_type_qualifier_list opt_assignment_expression RSQUARE
			   | opt_direct_astract_declarator LSQUARE STATIC opt_type_qualifier_list assignment_expression RSQUARE
			   | opt_direct_astract_declarator LSQUARE type_qualifier_list STATIC assignment_expression RSQUARE
			   | opt_direct_astract_declarator LSQUARE STAR RSQUARE
			   | opt_direct_astract_declarator LROUND opt_parameter_type_list RROUND
			   ;

typedef_name : identifier
	     ;

initializer : assignment_expression
	    | LCURLY initializer_list RCURLY
	    | LCURLY initializer_list COMMA RCURLY
	    ;

opt_designation : | designation ;
initializer_list : opt_designation initializer
		 | initializer_list COMMA opt_designation initializer
		 ;

designation : designator_list EQ
	    ;

designator_list : designator
		| designator_list designator
		;

designator : LSQUARE constant_expression RSQUARE
	   | DOT identifier
	   ;

static_assertion_declaration : U_STATIC_ASSERT LROUND constant_expression COMMA string_literal RROUND SEMI
			     ;

 /* A.2.3 STATEMENTS */

statement : labeled_statement
	  | compound_statement
	  | expression_statement
	  | selection_statement
	  | iteration_statement
	  | jump_statement
	  ;

labeled_statement : identifier COLON statement
		  | CASE constant_expression COLON statement
		  | DEFAULT COLON statement
		  ;

compound_statement : LCURLY RCURLY { $$ = ast_stmt_comp(); }
		   | LCURLY block_item_list RCURLY { $$ = $2; }
		   ;

block_item_list : block_item { $$ = ast_stmt_comp(); vec_append(&($$)->stmt_comp, &($1)); }
		| block_item_list block_item { vec_append(&($1)->stmt_comp, &($2)); }
		;

block_item : declaration
	   | statement
	   ;

expression_statement : SEMI { $$ = ast_stmt_expr(NULL); }
		     | expression SEMI { $$ = ast_stmt_expr($1); }
		     ;

selection_statement : IF LROUND expression RROUND statement { $$ = ast_stmt_if($3, $5); }
		    | IF LROUND expression RROUND statement ELSE statement
		    | SWITCH LROUND expression RROUND statement
		    ;

opt_expression : | expression ;
iteration_statement : WHILE LROUND expression RROUND statement { $$ = ast_stmt_while($3, $5); }
		    | DO statement WHILE LROUND expression RROUND SEMI
		    | FOR LROUND opt_expression SEMI opt_expression SEMI opt_expression RROUND statement
		    | FOR LROUND declaration opt_expression SEMI opt_expression RROUND statement
		    ;

jump_statement : GOTO identifier SEMI
	       | CONTINUE SEMI
	       | BREAK SEMI
	       | RETURN opt_expression SEMI
	       ;
