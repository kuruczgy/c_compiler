int main() {
	// printf("=== compiler_test.c ===%c", 10);

	int sp = 0;
	int pc = 0;

	char *stack = malloc(65536);
	char *program = malloc(65536);
	char *jumps = malloc(65536);

	int c;
	while ((c = getchar()) < 256) {
		*(program + pc) = c;

		if (c == '[') {
			// push onto stack
			*(stack + sp) = pc;
			sp = sp + 8;
		}
		if (c == ']') {
			// pop from stack
			sp = sp - 8;
			int jmp_pc;
			jmp_pc = *(stack + sp);
			*(jumps + pc) = jmp_pc;
			*(jumps + jmp_pc) = pc;
		}

		pc = pc + 8;
	}
	*(program + pc + 8) = 0;

	int i = 0;
	while (i < 65536) {
		*(stack + i) = 0;
		i = i + 8;
	}

	int ptr = 0;
	pc = 0;
	while (c = *(program + pc)) {
		if (c == '>') { ptr = ptr + 8; }
		if (c == '<') { ptr = ptr - 8; }
		if (c == '+') { ++*(stack + ptr); }
		if (c == '-') { --*(stack + ptr); }
		if (c == '.') { printf("%c", *(stack + ptr)); }
		if (c == ',') { *(stack + ptr) = getchar(); }
		if (c == '[') { if (*(stack + ptr) == 0) { pc = *(jumps + pc); } }
		if (c == ']') { if (*(stack + ptr)) { pc = *(jumps + pc); } }
		pc = pc + 8;
	}

	free(stack);
	free(program);
	free(jumps);
}
