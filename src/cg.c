// SPDX-License-Identifier: GPL-3.0-only
#include <ds/hashmap.h>
#include <stdio.h>
#include <c_compiler/cg.h>

struct state {
	struct hashmap vars; /* hashmap<int> */
	int sp; /* stack pointer */
	FILE *f;
	struct vec strings;
	int label;
};

typedef struct {
	int deref_n;
	long long int s;
} val;

static int get_label(struct state *s) {
	return s->label++;
}
static void put_label(struct state *s, int l) {
	fprintf(s->f, "label_%d:\n", l);
}

static void state_init(struct state *s) {
	hashmap_init(&s->vars, sizeof(int));
	s->sp = 0;
	s->f = stdout;
	s->strings = vec_new_empty(sizeof(const char *));
	s->label = 0;
}

static int const_eval(const struct ast_node *n, int *res) {
	if (n->kind == AST_INTEGER) {
		*res = n->integer;
		return 0;
	}
	return -1;
}

static void val_read(struct state *s, const val *v, const char *reg) {
	if (v->deref_n == -1) {
		fprintf(s->f, "mov %s, rbp\n", reg);
		fprintf(s->f, "sub %s, %lld\n", reg, -v->s);
	} else if (v->deref_n == 0) {
		fprintf(s->f, "mov %s, qword [rbp%lld]\n", reg, v->s);
	} else {
		fprintf(s->f, "mov rcx, qword [rbp%lld]\n", v->s);
		for (int i = 0; i < v->deref_n; ++i) {
			fprintf(s->f, "mov rcx, [rcx]\n");
		}
		fprintf(s->f, "mov %s, rcx\n", reg);
	}
}
static void val_store(struct state *s, const val *v, const char *reg) {
	if (v->deref_n == 0) {
		fprintf(s->f, "mov qword [rbp%lld], %s\n", v->s, reg);
	} else {
		fprintf(s->f, "mov rcx, qword [rbp%lld]\n", v->s);
		for (int i = 0; i < v->deref_n - 1; ++i) {
			fprintf(s->f, "mov rcx, [rcx]\n");
		}
		fprintf(s->f, "mov [rcx], %s\n", reg);
	}
}
static val val_push_new(struct state *s, const char *reg) {
	s->sp -= 8;
	fprintf(s->f, "mov qword [rbp%d], %s\n", s->sp, reg);
	return (val){ .s = s->sp };
}

static val cg_gen_expr(struct state *s, const struct ast_node *n);

static val cg_gen_unary(struct state *s, const struct ast_node *n) {
	val val_a = cg_gen_expr(s, n->unary.a);
	switch (n->unary.kind) {
	case AST_PRE_INCR:
		val_read(s, &val_a, "rax");
		fprintf(s->f, "add rax, 1\n");
		val_store(s, &val_a, "rax");
		return val_a;
	case AST_PRE_DECR:
		val_read(s, &val_a, "rax");
		fprintf(s->f, "sub rax, 1\n");
		val_store(s, &val_a, "rax");
		return val_a;
	case AST_POST_INCR:
		break;
	case AST_POST_DECR:
		break;
	case AST_UNARY_REF:
		val_a.deref_n--;
		return val_a;
	case AST_UNARY_DEREF:
		val_a.deref_n++;
		return val_a;
	case AST_UNARY_PLUS:
		break;
	case AST_UNARY_MINUS:
		break;
	case AST_UNARY_NOT:
		val_read(s, &val_a, "rax");
		fprintf(s->f, "not rax\n");
		return val_push_new(s, "rax");
	case AST_UNARY_NOTB:
		break;
	case AST_UNARY_SIZEOF:
		break;
	}
	return (val){ .s = 1 };
}

static val cg_gen_bin(struct state *s, const struct ast_node *n) {
	val val_a = cg_gen_expr(s, n->bin.a);
	val val_b = cg_gen_expr(s, n->bin.b);
	switch (n->bin.kind) {
	case AST_BIN_MUL:
		break;
	case AST_BIN_DIV:
		break;
	case AST_BIN_MOD:
		break;
	case AST_BIN_ADD:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "add rax, rbx\n");
		return val_push_new(s, "rax");
	case AST_BIN_SUB:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "sub rax, rbx\n");
		return val_push_new(s, "rax");
	case AST_BIN_LSHIFT:
		break;
	case AST_BIN_RSHIFT:
		break;
	case AST_BIN_LT:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "setl al\n");
		fprintf(s->f, "movzx rax, al\n");
		return val_push_new(s, "rax");
	case AST_BIN_GT:
		break;
	case AST_BIN_LEQ:
		break;
	case AST_BIN_GEQ:
		break;
	case AST_BIN_EQB:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "sete al\n");
		fprintf(s->f, "movzx rax, al\n");
		return val_push_new(s, "rax");
	case AST_BIN_NEQ:
		break;
	case AST_BIN_AND:
		break;
	case AST_BIN_XOR:
		break;
	case AST_BIN_OR:
		break;
	case AST_BIN_ANDB:
		break;
	case AST_BIN_ORB:
		break;
	case AST_BIN_ASSIGN:
		val_read(s, &val_b, "rax");
		val_store(s, &val_a, "rax");
		return val_a;
	case AST_BIN_COMMA:
		break;
	}
	return (val){ .s = 4 };
}

static val cg_gen_expr(struct state *s, const struct ast_node *n) {
	switch (n->kind) {
	case AST_IDENT: ;
		int *loc;
		if (hashmap_get(&s->vars, n->ident, (void**)&loc) == MAP_OK) {
			return (val){ .s = *loc };
		} else {
			return (val){ .s = 1 };
		}
	case AST_INTEGER:
		fprintf(s->f, "mov rax, %lld\n", n->integer);
		return val_push_new(s, "rax");
	case AST_CHARACTER_CONSTANT:
		fprintf(s->f, "mov rax, %d\n", n->character_constant);
		return val_push_new(s, "rax");
	case AST_STRING: ;
		int str = vec_append(&s->strings, &n->string);
		fprintf(s->f, "mov rax, s%d\n", str);
		return val_push_new(s, "rax");
	case AST_INDEX: ;
		// val val_a = cg_gen_expr(s, n->index.a);
		// val val_b = cg_gen_expr(s, n->index.b);
		// val_read(s, &val_a, "rax");
		// val_read(s, &val_b, "rbx");
		// fprintf(s->f, "add rax, rbx\n");
		// val res = val_push_new(s, "rax");
		// ++res.deref_n;
		// return res;
		break;
	case AST_CALL: ;
		const char *func = "<nope>";
		if (n->call.a->kind == AST_IDENT) {
			func = n->call.a->ident;
		}

		struct vec vals = vec_new_empty(sizeof(val));
		for (int i = 0; i < n->call.args.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->call.args, i);
			val v = cg_gen_expr(s, *ni);
			vec_append(&vals, &v);
		}

		const char *call_regs[] = { "rdi", "rsi", "rdx", "rcx" };
		for (int i = 0; i < vals.len; ++i) {
			val *vi = vec_get(&vals, i);
			val_read(s, vi, call_regs[i]);
		}

		fprintf(s->f, "sub rsp, %d\n", (-s->sp) + (16 + s->sp % 16));
		fprintf(s->f, "call %s\n", func);
		return val_push_new(s, "rax");
	case AST_MEMBER:
		// TODO
		break;
	case AST_MEMBER_DEREF:
		// TODO
		break;
	case AST_UNARY: return cg_gen_unary(s, n);
	case AST_BIN: return cg_gen_bin(s, n);
	default: // TODO: we shouldn't be here
		break;
	}
	return (val){ .s = 3 };
}

static void cg_gen_stmt_comp(struct state *s, const struct ast_node *n) {
	for (int i = 0; i < n->stmt_comp.len; ++i) {
		const struct ast_node *ni =
			*(struct ast_node * const *)vec_get_c(&n->stmt_comp, i);
		// fprintf(s->f, "; ");
		// ast_fprint(s->f, ni, 0);
		switch (ni->kind) {
		case AST_DECL: ;
			struct ast_node *name = ni->decl.t.ident;
			struct ast_type t = ni->decl.t;
			if (name->kind == AST_IDENT) {
				// TODO: type. now we assume literally EVERY type is 64 bit...
				int size = 8, array;
				if (t.array && (const_eval(t.array, &array) == 0)) {
					size *= array;
				}
				s->sp -= size;
				hashmap_put(&s->vars, name->ident, &s->sp);
			} else {
				// TODO: we shouldn't be here
			}
			break;
		case AST_STMT_EXPR:
			cg_gen_expr(s, ni->stmt_expr.a);
			break;
		case AST_STMT_WHILE: ;
			int label_start = get_label(s), label_end = get_label(s);
			put_label(s, label_start);
			val val_a = cg_gen_expr(s, ni->stmt_while.a);
			val_read(s, &val_a, "rax");
			fprintf(s->f, "cmp rax, 0\n");
			fprintf(s->f, "je label_%d\n", label_end);
			if (ni->stmt_while.b->kind == AST_STMT_COMP)
				cg_gen_stmt_comp(s, ni->stmt_while.b);
			fprintf(s->f, "jmp label_%d\n", label_start);
			put_label(s, label_end);
			break;
		case AST_STMT_IF: {
			int label_end = get_label(s);
			val val_a = cg_gen_expr(s, ni->stmt_if.a);
			val_read(s, &val_a, "rax");
			fprintf(s->f, "cmp rax, 0\n");
			fprintf(s->f, "je label_%d\n", label_end);
			if (ni->stmt_if.b->kind == AST_STMT_COMP)
				cg_gen_stmt_comp(s, ni->stmt_if.b);
			put_label(s, label_end);
			break;
		}
		default: ;
			// TODO: we shouldn't be here
		}
	}
}

void cg_gen(const struct ast_node *n) {
	struct state _s;
	struct state *s = &_s;
	state_init(s);

	fprintf(s->f, "global main\n");
	fprintf(s->f, "section .text\n");
	fprintf(s->f, "extern printf\n");
	fprintf(s->f, "extern scanf\n");
	fprintf(s->f, "extern malloc\n");
	fprintf(s->f, "extern free\n");
	fprintf(s->f, "extern getchar\n");
	fprintf(s->f, "main:\n");
	fprintf(s->f, "push rbp\n");
	fprintf(s->f, "mov rbp, rsp\n");

	if (n->kind == AST_STMT_COMP) {
		cg_gen_stmt_comp(s, n);
	}

	fprintf(s->f, "mov rsp, rbp\n");
	fprintf(s->f, "pop rbp\n");
	fprintf(s->f, "mov rax, 0\n");
	fprintf(s->f, "ret\n");

	fprintf(s->f, "section .rodata\n");
	for (int i = 0; i < s->strings.len; ++i) {
		const char * const *si = vec_get_c(&s->strings, i);
		fprintf(s->f, "s%d: db %s, 0\n", i, *si);
	}
}
