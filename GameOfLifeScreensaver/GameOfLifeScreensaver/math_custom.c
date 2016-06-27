#include "math_custom.h"
#include <Windows.h>

int Greatest(int num, int num2)
{
	if (num > num2) return num;
	return num2;
}

int Least(int num, int num2)
{
	if (num < num2) return num;
	return num2;
}

int Difference(int num, int num2)
{
	return Greatest(num, num2) - Least(num, num2);
}

int Abs(int val)
{
	if (val < 0) return -val;
	return val;
}

int GetColor(double* angle)
{
	*angle += 0.05;
	if (*angle > 2 * M_PI)
		*angle = -(2 * M_PI);
	return RGB(sin(*angle + 0) * 127 + 128, sin(*angle + 2) * 127 + 128, sin(*angle + 4) * 127 + 128);
}