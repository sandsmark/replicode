#include "compatibility.h"
#include <stdio.h>
#include <string.h>

int itoa(int n, char* buffer, int base)
{
	if (base == 10)
		return sprintf(buffer, "%d", n);
	else if (base == 8)
		return sprintf(buffer, "%o", n);
	else if (base == 16)
		return sprintf(buffer, "%x", n);
	else {
		strcat(buffer, "BADBASE");
		return 7;
	}
}
