extern _Noreturn void exit(int exit_code);
int main() {
	int x = 1;
	if (x != 1) exit(1);
	{
		// inner scope
		if (x != 1) exit(1);
		int x = 2;
		if (x != 2) exit(1);
	}
	if (x != 1) exit(1);
}
