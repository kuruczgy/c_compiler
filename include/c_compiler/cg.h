// SPDX-License-Identifier: GPL-3.0-only
#ifndef C_COMPILER_CG_H
#define C_COMPILER_CG_H
#include <c_compiler/ast.h>

void cg_gen(const struct ast_node *n);

#endif
