#include "dct.h"

#define PI_DIV_16 0.196349540849 // PI / 16
#define ONE_DIV_SQRT2 0.707106781187 // 1 / sqrt(2)

void dct_2d_8x8(float in[8][8], float out[8][8])
{
	// s - sum of spatial freq
	// k - coefficient. k is different for each first spatial value
	float s, k;
	float tmp;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			s = 0;

			// dct[x][y] = sum( (spatial freq in x dir) * (spatial freq in y dir) );
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					tmp = in[i][j];
					tmp *= cos((2 * i + 1) * x * PI_DIV_16);
					tmp *= cos((2 * j + 1) * y * PI_DIV_16);
					s += tmp;

				}
			}

			if (x == 0 && y == 0)
				k = 0.5;
			else if ((x == 0 && y != 0) || (y == 0 && x != 0))
				k = ONE_DIV_SQRT2;
			else
				k = 1.0;

			out[x][y] = s * k * 0.25;
		}
	}
}

void dct_2d(float in[64], float out[64]){
	// s - sum of spatial freq
	// k - coefficient. k is different for each first spatial value
	float s, k;
	float tmp;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			s = 0;

			// dct[x][y] = sum( (spatial freq in x dir) * (spatial freq in y dir) );
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					tmp = in[i * 8 + j];
					tmp *= cos((2 * i + 1) * x * PI_DIV_16);
					tmp *= cos((2 * j + 1) * y * PI_DIV_16);
					s += tmp;

				}
			}

			if (x == 0 && y == 0)
				k = 0.5;
			else if ((x == 0 && y != 0) || (y == 0 && x != 0))
				k = ONE_DIV_SQRT2;
			else
				k = 1.0;

			out[x * 8 + y] = s * k * 0.25;
		}
	}
}

void inverse_dct_2d_8x8(float in[8][8], float out[8][8])
{
	float s, k;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{

			s = 0;
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == 0 && j == 0)
						k = 0.5;
					else if ((i == 0 && j != 0) || (i != 0 && j == 0))
						k = 1 / sqrt(2.0);
					else
						k = 1.0;

					s += k * in[i][j] * cos((2 * x + 1) * M_PI * i / 16) * cos((2 * y + 1) * M_PI * j / 16);
				}
			}

			out[x][y] = s * 2 / 8;
		}
	}
}

void dct_1d(float in[8], float out[8])
{
	float K, sum;
	for (int i = 0; i < 8; i++)
	{
		K = (i == 0) ? 1 / sqrt(2.0) : 1;
		sum = .0;
		for (int j = 0; j < 8; j++)
		{
			sum += K * in[j] * cos(M_PI * (j + .5) * i / 8.0);
		}

		out[i] = sum * sqrt(1.0 / 4.0);
	}
}