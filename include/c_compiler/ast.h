// SPDX-License-Identifier: GPL-3.0-only
#ifndef C_COMPILER_AST_H
#define C_COMPILER_AST_H
#include <stdlib.h>
#include <string.h>
#include <ds/vec.h>

enum ast_kind {
	AST_IDENT,
	AST_INTEGER,
	AST_CHARACTER_CONSTANT,
	AST_STRING,
	AST_INDEX,
	AST_MEMBER,
	AST_MEMBER_DEREF,
	AST_UNARY,
	AST_BIN,
	AST_STMT_EXPR,
	AST_STMT_COMP,
	AST_STMT_WHILE,
	AST_STMT_IF,
	AST_CALL,
	AST_DECLARATION,
	AST_INIT_DECLARATOR,
	AST_DECLARATOR,
	AST_ARRAY_DECLARATOR,
	AST_FUNCTION_DECLARATOR,
	AST_PARAMETER_DECLARATION,
	AST_TRANSLATION_UNIT,
	AST_FUNCTION_DEFINITION,
};

enum ast_unary_kind {
	AST_PRE_INCR,
	AST_PRE_DECR,
	AST_POST_INCR,
	AST_POST_DECR,
	AST_UNARY_REF,
	AST_UNARY_DEREF,
	AST_UNARY_PLUS,
	AST_UNARY_MINUS,
	AST_UNARY_NOT,
	AST_UNARY_NOTB,
	AST_UNARY_SIZEOF
};

enum ast_bin_kind {
	AST_BIN_MUL,
	AST_BIN_DIV,
	AST_BIN_MOD,
	AST_BIN_ADD,
	AST_BIN_SUB,
	AST_BIN_LSHIFT,
	AST_BIN_RSHIFT,
	AST_BIN_LT,
	AST_BIN_GT,
	AST_BIN_LEQ,
	AST_BIN_GEQ,
	AST_BIN_EQB,
	AST_BIN_NEQ,
	AST_BIN_AND,
	AST_BIN_XOR,
	AST_BIN_OR,
	AST_BIN_ANDB,
	AST_BIN_ORB,
	AST_BIN_ASSIGN,
	AST_BIN_COMMA,
};

enum ast_type_kind {
	AST_TYPE_INT,
	AST_TYPE_CHAR
};

struct ast_type {
	enum ast_type_kind kind;
	int pointer;
	struct ast_node *array;
	struct ast_node *ident;
};


struct ast_node {
	enum ast_kind kind;
	union {
		char *ident;
		long long int integer;
		int character_constant;
		const char *string;
		struct {
			struct ast_node *a, *b; // a[b]
		} index;
		struct { struct ast_node *a; char *ident; } member;
		struct { struct ast_node *a; enum ast_unary_kind kind; } unary;
		struct { struct ast_node *a, *b; enum ast_bin_kind kind; } bin;
		struct { struct ast_node *a; } stmt_expr;
		struct vec stmt_comp; /* vec<struct ast_node *> */
		struct { struct ast_node *a, *b; } stmt_while;
		struct { struct ast_node *a, *b; } stmt_if;
		struct { struct ast_node *a; struct vec args; } call;
		struct {
			enum ast_type_kind declaration_specifiers;
			struct vec init_declarator_list;
		} declaration;
		struct {
			struct ast_node *declarator, *initializer;
		} init_declarator;
		struct {
			int pointer;
			struct ast_node *direct_declarator;
		} declarator;
		struct {
			struct ast_node *direct_declarator;
			struct ast_node *size;
		} array_declarator;
		struct {
			struct ast_node *direct_declarator;
			struct vec parameter_type_list;
		} function_declarator;
		struct {
			enum ast_type_kind declaration_specifiers;
			struct ast_node *declarator;
		} parameter_declaration;
		struct vec translation_unit; /* vec<struct ast_node *> */
		struct {
			enum ast_type_kind declaration_specifiers;
			struct ast_node *declarator;
			struct ast_node *compound_statement;
		} function_definition;
	};
};

void ast_fprint(FILE *f, const struct ast_node *n, int ind);
struct ast_node *ast_ident(const char *ident);
struct ast_node *ast_integer(long long int integer);
struct ast_node *ast_character_constant(int integer);
struct ast_node *ast_string(const char *ident);
struct ast_node *ast_index(struct ast_node *a, struct ast_node *b);
struct ast_node *ast_member(struct ast_node *a, const char *ident);
struct ast_node *ast_member_deref(struct ast_node *a, const char *ident);
struct ast_node *ast_unary(struct ast_node *a, enum ast_unary_kind kind);
struct ast_node *ast_bin(struct ast_node *a, struct ast_node *b, enum ast_bin_kind kind);
struct ast_node *ast_stmt_expr(struct ast_node *a);
struct ast_node *ast_stmt_comp();
struct ast_node *ast_call(struct ast_node *a, struct vec arg_expr_list);
struct ast_node *ast_stmt_while(struct ast_node *a, struct ast_node *b);
struct ast_node *ast_stmt_if(struct ast_node *a, struct ast_node *b);

struct ast_node *ast_declaration(enum ast_type_kind declaration_specifiers, struct vec init_declarator_list);
struct ast_node *ast_init_declarator(struct ast_node *declarator, struct ast_node *initializer);
struct ast_node *ast_declarator(int pointer, struct ast_node *direct_declarator);
struct ast_node *ast_array_declarator(struct ast_node *direct_declarator, struct ast_node *size);
struct ast_node *ast_function_declarator(struct ast_node *direct_declarator, struct vec parameter_type_list);
struct ast_node *ast_parameter_declaration(struct ast_node *declarator, enum ast_type_kind declaration_specifiers);
struct ast_node *ast_translation_unit(struct ast_node *item);
struct ast_node *ast_function_definition(enum ast_type_kind declaration_specifiers, struct ast_node *declarator, struct ast_node *compound_statement);
struct vec ast_list(struct ast_node *item);

#endif
