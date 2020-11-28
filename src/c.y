 // SPDX-License-Identifier: GPL-3.0-only
%{
#include <stdio.h>
#include <c_compiler/ast.h>
#include <c_compiler/cg.h>
#include <c.tab.h>

// typedef struct ast_node *YYSTYPE;

const char *lex_ident;
long long int lex_int;

extern int yylex();
extern FILE *yyin;

void yyerror(struct ast_node **res, const char *str)
{
	(void)res;
	fprintf(stderr, "error: %s; line: %d\n", str, yylloc.first_line);
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
	} else if (strcmp(argv[1], "asm") == 0) {
		return cg_gen(n);
	} else {
		return EXIT_FAILURE;
	}
}
%}

%locations
%define parse.error verbose

%parse-param {struct ast_node **res}

%union {
	struct ast_node *n;
	enum ast_unary_kind u;
	enum ast_bin_kind b;
	struct vec argument_expr_list; /* struct vec<ast_node *> */
	int integer;
	struct vec list; /* struct vec<ast_node *> */
	enum ast_storage_class_specifier storage_class_specifier;
	enum ast_builtin_type builtin_type;
	enum ast_type_qualifier type_qualifier;
	enum ast_function_specifier function_specifier;
	enum ast_su struct_or_union;
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

%type <n> identifier string_literal enumeration_constant
%type <n> start primary_expression postfix_expression unary_expression cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression xor_expression or_expression andb_expression orb_expression conditional_expression assignment_expression expression constant_expression opt_assignment_expression
%type <n> declaration struct_or_union_specifier specifier_qualifier_list struct_declarator enum_specifier enumerator alignment_specifier type_name abstract_declarator opt_abstract_declarator direct_abstract_declarator  opt_direct_astract_declarator designation designator static_assertion_declaration opt_designation initializer_list_item
%type <n> statement labeled_statement compound_statement block_item_list block_item expression_statement selection_statement iteration_statement jump_statement opt_expression
%type <n> translation_unit external_declaration function_definition

%type <u> unary_operator
%type <b> assigment_operator
%type <argument_expr_list> argument_expression_list
%type <n> type_specifier
%type <n> direct_declarator declarator
%type <integer> pointer

%type <n> init_declarator initializer parameter_declaration struct_declaration
%type <list> init_declarator_list parameter_type_list opt_parameter_type_list parameter_list struct_declaration_list struct_declarator_list enumerator_list designator_list initializer_list

%type <storage_class_specifier> storage_class_specifier
%type <builtin_type> builtin_type
%type <type_qualifier> type_qualifier
%type <function_specifier> function_specifier
%type <struct_or_union> struct_or_union

%type <n> declaration_specifiers

%type <n> opt_identifier

%start start

%%

start : translation_unit { *res = $1; } ;

opt_comma : | COMMA ;
opt_identifier : { $$ = NULL; } | identifier { $$ = $1; } ;
opt_abstract_declarator : { $$ = NULL; } | abstract_declarator { $$ = $1; } ;
opt_direct_astract_declarator : { $$ = NULL; } | direct_abstract_declarator { $$ = $1; };
opt_type_qualifier_list : | type_qualifier_list ;
opt_assignment_expression : { $$ = NULL; } | assignment_expression { $$ = $1; };
opt_parameter_type_list : { $$ = vec_new_empty(sizeof(struct ast_node *)); }
			| parameter_type_list { $$ = $1; };

string_literal : STRING { $$ = ast_string(lex_ident); } ;
identifier : IDENT { $$ = ast_ident(lex_ident); } ;
enumeration_constant : identifier { $$ = $1; } ;

 /* A.2.1 EXPRESSIONS */

primary_expression : identifier
		   | INTEGER { $$ = ast_integer(lex_int); }
		   | CHARACTER_CONSTANT { $$ = ast_character_constant(lex_int); }
		   | string_literal
		   | LROUND expression RROUND { $$ = $2; }
		   ;

 /* TODO: generic selection */
 /* generic_selection : U_GENERIC LROUND assignment_expression COMMA generic_assoc_list
		  ;
generic_assoc_list : generic_association
		   | generic_assoc_list generic_association
		   ;
generic_association : type_name COLON assignment_expression
		    | DEFAULT COLON assignment_expression
		    ; */

postfix_expression : primary_expression { $$ = $1; }
		   | postfix_expression LSQUARE expression RSQUARE { $$ = ast_index($1, $3); }
		   | postfix_expression LROUND RROUND { $$ = ast_call($1, vec_new_empty(sizeof(struct ast_node *))); }
		   | postfix_expression LROUND argument_expression_list RROUND { $$ = ast_call($1, $3); }
		   | postfix_expression DOT IDENT { $$ = ast_member($1, lex_ident); }
		   | postfix_expression ARROW IDENT { $$ = ast_member_deref($1, lex_ident); }
		   | postfix_expression INCR { $$ = ast_unary($1, AST_POST_INCR); }
		   | postfix_expression DECR { $$ = ast_unary($1, AST_POST_DECR); }
		   | LROUND type_name RROUND LCURLY initializer_list opt_comma
		     RCURLY {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_COMPOUND_LITERAL,
		.compound_literal = {
			.type_name = $2,
			.list = $5
		}
	}); }
		   ;

argument_expression_list : assignment_expression { $$ = vec_new_empty(sizeof(struct ast_node *)); vec_append(&($$), &($1)); }
			 | argument_expression_list COMMA assignment_expression { $$ = $1; vec_append(&($$), &($3)); }
			 ;

unary_expression : postfix_expression { $$ = $1; }
		 | INCR unary_expression { $$ = ast_unary($2, AST_PRE_INCR); }
		 | DECR unary_expression { $$ = ast_unary($2, AST_PRE_DECR); }
		 | unary_operator cast_expression { $$ = ast_unary($2, $1); }
		 | SIZEOF unary_expression { $$ = ast_unary($2, AST_UNARY_SIZEOF); }
		 | SIZEOF LROUND type_name RROUND {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_SIZEOF_EXPR,
		.sizeof_expr.type_name = $3
	}); }
		 | ALIGNOF LROUND type_name RROUND {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_ALIGNOF_EXPR,
		.alignof_expr.type_name = $3
	}); }
		 ;

unary_operator : AND { $$ = AST_UNARY_REF; }
	       | STAR { $$ = AST_UNARY_DEREF; }
	       | PLUS { $$ = AST_UNARY_PLUS; }
	       | MINUS { $$ = AST_UNARY_MINUS; }
	       | NOT { $$ = AST_UNARY_NOT; }
	       | NOTB { $$ = AST_UNARY_NOTB; }
	       ;

cast_expression : unary_expression { $$ = $1; }
		| LROUND type_name RROUND cast_expression {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_CAST,
		.cast = { .type_name = $2, .expr = $4 }
	}); }
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
		       | orb_expression QMARK expression
		         COLON conditional_expression {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_CONDITIONAL,
		.conditional = { .cond = $1, .expr = $3, .expr_else = $5 }
	}); }
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

declaration : declaration_specifiers SEMI { $$ = ast_declaration($1, vec_new_empty(sizeof(struct ast_node *))); }
	    | declaration_specifiers init_declarator_list SEMI { $$ = ast_declaration($1, $2); }
	    | static_assertion_declaration { $$ = $1; }
	    ;

declaration_specifiers : storage_class_specifier { $$ = ast_declaration_specifiers(); ($$)->declaration_specifiers.storage_class_specifiers[($1)]++; }
		       | storage_class_specifier declaration_specifiers { ($2)->declaration_specifiers.storage_class_specifiers[($1)]++; $$ = $2; }
		       | type_specifier { $$ = ast_declaration_specifiers(); vec_append(&($$)->declaration_specifiers.type_specifiers, &($1)); }
		       | type_specifier declaration_specifiers { vec_append(&($2)->declaration_specifiers.type_specifiers, &($1)); $$ = $2; }
		       | type_qualifier { $$ = ast_declaration_specifiers(); ($$)->declaration_specifiers.type_qualifiers[($1)]++; }
		       | type_qualifier declaration_specifiers { ($2)->declaration_specifiers.type_qualifiers[($1)]++; $$ = $2; }
		       | function_specifier { $$ = ast_declaration_specifiers(); ($$)->declaration_specifiers.function_specifiers[($1)]++; }
		       | function_specifier declaration_specifiers { ($2)->declaration_specifiers.function_specifiers[($1)]++; $$ = $2; }
		       | alignment_specifier { $$ = ast_declaration_specifiers(); vec_append(&($$)->declaration_specifiers.alignment_specifiers, &($1)); }
		       | alignment_specifier declaration_specifiers { vec_append(&($2)->declaration_specifiers.alignment_specifiers, &($1)); $$ = $2; }
		       ;

init_declarator_list : init_declarator { $$ = ast_list($1); }
		     | init_declarator_list COMMA init_declarator { vec_append(&($1), &($3)); $$ = $1; }
		     ;

init_declarator : declarator { $$ = ast_init_declarator($1, NULL); }
		| declarator EQ initializer { $$ = ast_init_declarator($1, $3); }
		;

storage_class_specifier : TYPEDEF { $$ = AST_STORAGE_CLASS_SPECIFIER_TYPEDEF; }
			| EXTERN { $$ = AST_STORAGE_CLASS_SPECIFIER_EXTERN; }
			| STATIC { $$ = AST_STORAGE_CLASS_SPECIFIER_STATIC; }
			| U_THREAD_LOCAL { $$ = AST_STORAGE_CLASS_SPECIFIER_THREAD_LOCAL; }
			| AUTO { $$ = AST_STORAGE_CLASS_SPECIFIER_AUTO; }
			| REGISTER { $$ = AST_STORAGE_CLASS_SPECIFIER_REGISTER; }
			;

builtin_type : VOID { $$ = AST_BUILTIN_TYPE_VOID; }
	     | CHAR { $$ = AST_BUILTIN_TYPE_CHAR; }
	     | SHORT { $$ = AST_BUILTIN_TYPE_SHORT; }
	     | INT { $$ = AST_BUILTIN_TYPE_INT; }
	     | LONG { $$ = AST_BUILTIN_TYPE_LONG; }
	     | FLOAT { $$ = AST_BUILTIN_TYPE_FLOAT; }
	     | DOUBLE { $$ = AST_BUILTIN_TYPE_DOUBLE; }
	     | SIGNED { $$ = AST_BUILTIN_TYPE_SIGNED; }
	     | UNSIGNED { $$ = AST_BUILTIN_TYPE_UNSIGNED; }
	     | U_BOOL { $$ = AST_BUILTIN_TYPE_BOOL; }
	     | U_COMPLEX { $$ = AST_BUILTIN_TYPE_COMPLEX; }
	     ;
type_specifier : builtin_type { $$ = ast_builtin_type($1); }
	       // | atomic_type_specifier // TODO: this is not context free
	       | struct_or_union_specifier
	       | enum_specifier
	       // | typedef_name { $$ = $1; } // TODO: this is not context free
	       ;

struct_or_union_specifier : struct_or_union LCURLY
			    struct_declaration_list RCURLY {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_SU_SPECIFIER,
		.su_specifier = { .su = $1, .ident = NULL, .declarations = $3 }
	}); }
			  | struct_or_union identifier LCURLY
			    struct_declaration_list RCURLY {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_SU_SPECIFIER,
		.su_specifier = { .su = $1, .ident = $2, .declarations = $4 }
	}); }
			  | struct_or_union identifier {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_SU_SPECIFIER_INCOMPLETE,
		.su_specifier = { .su = $1, .ident = $2 }
	}); }
			  ;

struct_or_union : STRUCT { $$ = AST_SU_STRUCT; }
		| UNION { $$ = AST_SU_UNION; }
		;

struct_declaration_list : struct_declaration { $$ = ast_list($1); }
			| struct_declaration_list struct_declaration { vec_append(&($1), &($2)); $$ = $1; }
			;

struct_declaration : specifier_qualifier_list SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STRUCT_DECLARATION,
		.struct_declaration = {
			.specifier_qualifier_list = $1,
			.declarators = vec_new_empty(sizeof(struct ast_node *))
		}
	}); }
		   | specifier_qualifier_list struct_declarator_list SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STRUCT_DECLARATION,
		.struct_declaration = {
			.specifier_qualifier_list = $1,
			.declarators = $2
		}
	}); }
		   | static_assertion_declaration { $$ = $1; }
		   ;

specifier_qualifier_list : type_specifier {
	$$ = ast_specifier_qualifier_list();
	vec_append(&($$)->specifier_qualifier_list.type_specifiers, &($1)); }
			 | type_specifier specifier_qualifier_list {
	vec_append(&($2)->specifier_qualifier_list.type_specifiers, &($1));
	$$ = $2; }
			 | type_qualifier specifier_qualifier_list {
	($2)->declaration_specifiers.type_qualifiers[($1)]++;
	$$ = $2; }
			 ;

struct_declarator_list : struct_declarator { $$ = ast_list($1); }
		       | struct_declarator_list COMMA struct_declarator {
	vec_append(&($1), &($3));
	$$ = $1; }
		       ;

struct_declarator : declarator {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STRUCT_DECLARATOR,
		.struct_declarator = { .declarator = $1, .bitfield_expr = NULL }
	}); }
		  | COLON constant_expression {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STRUCT_DECLARATOR,
		.struct_declarator = { .declarator = NULL, .bitfield_expr = $2 }
	}); }
		  | declarator COLON constant_expression {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STRUCT_DECLARATOR,
		.struct_declarator = { .declarator = $1, .bitfield_expr = $3 }
	}); }
		  ;

enum_specifier : ENUM opt_identifier LCURLY enumerator_list opt_comma RCURLY {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_ENUM_SPECIFIER,
		.enum_specifier = { .ident = $2, .enumerators = $4 }
	}); }
	       | ENUM identifier {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_ENUM_SPECIFIER_INCOMPLETE,
		.enum_specifier_incomplete = { .ident = $2 }
	}); }
	       ;

enumerator_list : enumerator { $$ = ast_list($1); }
		| enumerator_list COMMA enumerator {
	vec_append(&($1), &($3));
	$$ = $1; }
		;

enumerator : enumeration_constant {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_ENUMERATOR,
		.enumerator = { .ident = $1 }
	}); }
	   | enumeration_constant EQ constant_expression {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_ENUMERATOR,
		.enumerator = { .ident = $1, .expr = $3 }
	}); }
	   ;

 // TODO: this is not context free
 // atomic_type_specifier : U_ATOMIC LROUND type_name RROUND ;

type_qualifier : CONST { $$ = AST_TYPE_QUALIFIER_CONST; }
	       | RESTRICT { $$ = AST_TYPE_QUALIFIER_RESTRICT; }
	       | VOLATILE { $$ = AST_TYPE_QUALIFIER_VOLATILE; }
	       | U_ATOMIC { $$ = AST_TYPE_QUALIFIER_ATOMIC; }
	       ;

function_specifier : INLINE { $$ = AST_FUNCTION_SPECIFIER_INLINE; }
		   | U_NORETURN { $$ = AST_FUNCTION_SPECIFIER_NORETURN; }
		   ;

// alignment_specifier : U_ALIGNAS LROUND type_name RROUND ;
alignment_specifier : U_ALIGNAS LROUND constant_expression RROUND { $$ = ast_alloc((struct ast_node){ .kind = AST_ALIGNMENT_SPECIFIER, .alignment_specifier.expr = $3 }); }
		    ;

declarator : direct_declarator { $$ = ast_declarator(0, $1); }
	   | pointer direct_declarator { $$ = ast_declarator($1, $2); }
	   ;

direct_declarator : identifier { $$ = $1; }
		  | LROUND declarator RROUND { $$ = $2; }
		  | direct_declarator LSQUARE opt_type_qualifier_list
		    opt_assignment_expression
		    RSQUARE { $$ = ast_array_declarator($1, $4); }
		  | direct_declarator LSQUARE STATIC opt_type_qualifier_list
		    assignment_expression
		    RSQUARE { $$ = ast_array_declarator($1, $5); }
		  | direct_declarator LSQUARE type_qualifier_list STATIC
		    assignment_expression
		    RSQUARE { $$ = ast_array_declarator($1, $5); }
		  | direct_declarator LSQUARE opt_type_qualifier_list STAR
		    RSQUARE { $$ = ast_array_declarator($1, NULL); }
		  | direct_declarator LROUND opt_parameter_type_list
		    RROUND { $$ = ast_function_declarator($1, $3); }
		  // | direct_declarator LROUND opt_identifier_list RROUND
		  ;
 /* TODO: this is K&R style */
/* direct_declarator : direct_declarator LROUND identifier_list RROUND
		  ; */

pointer : STAR { $$ = 1; }
	| STAR type_qualifier_list { $$ = 1; }
	| STAR pointer { $$ = $2 + 1; }
	| STAR type_qualifier_list pointer { $$ = $3; }
	;

type_qualifier_list : type_qualifier
		    | type_qualifier_list type_qualifier
		    ;

parameter_type_list : parameter_list { $$ = $1; }
		    | parameter_list COMMA ELLIPSIS { $$ = $1; } // TODO
		    ;

parameter_list : parameter_declaration { $$ = ast_list($1); }
	       | parameter_list COMMA parameter_declaration { vec_append(&($1), &($3)); $$ = $1; }
	       ;

parameter_declaration : declaration_specifiers declarator { $$ = ast_parameter_declaration($2, $1); }
		      | declaration_specifiers { $$ = ast_parameter_declaration(NULL, $1); }
		      | declaration_specifiers abstract_declarator { $$ = ast_parameter_declaration($2, $1); }
		      ;

 /* identifier_list : identifier
		| identifier_list COMMA identifier
		; */

type_name : specifier_qualifier_list opt_abstract_declarator {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_TYPE_NAME,
		.type_name = {
			.specifier_qualifier_list = $1,
			.declarator = $2
		}
	}); }
	  ;

abstract_declarator : pointer { $$ = ast_declarator($1, NULL); }
		    | direct_abstract_declarator { $$ = ast_declarator(0, $1); }
		    | pointer direct_abstract_declarator
		      { $$ = ast_declarator($1, $2); }
		    ;

direct_abstract_declarator : LROUND abstract_declarator RROUND { $$ = $2; }
			   | opt_direct_astract_declarator LSQUARE
			     opt_type_qualifier_list opt_assignment_expression
			     RSQUARE { $$ = ast_array_declarator($1, $4); }
			   | opt_direct_astract_declarator LSQUARE STATIC
			     opt_type_qualifier_list assignment_expression
			     RSQUARE { $$ = ast_array_declarator($1, $5); }
			   | opt_direct_astract_declarator LSQUARE
			     type_qualifier_list STATIC assignment_expression
			     RSQUARE { $$ = ast_array_declarator($1, $5); }
			   | opt_direct_astract_declarator LSQUARE STAR
			     RSQUARE { $$ = ast_array_declarator($1, NULL); }
			   | opt_direct_astract_declarator LROUND
			     opt_parameter_type_list
			     RROUND { $$ = ast_function_declarator($1, $3); }
			   ;

 // TODO: this is not context free
 // typedef_name : identifier { $$ = $1; } ;

initializer : assignment_expression { $$ = $1; }
	    | LCURLY initializer_list opt_comma RCURLY {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_INITIALIZER,
		.initializer = { .list = $2 }
	}); }
	    ;

opt_designation : { $$ = NULL; } | designation { $$ = $1; };
initializer_list_item : opt_designation initializer {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_INITIALIZER_LIST_ITEM,
		.initializer_list_item = {
			.designation = $1,
			.initializer = $2,
		}
	}); }
		      ;
initializer_list : initializer_list_item { $$ = ast_list($1); }
		 | initializer_list COMMA initializer_list_item {
	vec_append(&($1), &($3));
	$$ = $1; }
		 ;

designation : designator_list EQ {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_DESIGNATION,
		.designation = $1
	}); }
	    ;

designator_list : designator { $$ = ast_list($1); }
		| designator_list designator {
	vec_append(&($1), &($2));
	$$ = $1; }
		;

designator : LSQUARE constant_expression RSQUARE {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_DESIGNATOR_INDEX,
		.designator_index = $2
	}); }
	   | DOT identifier {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_DESIGNATOR_IDENT,
		.designator_ident = $2
	}); }
	   ;

static_assertion_declaration : U_STATIC_ASSERT LROUND constant_expression
			       COMMA string_literal RROUND SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STATIC_ASSERT,
		.static_assert_ = { .cond = $3, .message = $5 }
	}); }
			     ;

 /* A.2.3 STATEMENTS */

statement : labeled_statement { $$ = $1; }
	  | compound_statement { $$ = $1; }
	  | expression_statement { $$ = $1; }
	  | selection_statement { $$ = $1; }
	  | iteration_statement { $$ = $1; }
	  | jump_statement { $$ = $1; }
	  ;

labeled_statement : identifier COLON statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_LABELED,
		.stmt_labeled = { .ident = $1, .stmt = $3 }
	}); }
		  | CASE constant_expression COLON statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_LABELED_CASE,
		.stmt_labeled_case = { .expr = $2, .stmt = $4 }
	}); }
		  | DEFAULT COLON statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_LABELED_DEFAULT,
		.stmt_labeled_default = { .stmt = $3 }
	}); }
		  ;

compound_statement : LCURLY RCURLY { $$ = ast_stmt_comp(); }
		   | LCURLY block_item_list RCURLY { $$ = $2; }
		   ;

block_item_list : block_item { $$ = ast_stmt_comp(); vec_append(&($$)->stmt_comp, &($1)); }
		| block_item_list block_item { vec_append(&($1)->stmt_comp, &($2)); }
		;

block_item : declaration { $$ = $1; }
	   | statement { $$ = $1; }
	   ;

expression_statement : SEMI { $$ = ast_stmt_expr(NULL); }
		     | expression SEMI { $$ = ast_stmt_expr($1); }
		     ;

selection_statement : IF LROUND expression RROUND statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_IF,
		.stmt_if = { .cond = $3, .stmt = $5 }
	}); }
		    | IF LROUND expression RROUND statement ELSE statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_IF,
		.stmt_if = { .cond = $3, .stmt = $5, .stmt_else = $7 }
	}); }
		    | SWITCH LROUND expression RROUND statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_SWITCH,
		.stmt_switch = { .cond = $3, .stmt = $5 }
	}); }
		    ;

opt_expression : { $$ = NULL; } | expression { $$ = $1; } ;
iteration_statement : WHILE LROUND expression RROUND statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_WHILE,
		.stmt_while = { .cond = $3, .stmt = $5 }
	}); }
		    | DO statement WHILE LROUND expression RROUND SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_DO_WHILE,
		.stmt_do_while = { .cond = $5, .stmt = $2 }
	}); }
		    | FOR LROUND opt_expression SEMI opt_expression SEMI
		      opt_expression RROUND statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_FOR,
		.stmt_for = { .a = $3, .b = $5, .c = $7, .stmt = $9 }
	}); }
		    | FOR LROUND declaration opt_expression SEMI
		      opt_expression RROUND statement {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_FOR,
		.stmt_for = { .a = $3, .b = $4, .c = $6, .stmt = $8 }
	}); }
		    ;

jump_statement : GOTO identifier SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_GOTO,
		.stmt_goto = { .ident = $2 }
	}); }
	       | CONTINUE SEMI {
	$$ = ast_alloc((struct ast_node){ .kind = AST_STMT_CONTINUE }); }
	       | BREAK SEMI {
	$$ = ast_alloc((struct ast_node){ .kind = AST_STMT_BREAK }); }
	       | RETURN opt_expression SEMI {
	$$ = ast_alloc((struct ast_node){
		.kind = AST_STMT_RETURN,
		.stmt_return = { .expr = $2 }
	}); }
	       ;

 /* A.2.4 EXTERNAL DEFINITIONS */
translation_unit : external_declaration { $$ = ast_translation_unit($1); }
		 | translation_unit external_declaration { vec_append(&($1)->translation_unit, &($2)); $$ = $1; }
		 ;

external_declaration : function_definition { $$ = $1; }
		     | declaration { $$ = $1; }
		     ;

function_definition : declaration_specifiers declarator compound_statement { $$ = ast_function_definition($1, $2, $3); }
		    ;

 /*TODO: this is the K&R style, it's not important to support it right now...*/
 /* opt_declaration_list : | declaration_list ;
function_definition : declaration_specifiers declarator opt_declaration_list compound_statement ;
declaration_list : declaration
		 | declaration_list declaration
		 ; */
