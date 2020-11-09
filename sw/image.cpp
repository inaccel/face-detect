/*
 *  TU Eindhoven
 *  Eindhoven, The Netherlands
 *
 *  Name            :   image.c
 *
 *  Author          :   Francesco Comaschi (f.comaschi@tue.nl)
 *
 *  Date            :   November 12, 2012
 *
 *  Function        :   Functions to manage .pgm images and integral images
 *
 *  History         :
 *      12-11-12    :   Initial version.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  If not, see <http://www.gnu.org/licenses/>
 *
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Happy coding!
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "image.h"
#include "stdio-wrapper.h"

char* strrev(char* str)
{
	char *p1, *p2;
	if (!str || !*str)
		return str;
	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}

//int chartoi(const char *string)
//{
//	int i;
//	i=0;
//	while(*string)
//	{
//		// i<<3 is equivalent of multiplying by 2*2*2 or 8
//		// so i<<3 + i<<1 means multiply by 10
//		i=(i<<3) + (i<<1) + (*string - '0');
//		string++;
//
//		// Dont increment i!
//
//	}
//	return(i);
//}

int myatoi (char* string)
{
	int sign = 1;
	// how many characters in the string
	int length = strlen(string);
	int i = 0;
	int number = 0;

	// handle sign
	if (string[0] == '-')
	{
		sign = -1;
		i++;
	}

//	for (i; i < length; i++)
	while(i < length)
	{
		// handle the decimal place if there is one
		if (string[i] == '.')
			break;
		number = number * 10 + (string[i]- 48);
		i++;
	}

	number *= sign;

	return number;
}

void itochar(int x, char* szBuffer, int radix)
{
	int i = 0, n, xx;
	n = x;
	while (n > 0)
	{
		xx = n%radix;
		n = n/radix;
		szBuffer[i++] = '0' + xx;
	}
	szBuffer[i] = '\0';
	strrev(szBuffer);
}

void createImage(int width, int height, MyImage *image)
{
	image->width = width;
	image->height = height;
	image->flag = 1;
	image->data = (unsigned char *)malloc(sizeof(unsigned char)*(height*width));
}

void createSumImage(int width, int height, MyIntImage *image)
{
	image->width = width;
	image->height = height;
	image->flag = 1;
	image->data = (int *)malloc(sizeof(int)*(height*width));
}

int freeImage(MyImage* image)
{
	if (image->flag == 0)
	{
		printf("no image to delete\n");
		return -1;
	}
	else
	{
//		printf("image deleted\n");
		free(image->data); 
		return 0;
	}
}

int freeSumImage(MyIntImage* image)
{
	if (image->flag == 0)
	{
		printf("no image to delete\n");
		return -1;
	}
	else
	{
//		printf("image deleted\n");
		free(image->data); 
		return 0;
	}
}

void setImage(int width, int height, MyImage *image)
{
	image->width = width;
	image->height = height;
}

void setSumImage(int width, int height, MyIntImage *image)
{
	image->width = width;
	image->height = height;
}
