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
	AST_COMPOUND_LITERAL,
	AST_SIZEOF_EXPR,
	AST_ALIGNOF_EXPR,
	AST_CAST,
	AST_BIN,
	AST_CONDITIONAL,
	AST_STMT_LABELED,
	AST_STMT_LABELED_CASE,
	AST_STMT_LABELED_DEFAULT,
	AST_STMT_EXPR,
	AST_STMT_COMP,
	AST_STMT_WHILE,
	AST_STMT_DO_WHILE,
	AST_STMT_FOR,
	AST_STMT_IF,
	AST_STMT_SWITCH,
	AST_STMT_GOTO,
	AST_STMT_CONTINUE,
	AST_STMT_BREAK,
	AST_STMT_RETURN,
	AST_CALL,
	AST_DECLARATION,
	AST_INIT_DECLARATOR,
	AST_DECLARATOR,
	AST_DECLARATION_SPECIFIERS,
	AST_ALIGNMENT_SPECIFIER,
	AST_POINTER_DECLARATOR,
	AST_ARRAY_DECLARATOR,
	AST_FUNCTION_DECLARATOR,
	AST_PARAMETER_DECLARATION,
	AST_TRANSLATION_UNIT,
	AST_FUNCTION_DEFINITION,
	AST_SU_SPECIFIER,
	AST_SU_SPECIFIER_INCOMPLETE,
	AST_STRUCT_DECLARATION,
	AST_STRUCT_DECLARATOR,
	AST_ENUM_SPECIFIER,
	AST_ENUM_SPECIFIER_INCOMPLETE,
	AST_ENUMERATOR,
	AST_DESIGNATOR_INDEX,
	AST_DESIGNATOR_IDENT,
	AST_DESIGNATION,
	AST_INITIALIZER,
	AST_INITIALIZER_LIST_ITEM,
	AST_TYPE_NAME,
	AST_STATIC_ASSERT,
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

enum ast_storage_class_specifier {
	AST_STORAGE_CLASS_SPECIFIER_TYPEDEF,
	AST_STORAGE_CLASS_SPECIFIER_EXTERN,
	AST_STORAGE_CLASS_SPECIFIER_STATIC,
	AST_STORAGE_CLASS_SPECIFIER_THREAD_LOCAL,
	AST_STORAGE_CLASS_SPECIFIER_AUTO,
	AST_STORAGE_CLASS_SPECIFIER_REGISTER,
	AST_STORAGE_CLASS_SPECIFIER_N,
};
enum ast_builtin_type {
	AST_BUILTIN_TYPE_VOID,
	AST_BUILTIN_TYPE_CHAR,
	AST_BUILTIN_TYPE_SHORT,
	AST_BUILTIN_TYPE_INT,
	AST_BUILTIN_TYPE_LONG,
	AST_BUILTIN_TYPE_FLOAT,
	AST_BUILTIN_TYPE_DOUBLE,
	AST_BUILTIN_TYPE_SIGNED,
	AST_BUILTIN_TYPE_UNSIGNED,
	AST_BUILTIN_TYPE_BOOL,
	AST_BUILTIN_TYPE_COMPLEX,
	AST_BUILTIN_TYPE_N,
};
enum ast_type_qualifier {
	AST_TYPE_QUALIFIER_CONST,
	AST_TYPE_QUALIFIER_RESTRICT,
	AST_TYPE_QUALIFIER_VOLATILE,
	AST_TYPE_QUALIFIER_ATOMIC,
	AST_TYPE_QUALIFIER_N,
};
enum ast_function_specifier {
	AST_FUNCTION_SPECIFIER_INLINE,
	AST_FUNCTION_SPECIFIER_NORETURN,
	AST_FUNCTION_SPECIFIER_N,
};
enum ast_su {
	AST_SU_STRUCT,
	AST_SU_UNION,
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
		struct {
			struct ast_node *type_name;
			struct vec list; /* vec<struct ast_node *> */
		} compound_literal;
		struct { struct ast_node *type_name; } sizeof_expr;
		struct { struct ast_node *type_name; } alignof_expr;
		struct {
			struct ast_node *type_name;
			struct ast_node *expr;
		} cast;
		struct { struct ast_node *a, *b; enum ast_bin_kind kind; } bin;
		struct {
			struct ast_node *cond;
			struct ast_node *expr, *expr_else;
		} conditional;
		struct { struct ast_node *ident, *stmt; } stmt_labeled;
		struct { struct ast_node *expr, *stmt; } stmt_labeled_case;
		struct { struct ast_node *stmt; } stmt_labeled_default;
		struct { struct ast_node *a; } stmt_expr;
		struct vec stmt_comp; /* vec<struct ast_node *> */
		struct { struct ast_node *cond, *stmt; } stmt_while;
		struct { struct ast_node *cond, *stmt; } stmt_do_while;
		struct { struct ast_node *a, *b, *c, *stmt; } stmt_for;
		struct { struct ast_node *cond, *stmt, *stmt_else; } stmt_if;
		struct { struct ast_node *cond, *stmt; } stmt_switch;
		struct { struct ast_node *ident; } stmt_goto;
		struct { struct ast_node *expr; } stmt_return;
		struct { struct ast_node *a; struct vec args; } call;
		struct {
			struct ast_node *declaration_specifiers;
			struct vec init_declarator_list;
		} declaration;
		struct {
			struct ast_node *declarator, *initializer;
		} init_declarator;
		struct {
			struct ast_node *ident; // can be null
			struct vec v; /* vec<struct ast_node *> */
		} declarator;
		struct {
			char storage_class_specifiers[AST_STORAGE_CLASS_SPECIFIER_N];
			char builtin_type_specifiers[AST_BUILTIN_TYPE_N];
			struct vec type_specifiers; /* vec<struct ast_node *> */
			char type_qualifiers[AST_TYPE_QUALIFIER_N];
			char function_specifiers[AST_FUNCTION_SPECIFIER_N];
			struct vec alignment_specifiers; /* vec<struct ast_node *> */
		} declaration_specifiers;
		struct {
			struct ast_node *expr;
		} alignment_specifier;
		struct {
			char type_qualifiers[AST_TYPE_QUALIFIER_N];
		} pointer_declarator;
		struct {
			struct ast_node *size;
		} array_declarator;
		struct {
			struct vec parameter_type_list;
		} function_declarator;
		struct {
			struct ast_node *declaration_specifiers;
			struct ast_node *declarator;
		} parameter_declaration;
		struct vec translation_unit; /* vec<struct ast_node *> */
		struct {
			struct ast_node *declaration_specifiers;
			struct ast_node *declarator;
			struct ast_node *compound_statement;
		} function_definition;
		struct {
			enum ast_su su;
			struct ast_node *ident;
			struct vec declarations; /* vec<struct ast_node *> */
		} su_specifier;
		struct {
			enum ast_su su;
			struct ast_node *ident;
		} su_specifier_incomplete;
		struct {
			struct ast_node *specifier_qualifier_list;
			struct vec declarators; /* vec<struct ast_node *> */
		} struct_declaration;
		struct {
			struct ast_node *declarator;
			struct ast_node *bitfield_expr;
		} struct_declarator;
		struct {
			struct ast_node *ident;
			struct vec enumerators; /* vec<struct ast_node *> */
		} enum_specifier;
		struct {
			struct ast_node *ident;
		} enum_specifier_incomplete;
		struct {
			struct ast_node *ident, *expr;
		} enumerator;
		struct ast_node *designator_index;
		struct ast_node *designator_ident;
		struct vec designation; /* vec<struct ast_node *> */
		struct {
			struct vec list; /* vec<struct ast_node *> */
		} initializer;
		struct {
			struct ast_node *designation;
			struct ast_node *initializer;
		} initializer_list_item;
		struct {
			struct ast_node *specifier_qualifier_list;
			struct ast_node *declarator;
		} type_name;
		struct {
			struct ast_node *cond, *message;
		} static_assert_;
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

struct ast_node *ast_type_specifier(struct ast_node *declaration_specifiers, enum ast_builtin_type bt, struct ast_node *n);
struct ast_node *ast_declarator_begin(struct ast_node *ident);
struct ast_node *ast_declaration(struct ast_node *declaration_specifiers, struct vec init_declarator_list);
struct ast_node *ast_init_declarator(struct ast_node *declarator, struct ast_node *initializer);
struct ast_node *ast_declaration_specifiers();
struct ast_node *ast_pointer_declarator(struct ast_node *direct_declarator, struct vec pointer);
struct ast_node *ast_array_declarator(struct ast_node *direct_declarator, struct ast_node *size);
struct ast_node *ast_function_declarator(struct ast_node *direct_declarator, struct vec parameter_type_list);
struct ast_node *ast_parameter_declaration(struct ast_node *declarator, struct ast_node *declaration_specifiers);
struct ast_node *ast_translation_unit(struct ast_node *item);
struct ast_node *ast_function_definition(struct ast_node *declaration_specifiers, struct ast_node *declarator, struct ast_node *compound_statement);
struct vec ast_list(struct ast_node *item);

struct ast_node *ast_alloc(struct ast_node node);

#endif
