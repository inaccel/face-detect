#ifndef RECTANGLES
#define RECTANGLES

#include <inaccel/coral>

void groupRectangles(inaccel::vector<int>& rectList_x, inaccel::vector<int>& rectList_y,
					 inaccel::vector<int>& rectList_w, inaccel::vector<int>& rectList_h,
					 int size, int groupThreshold, float eps);

#endif
