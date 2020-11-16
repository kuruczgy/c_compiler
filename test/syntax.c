
// global declarations
int global1 = 1, global2 = 2;

// global function declarations
int f(char);
extern static int f(char *argv[static 2]);

// this is only for testing parsing
typedef
extern
static
_Thread_local
auto
register
const
restrict
volatile
_Atomic
inline
_Noreturn
void
char
short
int
long
float
double
signed
unsigned
_Bool
_Complex
_Alignas(0) x;

struct ab {
	int a;
	int b;
	union {
		int x : 10, y : 20;
	} anon;
	_Static_assert(5 < 99999, "");
} hello;
struct ab hello2;

enum asd {
	ASD_A = 1,
	ASD_B = 2,
};

enum asd asd;

int main(int argc, char *argv[]) {
	// We use this format string trick to print the newline, because escape
	// sequeunces are not yet supported...
	printf("Hello world!%c", 10);

labeled: ;
goto labeled;

	switch (argc) {
		case 1: ;
		case 2: ;
		default: ;
	}

	int *arr[5];
	int x = 1, y = 2, z = 3;
	int b = 1 > 2 ? 1 : -1;
	int *a = &b;
	*a = 5;

	struct ab v = { .a = a, .b = b, .anon.x = -1 };
	v = (struct ab) { .anon = { .x = 1, .y = 2 } };
	int x = sizeof(int*);
	int z = alignof(void(*)(int x, int z));

	printf("pre inc: %d%c", ++(*a), 10);
	printf("pre dec: %d%c", --(*a), 10);
	printf("not: %d%c", ~(*a), 10);
	printf("add sub: %d%c", (*a) + 1 - 1, 10);
	while (*a < 100) {
		*a = *a + 5;
	}
	do break; while (1);
	for (int i = 0; i < 5; ++i) --i;
	for (x = 0; x < 10; ++x) { --x; }
	if (99 < *a) {
		printf("a: %d%c", *a, 10);
	} else {
		;
	}

	return 1;
}

