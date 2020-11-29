// SPDX-License-Identifier: GPL-3.0-only
#include <ds/hashmap.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <c_compiler/cg.h>

#define container_of(ptr, type, member) \
	(type *)((char *)(ptr) - offsetof(type, member))

#define GETI(x, i) *(struct ast_node * const *)vec_get_c(&(x), i)
#define FOR_EACH_NODE(x) \
	const struct ast_node *ni = GETI(x, 0); \
	for (int i = 0; ni; ni = ((++i < (x).len) ? GETI(x, i): NULL))

struct type {
	bool address_of;
	int app;
	const struct ast_declaration_specifiers *s;
	const struct ast_declarator *d;
};

static void warn_node(const char *msg, const struct ast_node *n) {
	fprintf(stderr, "%s: `", msg);
	ast_fprint(stderr, n, 0);
	fprintf(stderr, "`\n");
}
static void warn_type(const char *msg, const struct type *t) {
	const struct ast_node *ns = container_of(t->s, struct ast_node, declaration_specifiers);
	const struct ast_node *nd = container_of(t->d, struct ast_node, declarator);

	fprintf(stderr, "%s: { %sapp: %d, `", msg,
		t->address_of ? "&, " : "", t->app);
	ast_fprint(stderr, ns, 0);
	fprintf(stderr, "` `");
	ast_fprint(stderr, nd, 0);
	fprintf(stderr, "` }\n");
}

struct builtin_types {
	struct ast_node *spec_int, *spec_char;
	struct ast_node *p;
	struct ast_node *decl_p;
	struct ast_node *decl_empty;
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

static status const_eval(const struct ast_node *n, int *res) {
	if (n->kind == AST_INTEGER) {
		*res = n->integer;
		return S_OK;
	}
	warn_node("error: can't eval const expression", n);
	return S_ERROR;
}

static bool type_all_applied(const struct type *t) {
	return !t->address_of && t->app == t->d->v.len;
}
static bool type_is_const(const struct type *t) {
	if (type_all_applied(t)) {
		return t->s->type_qualifiers[AST_TYPE_QUALIFIER_CONST] > 0;
	} else if (t->address_of) {
		// cant modify the result of the address-of operator
		return true;
	} else {
		const struct ast_node *n =
			*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
		if (n->kind == AST_FUNCTION_DECLARATOR
				|| n->kind == AST_ARRAY_DECLARATOR) {
			// we can't modify arrays or functions
			return true;
		}
		assert(n->kind == AST_POINTER_DECLARATOR);
		return n->pointer_declarator
			.type_qualifiers[AST_TYPE_QUALIFIER_CONST] > 0;
	}
}
static bool val_modifiable_lvalue(const val *v) {
	return v->lvalue && !type_is_const(&v->t);
}
static bool type_is_pointer(struct type *t) {
	if (type_all_applied(t)) return false;
	if (t->address_of) return true;
	const struct ast_node *n =
		*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
	return n->kind == AST_POINTER_DECLARATOR;
}
static bool type_apply_address_of(struct type *t) {
	if (t->address_of) {
		warn_type("can't take address of", t);
		return false;
	}
	t->address_of = true;
	return true;
}
static bool type_apply_deref(struct type *t) {
	if (t->address_of) {
		t->address_of = false;
		return true;
	}
	if (type_all_applied(t)) goto error;
	const struct ast_node *n =
		*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
	if (n->kind == AST_POINTER_DECLARATOR) {
		t->app++;
		return true;
	}
error:
	warn_type("can't apply dereference operator", t);
	return false;
}
static bool type_apply_call(struct type *t) {
	if (t->address_of) goto error;
	if (type_all_applied(t)) goto error;
	const struct ast_node *n =
		*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
	if (n->kind == AST_FUNCTION_DECLARATOR) {
		t->app++;
		return true;
	}
error:
	warn_type("can't call", t);
	return false;
}
static bool type_apply_array(struct type *t) {
	if (t->address_of) goto error;
	if (type_all_applied(t)) return false;
	const struct ast_node *n =
		*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
	if (n->kind == AST_ARRAY_DECLARATOR) {
		t->app++;
		return true;
	}
error:
	warn_type("can't apply array subscripting", t);
	return false;
}
static bool type_is_arithmetic(const struct type *t) {
	if (!type_all_applied(t)) return false;
	// TODO: check builtin types
	return true;
}

static status type_get_size(const struct type *t, int *res) {
	if (t->address_of) {
		return *res = 8, S_OK;
	}
	if (type_all_applied(t)) {
		const char *bs = t->s->builtin_type_specifiers;
		if (bs[AST_BUILTIN_TYPE_CHAR]) return *res = 1, S_OK;
		if (bs[AST_BUILTIN_TYPE_INT]) return *res = 4, S_OK;
		if (bs[AST_BUILTIN_TYPE_VOID]) return *res = 0, S_OK;
		goto error;
	}
	const struct ast_node *n =
		*(const struct ast_node **)vec_get_c(&t->d->v, t->app);
	if (n->kind == AST_POINTER_DECLARATOR) return *res = 8, S_OK;
	if (n->kind == AST_ARRAY_DECLARATOR) {
		int array_n, tt_res;
		if (const_eval(n->array_declarator.size, &array_n) == S_ERROR)
			goto error;
		struct type tt = *t;
		if (!type_apply_array(&tt)) goto error;
		if (type_get_size(&tt, &tt_res) == S_ERROR) goto error;
		*res = array_n * tt_res;
		return S_OK;
	}
	if (n->kind == AST_FUNCTION_DECLARATOR) {
		// here we are supposed to automatically apply the address-of
		// operator
		return *res = 8, S_OK;
	}
	assert(false);
error:
	warn_type("can't determine size of", t);
	return S_ERROR;
}
static struct type type_from_typename(struct ast_node *n) {
	assert(n->kind == AST_TYPE_NAME);
	return (struct type){
		.app = 0,
		.s = &n->type_name.specifier_qualifier_list->declaration_specifiers,
		.d = &n->type_name.declarator->declarator
	};
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

	s->builtin.spec_int = ast_declaration_specifiers();
	s->builtin.spec_int->declaration_specifiers.builtin_type_specifiers[AST_BUILTIN_TYPE_INT]++;
	s->builtin.spec_char = ast_declaration_specifiers();
	s->builtin.spec_char->declaration_specifiers.builtin_type_specifiers[AST_BUILTIN_TYPE_CHAR]++;

	s->builtin.decl_empty = ast_declarator_begin(NULL);
	s->builtin.p = ast_alloc((struct ast_node){
		.kind = AST_POINTER_DECLARATOR,
	});
	s->builtin.decl_p = ast_declarator_begin(NULL);
	vec_append(&s->builtin.decl_p->declarator.v, &s->builtin.p);

	s->builtin.t_int = (struct type){
		.s = &s->builtin.spec_int->declaration_specifiers,
		.d = &s->builtin.decl_empty->declarator,
	};
	s->builtin.t_char = (struct type){
		.s = &s->builtin.spec_char->declaration_specifiers,
		.d = &s->builtin.decl_empty->declarator,
	};
	s->builtin.t_char_p = (struct type){
		.s = &s->builtin.spec_char->declaration_specifiers,
		.d = &s->builtin.decl_p->declarator,
	};
	s->builtin.t_size_t = s->builtin.t_int; // TODO
}

static const char *get_mov_size(int size) {
	if (size == 1) return "byte";
	if (size == 4) return "dword";
	if (size == 8) return "qword";
	return NULL;
}
static const char *get_reg(int i, int size) {
	if (i == 0) {
		if (size == 1) return "al";
		if (size == 4) return "eax";
		if (size == 8) return "rax";
	} else if (i == 1) {
		if (size == 1) return "bl";
		if (size == 4) return "ebx";
		if (size == 8) return "rbx";
	}
	return NULL;
}
static status val_read(struct state *s, const val *v, int regi) {
	int size;
	if (type_get_size(&v->t, &size) == S_ERROR) return S_ERROR;
	if (size == 0) {
		warn_type("can't read void type", &v->t);
		return S_ERROR;
	}
	const char *regs = get_reg(regi, size);
	const char *movs = get_mov_size(size);
	if (!regs || !movs) return S_ERROR;

	if (size != 8) {
		const char *regs8 = get_reg(regi, 8);
		fprintf(s->f, "xor %s, %s\n", regs8, regs8);
	}

	if (v->deref_n == 0) {
		fprintf(s->f, "mov %s, %s [rbp%lld] ; read\n", regs, movs, v->s);
		return S_OK;
	} else if (v->deref_n > 0) {
		fprintf(s->f, "; read (deref_n=%d) {\n", v->deref_n);
		// dereference pointer recursively
		fprintf(s->f, "mov rcx, qword [rbp%lld]\n", v->s);
		for (int i = 0; i < v->deref_n - 1; ++i) {
			fprintf(s->f, "mov rcx, [rcx]\n");
		}
		fprintf(s->f, "mov %s, %s [rcx]\n", regs, movs);
		fprintf(s->f, "; }\n");
		return S_OK;
	} else {
		assert(false);
	}
	return S_ERROR;
}
static status val_store(struct state *s, const val *v, int regi) {
	int size;
	if (type_get_size(&v->t, &size) == S_ERROR) return S_ERROR;
	if (size == 0) {
		warn_type("can't read void type", &v->t);
		return S_ERROR;
	}
	const char *regs = get_reg(regi, size);
	const char *movs = get_mov_size(size);
	if (!regs || !movs) return S_ERROR;

	if (v->deref_n == 0) {
		fprintf(s->f, "mov %s [rbp%lld], %s ; store\n", movs, v->s, regs);
		return S_OK;
	} else if (v->deref_n > 0) {
		fprintf(s->f, "; store (deref_n=%d) {\n", v->deref_n);
		fprintf(s->f, "mov rcx, qword [rbp%lld]\n", v->s);
		for (int i = 0; i < v->deref_n - 1; ++i) {
			fprintf(s->f, "mov rcx, [rcx]\n");
		}
		fprintf(s->f, "mov %s [rcx], %s\n", movs, regs);
		fprintf(s->f, "; }\n");
		return S_OK;
	} else {
		assert(false);
	}
}
static status val_push_new(struct state *s, struct type t, int regi, val *vres) {
	s->sp -= 8;
	*vres = (val){ .deref_n = 0, .s = s->sp, .lvalue = false, .t = t };
	if (val_store(s, vres, regi) == S_ERROR) return S_ERROR;
	return S_OK;
}

static status cg_gen_expr(struct state *s, const struct ast_node *n, val *res);

#define WARN_MOD_LVALUE(n) warn_node("Error: operand in this expression" \
	" shall be a modifiable lvalue", (n));
static status cg_gen_unary(struct state *s, const struct ast_node *n, val *res) {
	val val_a;
	if (cg_gen_expr(s, n->unary.a, &val_a) == S_ERROR) return S_ERROR;
	switch (n->unary.kind) {
	case AST_PRE_INCR:
		if (!val_modifiable_lvalue(&val_a)) {
			WARN_MOD_LVALUE(n);
			return S_ERROR;
		}
		val_read(s, &val_a, 0);
		fprintf(s->f, "add rax, 1\n");
		val_store(s, &val_a, 0);
		*res = val_a;
		return S_OK;
	case AST_PRE_DECR:
		if (!val_modifiable_lvalue(&val_a)) {
			WARN_MOD_LVALUE(n);
			return S_ERROR;
		}
		val_read(s, &val_a, 0);
		fprintf(s->f, "sub rax, 1\n");
		val_store(s, &val_a, 0);
		*res = val_a;
		return S_OK;
	case AST_POST_INCR: assert(false); break;
	case AST_POST_DECR: assert(false); break;
	case AST_UNARY_REF:
		if (!val_a.lvalue) {
			warn_node("Error: can't take address of non-lvalue", n);
			return S_ERROR;
		}
		if (!type_apply_address_of(&val_a.t)) return S_ERROR;
	 	fprintf(s->f, "mov rax, rbp\n");
	 	fprintf(s->f, "sub rax, %lld\n", -val_a.s);
		return val_push_new(s, val_a.t, 0, res);
	case AST_UNARY_DEREF:
		if (!type_apply_deref(&val_a.t)) return S_ERROR;
		val_a.lvalue = true;
		val_a.deref_n++;
		*res = val_a;
		return S_OK;
	case AST_UNARY_PLUS: assert(false); break;
	case AST_UNARY_MINUS: assert(false); break;
	case AST_UNARY_NOT:
		val_read(s, &val_a, 0);
		fprintf(s->f, "not rax\n");
		return val_push_new(s, val_a.t, 0, res);
	case AST_UNARY_NOTB: assert(false); break;
	case AST_UNARY_SIZEOF: assert(false); break;
	}
	return S_ERROR;
}

static status cg_gen_bin(struct state *s, const struct ast_node *n, val *res) {
	val val_a, val_b;
	if (cg_gen_expr(s, n->bin.a, &val_a) == S_ERROR) return S_ERROR;
	if (cg_gen_expr(s, n->bin.b, &val_b) == S_ERROR) return S_ERROR;
	bool
		a_ptr = type_is_pointer(&val_a.t),
		b_ptr = type_is_pointer(&val_b.t),
		a_arith = type_is_arithmetic(&val_a.t),
		b_arith = type_is_arithmetic(&val_b.t);
	switch (n->bin.kind) {
	case AST_BIN_MUL:
		val_read(s, &val_a, 0);
		val_read(s, &val_b, 1);
		fprintf(s->f, "imul eax, ebx\n");
		return val_push_new(s, val_a.t, 0, res);
	case AST_BIN_DIV: assert(false); break;
	case AST_BIN_MOD: assert(false); break;
	case AST_BIN_ADD: ;
		val_read(s, &val_a, 0);
		val_read(s, &val_b, 1);
		fprintf(s->f, "add rax, rbx\n");
		if (a_arith && b_arith) {
			// ok, both arithmetic
			return val_push_new(s, val_a.t, 0, res);
		} else if (a_ptr && b_arith) {
			return val_push_new(s, val_a.t, 0, res);
		} else if (a_arith && b_ptr) {
			return val_push_new(s, val_b.t, 0, res);
		}
		warn_node("Cant add operands", n);
		return S_ERROR;
	case AST_BIN_SUB:
		val_read(s, &val_a, 0);
		val_read(s, &val_b, 1);
		fprintf(s->f, "sub rax, rbx\n");
		if (a_arith && b_arith) {
			// ok, both arithmetic
			return val_push_new(s, val_a.t, 0, res);
		} else if (a_ptr && b_ptr) {
			// ok, pointers can have difference
			return val_push_new(s, val_a.t, 0, res);
		} else if (a_ptr && b_arith) {
			// ok, can subtract from pointer
			return val_push_new(s, val_a.t, 0, res);
		}
		warn_node("Cant subtract operands", n);
		return S_ERROR;
	case AST_BIN_LSHIFT: assert(false); break;
	case AST_BIN_RSHIFT:
		break;
	case AST_BIN_LT:
		val_read(s, &val_a, 0);
		val_read(s, &val_b, 1);
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "setl al\n");
		fprintf(s->f, "movzx rax, al\n");
		return val_push_new(s, s->builtin.t_int, 0, res);
	case AST_BIN_GT: assert(false); break;
	case AST_BIN_LEQ: assert(false); break;
	case AST_BIN_GEQ: assert(false); break;
	case AST_BIN_EQB:
		val_read(s, &val_a, 0);
		val_read(s, &val_b, 1);
		fprintf(s->f, "cmp rax, rbx\n");
		fprintf(s->f, "sete al\n");
		fprintf(s->f, "movzx rax, al\n");
		return val_push_new(s, s->builtin.t_int, 0, res);
		return S_OK;
	case AST_BIN_NEQ: assert(false); break;
	case AST_BIN_AND: assert(false); break;
	case AST_BIN_XOR: assert(false); break;
	case AST_BIN_OR: assert(false); break;
	case AST_BIN_ANDB: assert(false); break;
	case AST_BIN_ORB: assert(false); break;
	case AST_BIN_ASSIGN:
		val_read(s, &val_b, 0);
		val_store(s, &val_a, 0);
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
		return val_push_new(s, s->builtin.t_int, 0, res);
		return S_OK;
	case AST_CHARACTER_CONSTANT:
		fprintf(s->f, "mov rax, %d\n", n->character_constant);
		return val_push_new(s, s->builtin.t_int, 0, res);
		return S_OK;
	case AST_STRING: ;
		int str = vec_append(&s->strings, &n->string);
		fprintf(s->f, "mov rax, s%d\n", str);
		return val_push_new(s, s->builtin.t_char_p, 0, res);
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
		if (cg_gen_expr(s, n->call.a, &func_val) == S_ERROR)
			return S_ERROR;
		if (!type_apply_call(&func_val.t)) return S_ERROR;
		int f_res_size;
		if (type_get_size(&func_val.t, &f_res_size) == S_ERROR)
			return S_ERROR;
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

		const char *call_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
		for (int i = 0; i < vals.len; ++i) {
			val *vi = vec_get(&vals, i);
			assert(i < sizeof(call_regs) / sizeof(call_regs[0]));
			val_read(s, vi, 0);
			fprintf(s->f, "mov %s, rax\n", call_regs[i]);
		}
		vec_free(&vals);

		fprintf(s->f, "sub rsp, %d\n", (-s->sp) + (16 + s->sp % 16));
		fprintf(s->f, "call %s\n", func_ident);
		if (f_res_size > 0) {
			return val_push_new(s, func_val.t, 0, res);
		} else {
			*res = (val){ .s = 1, .t = func_val.t };
			return S_OK;
		}
	case AST_MEMBER: assert(false); break;
	case AST_MEMBER_DEREF: assert(false); break;
	case AST_UNARY:
		return cg_gen_unary(s, n, res);
	case AST_COMPOUND_LITERAL: assert(false); break;
	case AST_SIZEOF_EXPR: ;
		struct type t = type_from_typename(n->sizeof_expr.type_name);
		int size;
		if (type_get_size(&t, &size) == S_ERROR) return S_ERROR;
		fprintf(s->f, "mov rax, %d\n", size);
		return val_push_new(s, s->builtin.t_size_t, 0, res);
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

static status cg_gen_declaration(struct state *s, const struct ast_node *n) {
	FOR_EACH_NODE(n->declaration.init_declarator_list) {
		const struct ast_node *d = ni->init_declarator.declarator;
		const char *ident = d->declarator.ident->ident;
		struct ast_declaration_specifiers *ds = &n->declaration
			.declaration_specifiers->declaration_specifiers;
		bool ext = ds->storage_class_specifiers
			[AST_STORAGE_CLASS_SPECIFIER_EXTERN] > 0;
		if (ident) {
			if (ext) {
				fprintf(s->f, "extern %s\n", ident);
			}
			int size = 8;
			int loc = ext ? 1 : (s->sp -= size);
			struct decl decl = {
				.t = { .s = ds, .d = &d->declarator },
				.size = size,
				.loc = loc,
			};
			hashmap_put(&s->vars, ident, &decl);

			fprintf(s->f, "; alloced `%s` on stack at %d\n",
				ident, decl.loc);

			fprintf(stderr, "info: declared identifier `%s` as `",
				ident);
			ast_fprint(stderr, n->declaration.declaration_specifiers, 0);
			fprintf(stderr, "` `");
			ast_fprint(stderr, d,  0);
			fprintf(stderr, "`\n");

			if (ni->init_declarator.initializer) {
				val val_init;
				if (cg_gen_expr(s, ni->init_declarator
						.initializer, &val_init)
						== S_ERROR) {
					return S_ERROR;
				}
				val_read(s, &val_init, 0);
				val val_to;
				if (find_ident(s, ident, &val_to) == S_ERROR) {
					return S_ERROR;
				}
				val_store(s, &val_to, 0);
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
		val_read(s, &val_cond, 0);
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
		val_read(s, &val_cond, 0);
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
	const struct ast_declarator *d = &n->function_definition.declarator->declarator;
	const char *ident = d->ident->ident;

	fprintf(s->f, "%s:\n", ident);
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
