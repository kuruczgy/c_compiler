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

static void indent(FILE *f, int ind) {
	for (int i = 0; i < ind; ++i) fprintf(f, "\t");
}
void ast_fprint(FILE *f, const struct ast_node *n, int ind) {
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
	case AST_CALL:
		ast_fprint(f, n->call.a, ind);
		fprintf(f, "(");
		for (int i = 0; i < n->call.args.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->call.args, i);
			ast_fprint(f, *ni, ind);
			if (i < n->call.args.len - 1) fprintf(f, ", ");
		}
		fprintf(f, ")");
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
	case AST_STMT_EXPR:
		ast_fprint(f, n->stmt_expr.a, ind);
		fprintf(f, ";\n");
		break;
	case AST_STMT_COMP:
		fprintf(f, "{\n");
		for (int i = 0; i < n->stmt_comp.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->stmt_comp, i);
			indent(f, ind + 1);
			ast_fprint(f, *ni, ind + 1);
		}
		indent(f, ind);
		fprintf(f, "}\n");
		break;
	case AST_STMT_WHILE:
		fprintf(f, "while (");
		ast_fprint(f, n->stmt_while.a, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_while.b, ind);
		break;
	case AST_STMT_IF:
		fprintf(f, "if (");
		ast_fprint(f, n->stmt_if.a, ind);
		fprintf(f, ") ");
		ast_fprint(f, n->stmt_if.b, ind);
		break;
	case AST_DECL:
		if (n->decl.t.kind == AST_TYPE_INT) fprintf(f, "int");
		else fprintf(f, "??");
		fprintf(f, " ");

		for (int i = 0; i < n->decl.t.pointer; ++i) fprintf(f, "*");
		ast_fprint(f, n->decl.t.ident, ind);
		if (n->decl.t.array) {
			fprintf(f, "[");
			ast_fprint(f, n->decl.t.array, ind);
			fprintf(f, "]");
		}
		fprintf(f, ";\n");
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
struct ast_node *ast_decl(struct ast_type t) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_DECL,
		.decl = { .t = t }
	};
	return n;
}
struct ast_node *ast_stmt_while(struct ast_node *a, struct ast_node *b) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_STMT_WHILE,
		.stmt_while = {
			.a = a, .b = b
		}
	};
	return n;
}
struct ast_node *ast_stmt_if(struct ast_node *a, struct ast_node *b) {
	struct ast_node *n = malloc(sizeof(struct ast_node));
	*n = (struct ast_node){
		.kind = AST_STMT_IF,
		.stmt_if = {
			.a = a, .b = b
		}
	};
	return n;
}
