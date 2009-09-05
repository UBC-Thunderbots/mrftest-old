#include "util/hungarian.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace {
	const double EPS = 1e-9;

	bool equal(double x, double y) {
		return std::fabs(x - y) < EPS;
	}
}

hungarian::hungarian(std::size_t size) : weights(size, std::vector<double>(size, 0.0)), mx(size, UINT_MAX), my(size, UINT_MAX) {
}

void hungarian::execute() {
	// Meddlest-thou not in the affairs of Hungarian algorithms, for thou art crunchy and go well with Ketchup.
	// This was understood by Robert, communicated by Simon, written by Chris, and obfuscated by Anton.
	const unsigned int N = weights.size();

	double lx[N], ly[N];
	std::fill(lx, lx + N, 0);
	std::fill(ly, ly + N, 0);

	double sy[N];
	unsigned int py[N];
	bool S[N];

	std::fill(mx.begin(), mx.end(), UINT_MAX);
	std::fill(my.begin(), my.end(), UINT_MAX);

	for (unsigned int i = 0; i < N; i++)
		for (unsigned int j = 0; j < N; j++)
			ly[j] = std::max(ly[j], weights[i][j]);

	for (unsigned int szm = 0; szm < N; ++szm) {
		unsigned int u = 0;
		while (u < N && mx[u] != UINT_MAX)
			u++;
		std::fill(S, S + N, false);
		S[u] = true;
		std::fill(py, py + N, UINT_MAX);
		for (unsigned int y = 0; y < N; y++)
			sy[y] = ly[y] + lx[u] - weights[u][y];
		for (;;) {
			unsigned int y = 0;
			while (y < N && !(equal(sy[y], 0.0) && py[y] == UINT_MAX))
				y++;
			if (y == N) {
				double a = 1.0 / 0.0;
				for (unsigned int i = 0; i < N; i++)
					if (py[i] == UINT_MAX)
						a = std::min(a, sy[i]);
				for (unsigned int v = 0; v < N; v++) {
					if (S[v])
						lx[v] -= a;
					if (py[v] != UINT_MAX)
						ly[v] += a;
					else
						sy[v] -= a;
				}
			} else {
				for (unsigned int x = 0; x < N; x++)
					if (S[x] && equal(lx[x] + ly[y], weights[x][y])) {
						py[y] = x;
						break;
					}
				if (my[y] != UINT_MAX) {
					S[my[y]] = true;
					for (unsigned int z = 0; z < N; z++)
						sy[z] = std::min(sy[z], lx[my[y]] + ly[z] - weights[my[y]][z]);
				} else {
					while(y<N) {
						unsigned int p, ny;
						p = py[y];
						ny = mx[p];
						my[y] = p;
						mx[p] = y;
						y = ny;
					}
					break;
				}
			}
		}
	}
}

