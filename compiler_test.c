{
	// We use this format string trick to print the newline, because escape
	// sequeunces are not yet supported...
	printf("Hello world!%c", 10);

	int b;
	int *a;
	a = &b;
	*a = 5;

	printf("pre inc: %d%c", ++(*a), 10);
	printf("pre dec: %d%c", --(*a), 10);
	printf("not: %d%c", ~(*a), 10);
	printf("add sub: %d%c", (*a) + 1 - 1, 10);
	while (*a < 100) {
		*a = *a + 5;
	}
	if (99 < *a) {
		printf("a: %d%c", *a, 10);
	}
}

