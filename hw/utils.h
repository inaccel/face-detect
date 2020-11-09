#ifndef UTILS
#define UTILS

/*===============================================================*/
/*                                                               */
/*                           utils.h                             */
/*                                                               */
/*                       Utility functions                       */
/*                                                               */
/*===============================================================*/

const int IMAGE_HEIGHT = 240;
const int IMAGE_WIDTH = 320;
const int RESULT_SIZE = 100;

void print_usage(char* filename);

void parse_command_line_args(int argc, char** argv, std::string& outFile);

void drawRectangles(int result_size, int *result_x, int *result_y, int *result_w, int *result_h, unsigned char *frame);

#endif
