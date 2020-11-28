// SPDX-License-Identifier: GPL-3.0-only
#include <ds/hashmap.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <c_compiler/cg.h>

#define GETI(x, i) *(struct ast_node * const *)vec_get_c(&(x), i)
#define FOR_EACH_NODE(x) \
	const struct ast_node *ni = GETI(x, 0); \
	for (int i = 0; ni; ni = ((++i < (x).len) ? GETI(x, i): NULL))

struct type {
	const struct ast_node *specifiers;
	const struct ast_node *declarator;
};

struct builtin_types {
	struct type t_int, t_char, t_char_p, t_size_t;
};

struct state {
	struct hashmap vars; /* hashmap<struct decl> */
	int sp; /* stack pointer */
	FILE *f;
	struct vec strings;
	int label;
	struct builtin_types builtin;
};

struct decl {
	struct type t;
	int size;
	int loc;
};

typedef struct {
	int deref_n;
	long long int s;
	bool lvalue;
	struct type t;
} val;

typedef enum {
	S_OK,
	S_ERROR,
} status;

static bool type_is_const(const struct type *t) {
	const struct ast_node *s = t->specifiers;
	const char *qual = NULL;
	if (s->kind == AST_DECLARATION_SPECIFIERS) {
		qual = s->declaration_specifiers.type_qualifiers;
	} else if (s->kind == AST_SPECIFIER_QUALIFIER_LIST) {
		qual = s->specifier_qualifier_list.type_qualifiers;
	}
	return qual[AST_TYPE_QUALIFIER_CONST] > 0;
}
static bool type_has_declarator(const struct type *t) {
	return t->declarator != NULL && t->declarator->kind != AST_IDENT;
}
static bool val_modifiable_lvalue(const val *v) {
	return v->lvalue && v->deref_n >= 0 && !type_is_const(&v->t);
}
static bool val_can_deref(const val *v) {
	const struct ast_node *d = v->t.declarator;
	if (!d) return false;
	assert(d->kind == AST_DECLARATOR);
	if (d->declarator.direct_declarator && d->declarator
		.direct_declarator->kind != AST_IDENT) return false;
	return d->declarator.pointer >= v->deref_n + 1;
}
static bool type_is_arithmetic(const struct type *t) {
	if (type_has_declarator(t)) return false;
	const struct ast_node *s = t->specifiers;
	const struct vec *ts = NULL;
	if (s->kind == AST_DECLARATION_SPECIFIERS) {
		ts = &s->declaration_specifiers.type_specifiers;
	} else if (s->kind == AST_SPECIFIER_QUALIFIER_LIST) {
		ts = &s->specifier_qualifier_list.type_specifiers;
	}
	assert(ts);
	if (ts->len != 1) return false; // todo: support multiple
	const struct ast_node *bts = *(const struct ast_node **)vec_get_c(ts, 0);
	if (bts->kind != AST_BUILTIN_TYPE) return false;
	switch (bts->builtin_type) {
	case AST_BUILTIN_TYPE_CHAR:
	case AST_BUILTIN_TYPE_SHORT:
	case AST_BUILTIN_TYPE_INT:
	case AST_BUILTIN_TYPE_LONG:
	case AST_BUILTIN_TYPE_FLOAT:
	case AST_BUILTIN_TYPE_DOUBLE:
		return true;
	default:
		return false;
	}
	return false;
}
static struct type type_function_ret(const struct type *t) {
	return (struct type){ .specifiers = t->specifiers, .declarator = NULL };
}

static int get_label(struct state *s) {
	return s->label++;
}
static void put_label(struct state *s, int l) {
	fprintf(s->f, "label_%d:\n", l);
}

static void state_init(struct state *s) {
	hashmap_init(&s->vars, sizeof(struct decl));
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
static val val_push_new(struct state *s, struct type t, const char *reg) {
	s->sp -= 8;
	fprintf(s->f, "mov qword [rbp%d], %s\n", s->sp, reg);
	return (val){ .deref_n = 0, .s = s->sp, .lvalue = false, .t = t };
}

static status cg_gen_expr(struct state *s, const struct ast_node *n, val *res);

#define WARN_NODE(s1, n) do { \
	fprintf(stderr, "%s: `", s1); \
	ast_fprint(stderr, (n), 0); \
	fprintf(stderr, "`\n"); \
} while (0)
#define WARN_MOD_LVALUE(n) WARN_NODE("Error: operand in this expression" \
	"shall be a modifiable lvalue", (n));
static status cg_gen_unary(struct state *s, const struct ast_node *n, val *res) {
	val val_a;
	if (cg_gen_expr(s, n->unary.a, &val_a) == S_ERROR) return S_ERROR;
	switch (n->unary.kind) {
	case AST_PRE_INCR:
		if (!val_modifiable_lvalue(&val_a)) {
			WARN_MOD_LVALUE(n);
			return S_ERROR;
		}
		val_read(s, &val_a, "rax");
		fprintf(s->f, "add rax, 1\n");
		val_store(s, &val_a, "rax");
		*res = val_a;
		return S_OK;
	case AST_PRE_DECR:
		if (!val_modifiable_lvalue(&val_a)) {
			WARN_MOD_LVALUE(n);
			return S_ERROR;
		}
		val_read(s, &val_a, "rax");
		fprintf(s->f, "sub rax, 1\n");
		val_store(s, &val_a, "rax");
		*res = val_a;
		return S_OK;
	case AST_POST_INCR: assert(false); break;
	case AST_POST_DECR: assert(false); break;
	case AST_UNARY_REF:
		if (!val_a.lvalue) {
			WARN_NODE("Error: can't take address of non-lvalue", n);
			return S_ERROR;
		}
		val_a.deref_n--;
		val_a.lvalue = false;
		*res = val_a;
		return S_OK;
	case AST_UNARY_DEREF:
		if (!val_can_deref(&val_a)) {
			WARN_NODE("Error: can't dereference", n);
		}
		val_a.lvalue = true;
		val_a.deref_n++;
		*res = val_a;
		return S_OK;
	case AST_UNARY_PLUS: assert(false); break;
	case AST_UNARY_MINUS: assert(false); break;
	case AST_UNARY_NOT:
		val_read(s, &val_a, "rax");
		fprintf(s->f, "not rax\n");
		*res = val_push_new(s, val_a.t, "rax");
		return S_OK;
	case AST_UNARY_NOTB: assert(false); break;
	case AST_UNARY_SIZEOF: assert(false); break;
	}
	return S_ERROR;
}

static status cg_gen_bin(struct state *s, const struct ast_node *n, val *res) {
	val val_a, val_b;
	if (cg_gen_expr(s, n->bin.a, &val_a) == S_ERROR) return S_ERROR;
	if (cg_gen_expr(s, n->bin.b, &val_b) == S_ERROR) return S_ERROR;
	switch (n->bin.kind) {
	case AST_BIN_MUL: assert(false); break;
	case AST_BIN_DIV: assert(false); break;
	case AST_BIN_MOD: assert(false); break;
	case AST_BIN_ADD:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "add rax, rbx\n");
		*res = val_push_new(s, val_a.t, "rax");
		return S_OK;
	case AST_BIN_SUB:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "sub rax, rbx\n");
		*res = val_push_new(s, val_a.t, "rax");
		return S_OK;
	case AST_BIN_LSHIFT: assert(false); break;
	case AST_BIN_RSHIFT:
		break;
	case AST_BIN_LT:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "setl al\n");
		fprintf(s->f, "movzx rax, al\n");
		*res = val_push_new(s, s->builtin.t_int, "rax");
		return S_OK;
	case AST_BIN_GT: assert(false); break;
	case AST_BIN_LEQ: assert(false); break;
	case AST_BIN_GEQ: assert(false); break;
	case AST_BIN_EQB:
		val_read(s, &val_a, "rax");
		val_read(s, &val_b, "rbx");
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "sete al\n");
		fprintf(s->f, "movzx rax, al\n");
		*res = val_push_new(s, s->builtin.t_int, "rax");
		return S_OK;
	case AST_BIN_NEQ: assert(false); break;
	case AST_BIN_AND: assert(false); break;
	case AST_BIN_XOR: assert(false); break;
	case AST_BIN_OR: assert(false); break;
	case AST_BIN_ANDB: assert(false); break;
	case AST_BIN_ORB: assert(false); break;
	case AST_BIN_ASSIGN:
		val_read(s, &val_b, "rax");
		val_store(s, &val_a, "rax");
		*res = val_a;
		return S_OK;
	case AST_BIN_COMMA: assert(false); break;
	}
	return S_ERROR;
}

static status find_ident(struct state *s, const char *ident, val *res) {
	struct decl *decl;
	if (hashmap_get(&s->vars, ident, (void**)&decl) == MAP_OK) {
		*res = (val){ .deref_n = 0, .s = decl->loc,
			.lvalue = true, .t = decl->t };
		return S_OK;
	} else {
		fprintf(stderr, "Error: undefined identifier `%s`\n", ident);
		return S_ERROR;
	}
}

static status cg_gen_expr(struct state *s, const struct ast_node *n, val *res) {
	switch (n->kind) {
	case AST_IDENT:
		return find_ident(s, n->ident, res);
	case AST_INTEGER:
		fprintf(s->f, "mov rax, %lld\n", n->integer);
		*res = val_push_new(s, s->builtin.t_int, "rax");
		return S_OK;
	case AST_CHARACTER_CONSTANT:
		fprintf(s->f, "mov rax, %d\n", n->character_constant);
		*res = val_push_new(s, s->builtin.t_int, "rax");
		return S_OK;
	case AST_STRING: ;
		int str = vec_append(&s->strings, &n->string);
		fprintf(s->f, "mov rax, s%d\n", str);
		*res = val_push_new(s, s->builtin.t_char_p, "rax");
		return S_OK;
	case AST_INDEX: assert(false);
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
		val func_val;
		if (cg_gen_expr(s, n->call.a, &func_val) == S_ERROR) return S_ERROR;
		const char *func_ident = "<nope>";
		if (n->call.a->kind == AST_IDENT) {
			func_ident = n->call.a->ident;
		}

		struct vec vals = vec_new_empty(sizeof(val));
		for (int i = 0; i < n->call.args.len; ++i) {
			const struct ast_node * const *ni = vec_get_c(&n->call.args, i);
			val v;
			if (cg_gen_expr(s, *ni, &v) == S_ERROR) {
				vec_free(&vals);
				return S_ERROR;
			}
			vec_append(&vals, &v);
		}

		const char *call_regs[] = { "rdi", "rsi", "rdx", "rcx" };
		for (int i = 0; i < vals.len; ++i) {
			val *vi = vec_get(&vals, i);
			assert(i < 4);
			val_read(s, vi, call_regs[i]);
		}
		vec_free(&vals);

		fprintf(s->f, "sub rsp, %d\n", (-s->sp) + (16 + s->sp % 16));
		fprintf(s->f, "call %s\n", func_ident);
		*res = val_push_new(s, type_function_ret(&func_val.t), "rax");
		return S_OK;
	case AST_MEMBER: assert(false); break;
	case AST_MEMBER_DEREF: assert(false); break;
	case AST_UNARY:
		return cg_gen_unary(s, n, res);
	case AST_COMPOUND_LITERAL: assert(false); break;
	case AST_SIZEOF_EXPR:
		fprintf(s->f, "mov rax, %lld\n", 8LL); // TODO: types
		*res = val_push_new(s, s->builtin.t_size_t, "rax");
		return S_OK;
	case AST_ALIGNOF_EXPR: assert(false); break;
	case AST_CAST: assert(false); break;
	case AST_BIN:
		return cg_gen_bin(s, n, res);
	case AST_CONDITIONAL: assert(false); break;
	default: // TODO: we shouldn't be here
		break;
	}
	return S_ERROR;
}

static const char *declarator_get_ident(const struct ast_node *n) {
	if (n == NULL) return NULL;
	switch (n->kind) {
	case AST_IDENT:
		return n->ident;
	case AST_DECLARATOR:
		return declarator_get_ident(n->declarator.direct_declarator);
	case AST_ARRAY_DECLARATOR:
		return declarator_get_ident(n->array_declarator.direct_declarator);
	case AST_FUNCTION_DECLARATOR:
		return declarator_get_ident(n->function_declarator.direct_declarator);
	default:
		assert(false);
		return NULL;
	}
}
static status cg_gen_declaration(struct state *s, const struct ast_node *n) {
	FOR_EACH_NODE(n->declaration.init_declarator_list) {
		const struct ast_node *d = ni->init_declarator.declarator;
		const char *ident = declarator_get_ident(d);
		if (ident) {
			// TODO: type. now we assume literally EVERY type is 64 bit...
			int size = 8;
			s->sp -= size;
			struct decl decl = {
				.t = {
					.specifiers = n->declaration
						.declaration_specifiers,
					.declarator = d,
				},
				.size = size,
				.loc = s->sp,
			};
			hashmap_put(&s->vars, ident, &decl);

			fprintf(stderr, "info: declared identifier `%s` as `",
				ident);
			ast_fprint(stderr, decl.t.specifiers, 0);
			fprintf(stderr, "` `");
			ast_fprint(stderr, decl.t.declarator, 0);
			fprintf(stderr, "`\n");

			if (ni->init_declarator.initializer) {
				val val_init;
				if (cg_gen_expr(s, ni->init_declarator
						.initializer, &val_init)
						== S_ERROR) {
					return S_ERROR;
				}
				val_read(s, &val_init, "rax");
				val val_to;
				if (find_ident(s, ident, &val_to) == S_ERROR) {
					return S_ERROR;
				}
				val_store(s, &val_to, "rax");
			}
		}
	}
	return S_OK;
}

static status cg_gen_stmt_comp(struct state *s, const struct ast_node *n);

static status cg_gen_stmt(struct state *s, const struct ast_node *n) {
	switch (n->kind) {
	case AST_STMT_EXPR: ;
		val ignored_val;
		return cg_gen_expr(s, n->stmt_expr.a, &ignored_val);
	case AST_STMT_WHILE: ;
		int label_start = get_label(s), label_end = get_label(s);
		put_label(s, label_start);
		val val_cond;
		if (cg_gen_expr(s, n->stmt_while.cond, &val_cond) == S_ERROR) {
			return S_ERROR;
		}
		val_read(s, &val_cond, "rax");
		fprintf(s->f, "cmp rax, 0\n");
		fprintf(s->f, "je label_%d\n", label_end);
		if (cg_gen_stmt(s, n->stmt_while.stmt) == S_ERROR) {
			return S_ERROR;
		}
		fprintf(s->f, "jmp label_%d\n", label_start);
		put_label(s, label_end);
		return S_OK;
	case AST_STMT_IF: {
		int label_end = get_label(s);
		val val_cond;
		if (cg_gen_expr(s, n->stmt_if.cond, &val_cond) == S_ERROR) {
			return S_ERROR;
		}
		val_read(s, &val_cond, "rax");
		fprintf(s->f, "cmp rax, 0\n");
		fprintf(s->f, "je label_%d\n", label_end);
		if (cg_gen_stmt(s, n->stmt_if.stmt) == S_ERROR) return S_ERROR;
		// TODO: else
		put_label(s, label_end);
		return S_OK;
	case AST_STMT_COMP:
		return cg_gen_stmt_comp(s, n);
	}
	default: ;
		// TODO: we shouldn't be here
	}
	return S_ERROR;
}

static status cg_gen_stmt_comp(struct state *s, const struct ast_node *n) {
	FOR_EACH_NODE(n->stmt_comp) {
		status st;
		if (ni->kind == AST_DECLARATION) {
			st = cg_gen_declaration(s, ni);
		} else {
			st = cg_gen_stmt(s, ni);
		}
		if (st == S_ERROR) return S_ERROR;
	}
	return S_OK;
}

static status cg_gen_function_definition(struct state *s,
		const struct ast_node *n) {
	const struct ast_node
		*d = n->function_definition.declarator,
		*dd = d->declarator.direct_declarator;
	if (dd->kind != AST_FUNCTION_DECLARATOR) return S_ERROR;
	const struct ast_node
		*i = dd->function_declarator.direct_declarator;
	if (i->kind != AST_IDENT) return S_ERROR;

	fprintf(s->f, "%s:\n", i->ident);
	fprintf(s->f, "push rbp\n");
	fprintf(s->f, "mov rbp, rsp\n");
	if (cg_gen_stmt_comp(s, n->function_definition.compound_statement)
			== S_ERROR) {
		return S_ERROR;
	}
	fprintf(s->f, "mov rsp, rbp\n");
	fprintf(s->f, "pop rbp\n");
	fprintf(s->f, "mov rax, 0\n");
	fprintf(s->f, "ret\n");
	fprintf(s->f, "\n");
	return S_OK;
}

int cg_gen(const struct ast_node *n) {
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

	if (n->kind == AST_TRANSLATION_UNIT) {
		for (int i = 0; i < n->translation_unit.len; ++i) {
			const struct ast_node *ni = *(struct ast_node * const *)
				vec_get_c(&n->translation_unit, i);
			status st = S_ERROR;
			if (ni->kind == AST_DECLARATION) {
				st = cg_gen_declaration(s, ni);
			} else if (ni->kind == AST_FUNCTION_DEFINITION) {
				st = cg_gen_function_definition(s, ni);
			} else {
				assert(false);
			}
			if (st == S_ERROR) return 1;
		}
	}

	fprintf(s->f, "section .rodata\n");
	for (int i = 0; i < s->strings.len; ++i) {
		const char * const *si = vec_get_c(&s->strings, i);
		fprintf(s->f, "s%d: db %s, 0\n", i, *si);
	}

	return 0;
}
