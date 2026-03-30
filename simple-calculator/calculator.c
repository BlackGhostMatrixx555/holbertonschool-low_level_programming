#include <stdio.h>

/**
 * print_menu - Prints the calculator menu
 */
void print_menu(void)
{
	printf("Simple Calculator\n");
	printf("1) Add\n");
	printf("2) Subtract\n");
	printf("3) Multiply\n");
	printf("4) Divide\n");
	printf("0) Quit\n");
}

/**
 * get_operands - Reads two float operands from the user
 * @a: Pointer to the first operand
 * @b: Pointer to the second operand
 */
void get_operands(float *a, float *b)
{
	printf("A: ");
	scanf("%f", a);
	printf("B: ");
	scanf("%f", b);
}

/**
 * do_add - Performs addition and prints the result
 */
void do_add(void)
{
	float a, b;

	get_operands(&a, &b);
	printf("Result: %g\n", a + b);
}

/**
 * do_subtract - Performs subtraction and prints the result
 */
void do_subtract(void)
{
	float a, b;

	get_operands(&a, &b);
	printf("Result: %g\n", a - b);
}

/**
 * do_multiply - Performs multiplication and prints the result
 */
void do_multiply(void)
{
	float a, b;

	get_operands(&a, &b);
	printf("Result: %g\n", a * b);
}

/**
 * do_divide - Performs division and prints the result or an error
 */
void do_divide(void)
{
	float a, b;

	get_operands(&a, &b);
	if (b == 0)
	{
		printf("Error: division by zero\n");
		return;
	}
	printf("Result: %g\n", a / b);
}

/**
 * main - Entry point of the simple calculator
 *
 * Return: 0 on success
 */
int main(void)
{
	int choice;

	print_menu();
	while (1)
	{
		printf("Choice: ");
		scanf("%d", &choice);
		if (choice == 0)
		{
			printf("Bye!\n");
			break;
		}
		else if (choice == 1)
			do_add();
		else if (choice == 2)
			do_subtract();
		else if (choice == 3)
			do_multiply();
		else if (choice == 4)
			do_divide();
		else
			printf("Invalid choice\n");
	}
	return (0);
}
