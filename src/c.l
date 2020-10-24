 // SPDX-License-Identifier: GPL-3.0-only
%{
#include <stdlib.h>
#include <c_compiler/ast.h>
#include "c.tab.h"

extern const char *lex_ident;
extern long long int lex_int;

%}

digit [0-9]
nondigit [a-zA-Z_]
whitespace [ \n\t\r]

%%

\/\/.*\n ;

\"[^"\\\n]*\" { lex_ident = yytext; return STRING; }
\'[^'\\\n]\' { lex_int = yytext[1]; return CHARACTER_CONSTANT; }

alignof return ALIGNOF;
auto return AUTO;
break return BREAK;
case return CASE;
char return CHAR;
const return CONST;
continue return CONTINUE;
default return DEFAULT;
do return DO;
double return DOUBLE;
else return ELSE;
enum return ENUM;
extern return EXTERN;
float return FLOAT;
for return FOR;
goto return GOTO;
if return IF;
inline return INLINE;
int return INT;
long return LONG;
register return REGISTER;
restrict return RESTRICT;
return return RETURN;
short return SHORT;
signed return SIGNED;
sizeof return SIZEOF;
static return STATIC;
struct return STRUCT;
switch return SWITCH;
typedef return TYPEDEF;
union return UNION;
unsigned return UNSIGNED;
void return VOID;
volatile return VOLATILE;
while return WHILE;
_Alignas return U_ALIGNAS;
_Atomic return U_ATOMIC;
_Bool return U_BOOL;
_Complex return U_COMPLEX;
_Generic return U_GENERIC;
_Imaginary return U_IMAGINARY;
_Noreturn return U_NORETURN;
_Static_assert return U_STATIC_ASSERT;
_Thread_local return U_THREAD_LOCAL;

{nondigit}({nondigit}|{digit})* { lex_ident = yytext; return IDENT; }

[1-9]{digit}*|0 { lex_int = atoi(yytext); return INTEGER; }

\[ return LSQUARE;
\] return RSQUARE;
\{ return LCURLY;
\} return RCURLY;
\( return LROUND;
\) return RROUND;
\. return DOT;
-> return ARROW;
"++" return INCR;
-- return DECR;
& return AND;
\* return STAR;
\+ return PLUS;
- return MINUS;
~ return NOT;
! return NOTB;
"/" return DIV;
% return MOD;
"<<" return LSHIFT;
">>" return RSHIFT;
"<" return LT;
">" return GT;
"<=" return LEQ;
">=" return GEQ;
== return EQB;
!= return NEQ;
"^" return XOR;
\| return OR;
&& return ANDB;
"||" return ORB;
"?" return QMARK;
: return COLON;
; return SEMI;
"..." return ELLIPSIS;
= return EQ;
, return COMMA;

{whitespace}+ ;

. ;
