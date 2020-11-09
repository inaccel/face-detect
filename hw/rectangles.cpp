#include "rectangles.h"

int partition(inaccel::vector<int>& _vec_x, inaccel::vector<int>& _vec_y,
			  inaccel::vector<int>& _vec_w, inaccel::vector<int>& _vec_h,
			  int size, std::vector<int>& labels, float eps);

int myMax(int a, int b) {
	return (a >= b) ? a : b;
}

int myMin(int a, int b) {
	return (a <= b) ? a : b;
}

inline int myRound(float value) {
	return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

int myAbs(int n) {
	return (n >= 0) ? n : -n;
}

int predicate(float eps, int &r1_x, int &r1_y, int &r1_w, int &r1_h,
							int &r2_x, int &r2_y, int &r2_w, int &r2_h) {
	float delta = eps*(myMin(r1_w, r2_w) + myMin(r1_h, r2_h))*0.5;
	return myAbs(r1_x - r2_x) <= delta &&
		myAbs(r1_y - r2_y) <= delta &&
		myAbs(r1_x + r1_w - r2_x - r2_w) <= delta &&
		myAbs(r1_y + r1_h - r2_y - r2_h) <= delta;
}

void groupRectangles(inaccel::vector<int>& rectList_x, inaccel::vector<int>& rectList_y,
					 inaccel::vector<int>& rectList_w, inaccel::vector<int>& rectList_h,
					 int size,
					 int groupThreshold, float eps) {

	if(groupThreshold <= 0 || rectList_x.empty() || rectList_y.empty()
						   || rectList_w.empty() || rectList_h.empty()) return;

	std::vector<int> labels;

	int nclasses = partition(rectList_x, rectList_y, rectList_w, rectList_h, size, labels, eps);

	std::vector<int> rrects_x(nclasses);
	std::vector<int> rrects_y(nclasses);
	std::vector<int> rrects_w(nclasses);
	std::vector<int> rrects_h(nclasses);
	std::vector<int> rweights(nclasses);

	int i, j, nlabels = (int)labels.size();


	for( i = 0; i < nlabels; i++ ) {
		int cls = labels[i];
		rrects_x[cls] += rectList_x[i];
		rrects_y[cls] += rectList_y[i];
		rrects_w[cls] += rectList_w[i];
		rrects_h[cls] += rectList_h[i];
		rweights[cls]++;
	}

	for( i = 0; i < nclasses; i++ ) {
		float s = 1.f/rweights[i];
		rrects_x[i] = myRound(rrects_x[i] * s);
		rrects_y[i] = myRound(rrects_y[i] * s);
		rrects_w[i] = myRound(rrects_w[i] * s);
		rrects_h[i] = myRound(rrects_h[i] * s);
	}

	rectList_x.clear();
	rectList_y.clear();
	rectList_w.clear();
	rectList_h.clear();

	for( i = 0; i < nclasses; i++ ) {
		int r1_x = rrects_x[i];
		int r1_y = rrects_y[i];
		int r1_w = rrects_w[i];
		int r1_h = rrects_h[i];
		int n1 = rweights[i];
		if( n1 <= groupThreshold ) continue;
		/* filter out small face rectangles inside large rectangles */
		for( j = 0; j < nclasses; j++ ) {
			int n2 = rweights[j];
			/*********************************
			 * if it is the same rectangle,
			 * or the number of rectangles in class j is < group threshold,
			 * do nothing
			 ********************************/
			if( j == i || n2 <= groupThreshold ) continue;
			int r2_x = rrects_x[j];
			int r2_y = rrects_y[j];
			int r2_w = rrects_w[j];
			int r2_h = rrects_h[j];

			int dx = myRound( r2_w * eps );
			int dy = myRound( r2_h * eps );

			if( i != j &&
				r1_x >= r2_x - dx &&
				r1_y >= r2_y - dy &&
				r1_x + r1_w <= r2_x + r2_w + dx &&
				r1_y + r1_h <= r2_y + r2_h + dy &&
				(n2 > myMax(3, n1) || n1 < 3) )
				break;
		}

		if( j == nclasses ) {
			rectList_x.push_back(r1_x); // insert back r1_x
			rectList_y.push_back(r1_y); // insert back r1_y
			rectList_w.push_back(r1_w); // insert back r1_w
			rectList_h.push_back(r1_h); // insert back r1_h
		}
	}
}


int partition(inaccel::vector<int>& _vec_x, inaccel::vector<int>& _vec_y,
			  inaccel::vector<int>& _vec_w, inaccel::vector<int>& _vec_h,
			  int size,
			  std::vector<int>& labels, float eps) {
	int i, j, N = size;

	int *vec_x = _vec_x.data();
	int *vec_y = _vec_y.data();
	int *vec_w = _vec_w.data();
	int *vec_h = _vec_h.data();

	const int PARENT=0;
	const int RANK=1;

	std::vector<int> _nodes(N*2);

	int (*nodes)[2] = (int(*)[2])&_nodes[0];

	/* The first O(N) pass: create N single-vertex trees */
	for(i = 0; i < N; i++) {
		nodes[i][PARENT]=-1;
		nodes[i][RANK] = 0;
	}

	/* The main O(N^2) pass: merge connected components */
	for( i = 0; i < N; i++ ) {
		int root = i;

		/* find root */
		while( nodes[root][PARENT] >= 0 ) root = nodes[root][PARENT];

		for( j = 0; j < N; j++ ) {
			if( i == j ||
				!predicate(eps, vec_x[i], vec_y[i], vec_w[i], vec_h[i],
								vec_x[j], vec_y[j], vec_w[j], vec_h[j])) continue;
			int root2 = j;

			while( nodes[root2][PARENT] >= 0 ) root2 = nodes[root2][PARENT];

			if( root2 != root ) {
				/* unite both trees */
				int rank = nodes[root][RANK], rank2 = nodes[root2][RANK];
				if( rank > rank2 ) nodes[root2][PARENT] = root;
				else {
					nodes[root][PARENT] = root2;
					nodes[root2][RANK] += rank == rank2;
					root = root2;
				}

				int k = j, parent;

				/* compress the path from node2 to root */
				while( (parent = nodes[k][PARENT]) >= 0 ) {
					nodes[k][PARENT] = root;
					k = parent;
				}

				/* compress the path from node to root */
				k = i;
				while( (parent = nodes[k][PARENT]) >= 0 ) {
					nodes[k][PARENT] = root;
					k = parent;
				}
			}
		}
	}

	/* Final O(N) pass: enumerate classes */
	labels.resize(N);
	int nclasses = 0;

	for( i = 0; i < N; i++ ) {
		int root = i;
		while( nodes[root][PARENT] >= 0 )
		root = nodes[root][PARENT];
		/* re-use the rank as the class label */
		if( nodes[root][RANK] >= 0 ) nodes[root][RANK] = ~nclasses++;
		labels[i] = ~nodes[root][RANK];
	}

	return nclasses;
}
