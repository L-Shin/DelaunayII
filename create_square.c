#include <stdio.h>

int main() 
{
	FILE *fp;
	fp = fopen("orderlygrid.node", "w");
	int i, j, k;
	double x, y;
	x = 0.0;
	k = 0;
	// header
	fprintf(fp, "10000 2 0 0\n");
	for (i = 0; i < 100; i++) {
		y = 0.0;
		for (j = 0; j < 100; j++) {
			fprintf(fp, "%d %f %f\n", k, x, y);
			k++;
			y = y + 1.0;
		}
		x = x + 1.0;
	}		 
	fclose(fp);
	return 0;
}
