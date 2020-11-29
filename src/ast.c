// SPDX-License-Identifier: GPL-3.0-only
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <c_compiler/ast.h>

char *strdup(const char *s) {
	size_t len = strlen(s);
	char *res = malloc(len + 1);
	memcpy(res, s, len + 1);
	return res;
}

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
	case AST_COMPOUND_LITERAL:
		fprintf(f, "(");
		ast_fprint(f, n->compound_literal.type_name, ind);
		fprintf(f, "){\n");
		v = &n->compound_literal.list;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
			fprintf(f, ",\n");
		}
		indent(f, ind);
		fprintf(f, "}");
		break;
	case AST_SIZEOF_EXPR:
		fprintf(f, "sizeof(");
		ast_fprint(f, n->sizeof_expr.type_name, ind);
		fprintf(f, ")");
		break;
	case AST_ALIGNOF_EXPR:
		fprintf(f, "alignof(");
		ast_fprint(f, n->alignof_expr.type_name, ind);
		fprintf(f, ")");
		break;
	case AST_CAST:
		fprintf(f, "(");
		ast_fprint(f, n->cast.type_name, ind);
		fprintf(f, ")");
		ast_fprint(f, n->cast.expr, ind);
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
		// this output won't be valid C syntax, but it better shows our
		// internal representation
		if (n->declarator.ident) {
			ast_fprint(f, n->declarator.ident, ind);
		}
		v = &n->declarator.v;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
		}
		break;
	case AST_DECLARATION_SPECIFIERS:
		for (int i = 0; i < AST_STORAGE_CLASS_SPECIFIER_N; ++i) {
			for (int k = 0; k < n->declaration_specifiers.storage_class_specifiers[i]; ++k) {
				fprintf(f, "%s", storage_class_specifier[i]);
				fprintf(f, " ");
			}
		}
		for (int i = 0; i < AST_TYPE_QUALIFIER_N; ++i) {
			for (int k = 0; k < n->declaration_specifiers.type_qualifiers[i]; ++k) {
				fprintf(f, "%s", type_qualifier[i]);
				fprintf(f, " ");
			}
		}
		for (int i = 0; i < AST_FUNCTION_SPECIFIER_N; ++i) {
			for (int k = 0; k < n->declaration_specifiers.function_specifiers[i]; ++k) {
				fprintf(f, "%s", function_specifier[i]);
				fprintf(f, " ");
			}
		}
		v = &n->declaration_specifiers.alignment_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			fprintf(f, " ");
		}
		for (int i = 0; i < AST_BUILTIN_TYPE_N; ++i) {
			for (int k = 0; k < n->declaration_specifiers.builtin_type_specifiers[i]; ++k) {
				fprintf(f, "%s", builtin_type[i]);
				fprintf(f, " ");
			}
		}
		v = &n->declaration_specifiers.type_specifiers;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
			if (i < v->len - 1) fprintf(f, " ");
		}
		break;
	case AST_ALIGNMENT_SPECIFIER:
		fprintf(f, "_Alignas(");
		ast_fprint(f, n->alignment_specifier.expr, ind);
		fprintf(f, ")");
		break;
	case AST_POINTER_DECLARATOR:
		fprintf(f, "*");
		for (int i = 0; i < AST_TYPE_QUALIFIER_N; ++i) {
			for (int k = 0; k < n->pointer_declarator.type_qualifiers[i]; ++k) {
				fprintf(f, "%s", type_qualifier[i]);
				fprintf(f, " ");
			}
		}
		break;
	case AST_ARRAY_DECLARATOR:
		fprintf(f, "[");
		if (n->array_declarator.size) {
			ast_fprint(f, n->array_declarator.size, ind);
		}
		fprintf(f, "]");
		break;
	case AST_FUNCTION_DECLARATOR:
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
	case AST_ENUM_SPECIFIER:
		fprintf(f, "enum ");
		if (n->enum_specifier.ident) {
			ast_fprint(f, n->enum_specifier.ident, ind);
			fprintf(f, " ");
		}
		fprintf(f, "{\n");
		v = &n->enum_specifier.enumerators;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
			fprintf(f, ",\n");
		}
		indent(f, ind);
		fprintf(f, "}");
		break;
	case AST_ENUM_SPECIFIER_INCOMPLETE:
		fprintf(f, "enum ");
		ast_fprint(f, n->enum_specifier_incomplete.ident, ind);
		break;
	case AST_ENUMERATOR:
		ast_fprint(f, n->enumerator.ident, ind);
		if (n->enumerator.expr) {
			fprintf(f, " = ");
			ast_fprint(f, n->enumerator.expr, ind);
		}
		break;
	case AST_DESIGNATOR_INDEX:
		fprintf(f, "[");
		ast_fprint(f, n->designator_index, ind);
		fprintf(f, "]");
		break;
	case AST_DESIGNATOR_IDENT:
		fprintf(f, ".");
		ast_fprint(f, n->designator_ident, ind);
		break;
	case AST_DESIGNATION:
		v = &n->designation;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			ast_fprint(f, *ni, ind);
		}
		fprintf(f, " = ");
		break;
	case AST_INITIALIZER:
		fprintf(f, "{\n");
		v = &n->initializer.list;
		for (int i = 0; i < v->len; ++i) {
			const struct ast_node * const *ni = vec_get_c(v, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
			fprintf(f, ",\n");
		}
		indent(f, ind);
		fprintf(f, "}");
		break;
	case AST_INITIALIZER_LIST_ITEM:
		if (n->initializer_list_item.designation) {
			ast_fprint(f, n->initializer_list_item.designation, ind);
		}
		ast_fprint(f, n->initializer_list_item.initializer, ind);
		break;
	case AST_TYPE_NAME:
		if (n->type_name.specifier_qualifier_list) {
			ast_fprint(f, n->type_name.specifier_qualifier_list, ind);
			fprintf(f, " ");
		}
		if (n->type_name.declarator) {
			ast_fprint(f, n->type_name.declarator, ind);
		}
		break;
	case AST_STATIC_ASSERT:
		fprintf(f, "_Static_assert(");
		ast_fprint(f, n->static_assert_.cond, ind);
		fprintf(f, ", ");
		ast_fprint(f, n->static_assert_.message, ind);
		fprintf(f, ");");
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
struct ast_node *ast_type_specifier(struct ast_node *declaration_specifiers, enum ast_builtin_type bt, struct ast_node *n) {
	if (n) {
		vec_append(&declaration_specifiers->declaration_specifiers.type_specifiers, &n);
	}
	if (bt != AST_BUILTIN_TYPE_N) {
		declaration_specifiers->declaration_specifiers.builtin_type_specifiers[bt]++;
	}
	return declaration_specifiers;
}
struct ast_node *ast_declarator_begin(struct ast_node *ident) {
	return ast_alloc((struct ast_node){
		.kind = AST_DECLARATOR,
		.declarator = {
			.ident = ident,
			.v = vec_new_empty(sizeof(struct ast_node *)),
		}
	});
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
struct ast_node *ast_pointer_declarator(struct ast_node *direct_declarator, struct vec pointer) {
	for (int i = 0; i < pointer.len; ++i) {
		vec_append(&direct_declarator->declarator.v, vec_get(&pointer, i));
	}
	vec_free(&pointer);
	return direct_declarator;
}
struct ast_node *ast_array_declarator(struct ast_node *direct_declarator, struct ast_node *size) {
	struct ast_node *n = ast_alloc((struct ast_node){
		.kind = AST_ARRAY_DECLARATOR,
		.array_declarator.size = size,
	});
	vec_append(&direct_declarator->declarator.v, &n);
	return direct_declarator;
}
struct ast_node *ast_function_declarator(struct ast_node *direct_declarator, struct vec parameter_type_list) {
	struct ast_node *n = ast_alloc((struct ast_node){
		.kind = AST_FUNCTION_DECLARATOR,
		.function_declarator.parameter_type_list = parameter_type_list,
	});
	vec_append(&direct_declarator->declarator.v, &n);
	return direct_declarator;
}
struct ast_node *ast_declaration_specifiers() {
	return ast_alloc((struct ast_node){
		.kind = AST_DECLARATION_SPECIFIERS,
		.declaration_specifiers = {
			.storage_class_specifiers = { 0 },
			.type_specifiers = vec_new_empty(sizeof(struct ast_node *)),
			.type_qualifiers = { 0 },
			.function_specifiers = { 0 },
			.alignment_specifiers = vec_new_empty(sizeof(struct ast_node *)),
		}
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
