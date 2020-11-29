extern void *malloc(int size);
extern int printf(const char *format, ...);

int main() {
	int a = 5;
	int *ap = &a;
	*ap = 1;
	printf("%d ", *ap);

	int **app = &ap;
	**app = 2;
	printf("%d ", **app);

	char *mem = malloc(8);
	*(mem + 0) = '3';
	*(mem + 1) = ' ';
	*(mem + 1) = 10;
	*(mem + 2) = 0;

	printf("%s", mem);
}
