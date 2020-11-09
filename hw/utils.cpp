/*===============================================================*/
/*                                                               */
/*                          utils.cpp                            */
/*                                                               */
/*                       Utility functions                       */
/*                                                               */
/*===============================================================*/

#include <getopt.h>
#include <iostream>
#include <string>

#include "utils.h"

void print_usage(char* filename) {
	std::cout << "usage: " << filename << " <options>\n";
	std::cout << "  -o [output file]\n";
}

void parse_command_line_args(int argc, char** argv, std::string& outFile) {
	int c = 0;

	while ((c = getopt(argc, argv, "o:")) != -1) {
		switch (c) {
			case 'i':
				outFile = optarg;
				break;
			case 'o':
				outFile = optarg;
				break;
			default: {
				print_usage(argv[0]);
				exit(-1);
			}
		} // matching on arguments
	} // while args present
}

void drawRectangles(int result_size, int *result_x, int *result_y, int *result_w, int *result_h, unsigned char *frame) {
	for (int i = 0; i < std::min(result_size, RESULT_SIZE); i++) {
		int x = result_x[i];
		int y = result_y[i];
		int w = result_w[i];
		int h = result_h[i];

		for(int k = x; k < x + w; k++) {
			frame[y * IMAGE_WIDTH + k] = 255;
			frame[(y + h) * IMAGE_WIDTH + k] = 255;
		}

		for(int k = y; k < y + h; k++) {
			frame[k * IMAGE_WIDTH + x] = 255;
			frame[k * IMAGE_WIDTH + (x + w)] = 255;
		}
	}
}
