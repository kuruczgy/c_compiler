// SPDX-License-Identifier: GPL-3.0-only
#include <stdio.h>
#include <c_compiler/ast.h>

static const char *unary_op[] = {
	[AST_PRE_INCR] = "++",
	[AST_PRE_DECR] = "--",
	[AST_POST_INCR] = "(post++)",
	[AST_POST_DECR] = "(post--)",
	[AST_UNARY_REF] = "&",
	[AST_UNARY_DEREF] = "*",
	[AST_UNARY_PLUS] = "+",
	[AST_UNARY_MINUS] = "-",
	[AST_UNARY_NOT] = "~",
	[AST_UNARY_NOTB] = "!",
	[AST_UNARY_SIZEOF] = "sizeof",
};
static const char *bin_op[] = {
	[AST_BIN_MUL] = "*",
	[AST_BIN_DIV] = "/",
	[AST_BIN_MOD] = "%",
	[AST_BIN_ADD] = "+",
	[AST_BIN_SUB] = "-",
	[AST_BIN_LSHIFT] = "<<",
	[AST_BIN_RSHIFT] = ">>",
	[AST_BIN_LT] = "<",
	[AST_BIN_GT] = ">",
	[AST_BIN_LEQ] = "<=",
	[AST_BIN_GEQ] = ">=",
	[AST_BIN_EQB] = "==",
	[AST_BIN_NEQ] = "!=",
	[AST_BIN_AND] = "&",
	[AST_BIN_XOR] = "^",
	[AST_BIN_OR] = "|",
	[AST_BIN_ANDB] = "&&",
	[AST_BIN_ORB] = "||",
	[AST_BIN_ASSIGN] = "=",
	[AST_BIN_COMMA] = ",",
};
static const char *storage_class_specifier[] = {
	[AST_STORAGE_CLASS_SPECIFIER_TYPEDEF] = "typedef",
	[AST_STORAGE_CLASS_SPECIFIER_EXTERN] = "extern",
	[AST_STORAGE_CLASS_SPECIFIER_STATIC] = "static",
	[AST_STORAGE_CLASS_SPECIFIER_THREAD_LOCAL] = "_Thread_local",
	[AST_STORAGE_CLASS_SPECIFIER_AUTO] = "auto",
	[AST_STORAGE_CLASS_SPECIFIER_REGISTER] = "register",
};
static const char *builtin_type[] = {
	[AST_BUILTIN_TYPE_VOID] = "void",
	[AST_BUILTIN_TYPE_CHAR] = "char",
	[AST_BUILTIN_TYPE_SHORT] = "short",
	[AST_BUILTIN_TYPE_INT] = "int",
	[AST_BUILTIN_TYPE_LONG] = "long",
	[AST_BUILTIN_TYPE_FLOAT] = "float",
	[AST_BUILTIN_TYPE_DOUBLE] = "double",
	[AST_BUILTIN_TYPE_SIGNED] = "signed",
	[AST_BUILTIN_TYPE_UNSIGNED] = "unsigned",
	[AST_BUILTIN_TYPE_BOOL] = "_Bool",
	[AST_BUILTIN_TYPE_COMPLEX] = "_Complex",
};
static const char *type_qualifier[] = {
	[AST_TYPE_QUALIFIER_CONST] = "const",
	[AST_TYPE_QUALIFIER_RESTRICT] = "restrict",
	[AST_TYPE_QUALIFIER_VOLATILE] = "volatile",
	[AST_TYPE_QUALIFIER_ATOMIC] = "_Atomic",
};
static const char *function_specifier[] = {
	[AST_FUNCTION_SPECIFIER_INLINE] = "inline",
	[AST_FUNCTION_SPECIFIER_NORETURN] = "_Noreturn",
};
static const char *struct_or_union[] = {
	[AST_SU_STRUCT] = "struct",
	[AST_SU_UNION] = "union",
};

static void indent(FILE *f, int ind) {
	for (int i = 0; i < ind; ++i) fprintf(f, "\t");
}
void ast_fprint(FILE *f, const struct ast_node *n, int ind) {
	const struct vec *v;
	switch (n->kind) {
	case AST_IDENT: fprintf(f, "%s", n->ident); break;
	case AST_INTEGER: fprintf(f, "%lld", n->integer); break;
	case AST_CHARACTER_CONSTANT: fprintf(f, "'%c'", n->character_constant); break;
	case AST_STRING: fprintf(f, "%s", n->string); break;
	case AST_INDEX:
		ast_fprint(f, n->index.a, ind);
		fprintf(f, "[");
		ast_fprint(f, n->index.b, ind);
		fprintf(f, "]");
		break;
	case AST_MEMBER:
		ast_fprint(f, n->member.a, ind);
		fprintf(f, ".%s", n->member.ident);
		break;
	case AST_MEMBER_DEREF:
		ast_fprint(f, n->member.a, ind);
		fprintf(f, "->%s", n->member.ident);
		break;
	case AST_UNARY:
		fprintf(f, "%s", unary_op[n->unary.kind]);
		fprintf(f, "(");
		ast_fprint(f, n->unary.a, ind);
		fprintf(f, ")");
		break;
	case AST_BIN:
		fprintf(f, "(");
		ast_fprint(f, n->bin.a, ind);
		fprintf(f, ") %s (", bin_op[n->bin.kind]);
		ast_fprint(f, n->bin.b, ind);
		fprintf(f, ")");
		break;
	case AST_CONDITIONAL:
		ast_fprint(f, n->conditional.cond, ind);
		fprintf(f, " ? ");
		ast_fprint(f, n->conditional.expr, ind);
		fprintf(f, " : ");
		ast_fprint(f, n->conditional.expr_else, ind);
		break;
	case AST_STMT_LABELED:
		ast_fprint(f, n->stmt_labeled.ident, ind);
		fprintf(f, ": ");
		ast_fprint(f, n->stmt_labeled.stmt, ind);
		break;
	case AST_STMT_LABELED_CASE:
		fprintf(f, "case ");
		ast_fprint(f, n->stmt_labeled_case.expr, ind);
		fprintf(f, ": ");
		ast_fprint(f, n->stmt_labeled_case.stmt, ind);
		break;
	case AST_STMT_LABELED_DEFAULT:
		fprintf(f, "default: ");
		ast_fprint(f, n->stmt_labeled_default.stmt, ind);
		break;
	case AST_STMT_EXPR:
		if (n->stmt_expr.a) {
			ast_fprint(f, n->stmt_expr.a, ind);
		}
		fprintf(f, ";");
		break;
	case AST_STMT_COMP:
		fprintf(f, "{\n");
		for (int i = 0; i < n->stmt_comp.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->stmt_comp, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
			fprintf(f, "\n");
		}
		indent(f, ind);
		fprintf(f, "}");
		break;
	case AST_STMT_WHILE:
		fprintf(f, "while (");
		ast_fprint(f, n->stmt_while.cond, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_while.stmt, ind);
		break;
	case AST_STMT_DO_WHILE:
		fprintf(f, "do ");
		ast_fprint(f, n->stmt_do_while.stmt, ind);
		fprintf(f, " while (");
		ast_fprint(f, n->stmt_do_while.cond, ind);
		fprintf(f, ");");
		break;
	case AST_STMT_FOR:
		fprintf(f, "for (");
		if (n->stmt_for.a) ast_fprint(f, n->stmt_for.a, ind);
		if (!n->stmt_for.a || n->stmt_for.a->kind != AST_DECLARATION) {
			fprintf(f, ";");
		}
		fprintf(f, " ");
		if (n->stmt_for.b) ast_fprint(f, n->stmt_for.b, ind);
		fprintf(f, "; ");
		if (n->stmt_for.c) ast_fprint(f, n->stmt_for.c, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_for.stmt, ind);
		break;
	case AST_STMT_IF:
		fprintf(f, "if (");
		ast_fprint(f, n->stmt_if.cond, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_if.stmt, ind);
		if (n->stmt_if.stmt_else) {
			fprintf(f, " else ");
			ast_fprint(f, n->stmt_if.stmt_else, ind);
		}
		break;
	case AST_STMT_SWITCH:
		fprintf(f, "switch (");
		ast_fprint(f, n->stmt_switch.cond, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_switch.stmt, ind);
		break;
	case AST_STMT_GOTO:
		fprintf(f, "goto ");
		ast_fprint(f, n->stmt_goto.ident, ind);
		fprintf(f, ";");
		break;
	case AST_STMT_CONTINUE:
		fprintf(f, "continue;");
		break;
	case AST_STMT_BREAK:
		fprintf(f, "break;");
		break;
	case AST_STMT_RETURN:
		fprintf(f, "return");
		if (n->stmt_return.expr) {
			fprintf(f, " ");
			ast_fprint(f, n->stmt_return.expr, ind);
		}
		fprintf(f, ";");
		break;
	case AST_CALL:
		ast_fprint(f, n->call.a, ind);
		fprintf(f, "(");
		v = &n->call.args;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, ", ");
		}
		fprintf(f, ")");
		break;
	case AST_DECLARATION:
		ast_fprint(f, n->declaration.declaration_specifiers, ind);
		v = &n->declaration.init_declarator_list;
		for (int i = 0; i < v->len; ++i) {
			fprintf(f, " ");
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, ",");
		}
		fprintf(f, ";");
		break;
	case AST_INIT_DECLARATOR:
		ast_fprint(f, n->init_declarator.declarator, ind);
		if (n->init_declarator.initializer) {
			fprintf(f, " = ");
			ast_fprint(f, n->init_declarator.initializer, ind);
		}
		break;
	case AST_DECLARATOR:
		for (int i = 0; i < n->declarator.pointer; ++i) {
			fprintf(f, "*");
		}
		ast_fprint(f, n->declarator.direct_declarator, ind);
		break;
	case AST_DECLARATION_SPECIFIERS:
		v = &n->declaration_specifiers.storage_class_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const enum ast_storage_class_specifier *ei = vec_get_c(v, i);
			fprintf(f, "%s", storage_class_specifier[*ei]);
			fprintf(f, " ");
		}
		v = &n->declaration_specifiers.type_qualifiers;
		for (int i = 0; i < v->len; ++i) {
			const enum ast_type_qualifier *ei = vec_get_c(v, i);
			fprintf(f, "%s", type_qualifier[*ei]);
			fprintf(f, " ");
		}
		v = &n->declaration_specifiers.function_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const enum ast_function_specifier *ei = vec_get_c(v, i);
			fprintf(f, "%s", function_specifier[*ei]);
			fprintf(f, " ");
		}
		v = &n->declaration_specifiers.alignment_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			fprintf(f, " ");
		}
		v = &n->declaration_specifiers.type_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, " ");
		}
		break;
	case AST_SPECIFIER_QUALIFIER_LIST:
		v = &n->specifier_qualifier_list.type_qualifiers;
		for (int i = 0; i < v->len; ++i) {
			const enum ast_type_qualifier *ei = vec_get_c(v, i);
			fprintf(f, "%s", type_qualifier[*ei]);
			fprintf(f, " ");
		}
		v = &n->specifier_qualifier_list.type_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, " ");
		}
		break;
	case AST_BUILTIN_TYPE:
		fprintf(f, "%s", builtin_type[n->builtin_type]);
		break;
	case AST_ALIGNMENT_SPECIFIER:
		fprintf(f, "_Alignas(");
		ast_fprint(f, n->alignment_specifier.expr, ind);
		fprintf(f, ")");
		break;
	case AST_ARRAY_DECLARATOR:
		ast_fprint(f, n->array_declarator.direct_declarator, ind);
		fprintf(f, "[");
		if (n->array_declarator.size) {
			ast_fprint(f, n->array_declarator.size, ind);
		}
		fprintf(f, "]");
		break;
	case AST_FUNCTION_DECLARATOR:
		ast_fprint(f, n->function_declarator.direct_declarator, ind);
		fprintf(f, "(");
		v = &n->function_declarator.parameter_type_list;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, ", ");
		}
		fprintf(f, ")");
		break;
	case AST_PARAMETER_DECLARATION:
		ast_fprint(f, n->parameter_declaration.declaration_specifiers, ind);
		if (n->parameter_declaration.declarator) {
			fprintf(f, " ");
			ast_fprint(f, n->parameter_declaration.declarator, ind);
		}
		break;
	case AST_TRANSLATION_UNIT:
		for (int i = 0; i < n->translation_unit.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->translation_unit, i);
			ast_fprint(f, *ni, ind);
			fprintf(f, "\n");
		}
		break;
	case AST_FUNCTION_DEFINITION:
		ast_fprint(f, n->function_definition.declaration_specifiers, ind);
		fprintf(f, " ");
		ast_fprint(f, n->function_definition.declarator, ind);
		fprintf(f, " ");
		ast_fprint(f, n->function_definition.compound_statement, ind);
		break;
	case AST_SU_SPECIFIER:
		fprintf(f, "%s ", struct_or_union[n->su_specifier.su]);
		if (n->su_specifier.ident) {
			ast_fprint(f, n->su_specifier.ident, ind);
			fprintf(f, " ");
		}
		fprintf(f, "{\n");
		v = &n->su_specifier.declarations;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
			fprintf(f, "\n");
		}
		indent(f, ind);
		fprintf(f, "}");
		break;
	case AST_SU_SPECIFIER_INCOMPLETE:
		fprintf(f, "%s ", struct_or_union[n->su_specifier_incomplete.su]);
		ast_fprint(f, n->su_specifier_incomplete.ident, ind);
		break;
	case AST_STRUCT_DECLARATION:
		ast_fprint(f, n->struct_declaration.specifier_qualifier_list, ind);
		v = &n->struct_declaration.declarators;
		if (vec_any(v)) {
			fprintf(f, " ");
			for (int i = 0; i < v->len; ++i) {
				const struct ast_node * const *ni = vec_get_c(v, i);
				ast_fprint(f, *ni, ind);
				if (i < v->len - 1) {
					fprintf(f, ", ");
				}
			}
		}
		fprintf(f, ";");
		break;
	case AST_STRUCT_DECLARATOR:
		if (n->struct_declarator.declarator) {
			ast_fprint(f, n->struct_declarator.declarator, ind);
		}
		if (n->struct_declarator.bitfield_expr) {
			fprintf(f, ": ");
			ast_fprint(f, n->struct_declarator.bitfield_expr, ind);
		}
		break;
	}
}
struct ast_node *ast_ident(const char *ident) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_IDENT,
		.ident = strdup(ident)
	};
	return n;
}
struct ast_node *ast_integer(long long int integer) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_INTEGER,
		.integer = integer
	};
	return n;
}
struct ast_node *ast_character_constant(int integer) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_CHARACTER_CONSTANT,
		.character_constant = integer
	};
	return n;
}
struct ast_node *ast_string(const char *ident) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_STRING,
		.string = strdup(ident)
	};
	return n;
}
struct ast_node *ast_index(struct ast_node *a, struct ast_node *b) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_INDEX,
		.index = { .a = a, .b = b }
	};
	return n;
}
struct ast_node *ast_member(struct ast_node *a, const char *ident) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_MEMBER,
		.member = { .a = a, .ident = strdup(ident) }
	};
	return n;
}
struct ast_node *ast_member_deref(struct ast_node *a, const char *ident) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_MEMBER_DEREF,
		.member = { .a = a, .ident = strdup(ident) }
	};
	return n;
}
struct ast_node *ast_unary(struct ast_node *a, enum ast_unary_kind kind) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_UNARY,
		.unary = { .a = a, .kind = kind }
	};
	return n;
}
struct ast_node *ast_bin(struct ast_node *a, struct ast_node *b, enum ast_bin_kind kind) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_BIN,
		.bin = { .a = a, .b = b, .kind = kind }
	};
	return n;
}
struct ast_node *ast_stmt_expr(struct ast_node *a) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_STMT_EXPR,
		.stmt_expr = { .a = a }
	};
	return n;
}
struct ast_node *ast_stmt_comp() {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_STMT_COMP,
		.stmt_comp = vec_new_empty(sizeof(struct ast_node *)),
	};
	return n;
}
struct ast_node *ast_call(struct ast_node *a, struct vec arg_expr_list) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_CALL,
		.call = {
			.a = a,
			.args = arg_expr_list
		}
	};
	return n;
}
struct ast_node *ast_declaration(struct ast_node *declaration_specifiers, struct vec init_declarator_list) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_DECLARATION,
		.declaration = {
			.declaration_specifiers = declaration_specifiers,
			.init_declarator_list = init_declarator_list
		}
	};
	return n;
}
struct ast_node *ast_init_declarator(struct ast_node *declarator, struct ast_node *initializer) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_INIT_DECLARATOR,
		.init_declarator = {
			.declarator = declarator,
			.initializer = initializer
		}
	};
	return n;
}
struct ast_node *ast_declarator(int pointer, struct ast_node *direct_declarator) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_DECLARATOR,
		.declarator = {
			.pointer = pointer,
			.direct_declarator = direct_declarator
		}
	};
	return n;
}
struct ast_node *ast_array_declarator(struct ast_node *direct_declarator, struct ast_node *size) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_ARRAY_DECLARATOR,
		.array_declarator = {
			.direct_declarator = direct_declarator,
			size = size
		}
	};
	return n;
}
struct ast_node *ast_function_declarator(struct ast_node *direct_declarator, struct vec parameter_type_list) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_FUNCTION_DECLARATOR,
		.function_declarator = {
			.direct_declarator = direct_declarator,
			.parameter_type_list = parameter_type_list
		}
	};
	return n;
}
struct ast_node *ast_declaration_specifiers() {
	return ast_alloc((struct ast_node){
		.kind = AST_DECLARATION_SPECIFIERS,
		.declaration_specifiers = {
			.storage_class_specifiers = vec_new_empty(sizeof(enum ast_storage_class_specifier)),
			.type_specifiers = vec_new_empty(sizeof(struct ast_node *)),
			.type_qualifiers = vec_new_empty(sizeof(enum ast_type_qualifier)),
			.function_specifiers = vec_new_empty(sizeof(enum ast_function_specifier)),
			.alignment_specifiers = vec_new_empty(sizeof(struct ast_node *)),
		}
	});
}
struct ast_node *ast_specifier_qualifier_list() {
	return ast_alloc((struct ast_node){
		.kind = AST_SPECIFIER_QUALIFIER_LIST,
		.specifier_qualifier_list = {
			.type_specifiers = vec_new_empty(sizeof(struct ast_node *)),
			.type_qualifiers = vec_new_empty(sizeof(enum ast_type_qualifier)),
		}
	});
}
struct ast_node *ast_builtin_type(enum ast_builtin_type builtin_type) {
	return ast_alloc((struct ast_node){
		.kind = AST_BUILTIN_TYPE,
		.builtin_type = builtin_type
	});
}
struct ast_node *ast_parameter_declaration(struct ast_node *declarator, struct ast_node *declaration_specifiers) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_PARAMETER_DECLARATION,
		.parameter_declaration = {
			.declarator = declarator,
			.declaration_specifiers = declaration_specifiers
		}
	};
	return n;
}
struct ast_node *ast_translation_unit(struct ast_node *item) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_TRANSLATION_UNIT,
		.translation_unit = vec_new_empty(sizeof(struct ast_node *))
	};
	vec_append(&n->translation_unit, &item);
	return n;
}
struct ast_node *ast_function_definition(struct ast_node *declaration_specifiers, struct ast_node *declarator, struct ast_node *compound_statement) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_FUNCTION_DEFINITION,
		.function_definition = {
			.declaration_specifiers = declaration_specifiers,
			.declarator = declarator,
			.compound_statement = compound_statement
		}
	};
	return n;
}
struct vec ast_list(struct ast_node *item) {
	struct vec list = vec_new_empty(sizeof(struct ast_node *));
	vec_append(&list, &item);
	return list;
}
struct ast_node *ast_alloc(struct ast_node node) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = node;
	return n;
}
