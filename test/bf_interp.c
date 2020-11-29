extern void *malloc(int size);
extern int getchar();
extern int printf(const char *format, ...);
extern void free(void *ptr);

int main() {
	// printf("=== compiler_test.c ===%c", 10);

	int sp = 0;
	int pc = 0;

	int *stack = malloc(65536);
	char *program = malloc(65536);
	int *jumps = malloc(65536);

	int c;
	while ((c = getchar()) < 256) {
		*(program + pc) = c;

		if (c == '[') {
			// push onto stack
			*(stack + sp) = pc;
			sp = sp + sizeof(int);
		}
		if (c == ']') {
			// pop from stack
			sp = sp - sizeof(int);
			int jmp_pc;
			jmp_pc = *(stack + sp);
			*(jumps + pc * sizeof(int)) = jmp_pc;
			*(jumps + jmp_pc * sizeof(int)) = pc;
		}

		pc = pc + sizeof(char);
	}
	*(program + pc + sizeof(char)) = 0;

	int i = 0;
	while (i < 65536) {
		*(stack + i) = 0;
		i = i + sizeof(int);
	}

	int ptr = 0;
	pc = 0;
	while (c = *(program + pc)) {
		if (c == '>') { ptr = ptr + sizeof(int); }
		if (c == '<') { ptr = ptr - sizeof(int); }
		if (c == '+') { ++*(stack + ptr); }
		if (c == '-') { --*(stack + ptr); }
		if (c == '.') { printf("%c", *(stack + ptr)); }
		if (c == ',') { *(stack + ptr) = getchar(); }
		if (c == '[') {
			if (*(stack + ptr) == 0) {
				pc = *(jumps + pc * sizeof(int));
			}
		}
		if (c == ']') {
			if (*(stack + ptr)) {
				pc = *(jumps + pc * sizeof(int));
			}
		}
		pc = pc + sizeof(char);
	}

	free(stack);
	free(program);
	free(jumps);
}
