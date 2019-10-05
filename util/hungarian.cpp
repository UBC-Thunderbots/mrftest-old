#include "util/hungarian.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
const double EPS = 1e-9;

bool equal(double x, double y)
{
    return std::fabs(x - y) < EPS;
}
}

Hungarian::Hungarian(std::size_t size)
    : weights(size, std::vector<double>(size, 0.0)),
      mx(size, std::numeric_limits<unsigned int>::max()),
      my(size, std::numeric_limits<unsigned int>::max())
{
}

void Hungarian::execute()
{
    // Meddlest-thou not in the affairs of Hungarian algorithms, for thou art
    // crunchy and go well with Ketchup.
    // This was understood by Robert, communicated by Simon, written by Chris,
    // and obfuscated by Anton.
    const std::size_t N = weights.size();

    double lx[N], ly[N];
    std::fill(lx, lx + N, 0);
    std::fill(ly, ly + N, 0);

    double sy[N];
    std::size_t py[N];
    bool S[N];

    std::fill(mx.begin(), mx.end(), std::numeric_limits<unsigned int>::max());
    std::fill(my.begin(), my.end(), std::numeric_limits<unsigned int>::max());

    for (unsigned int i = 0; i < N; i++)
    {
        for (unsigned int j = 0; j < N; j++)
        {
            ly[j] = std::max(ly[j], weights[i][j]);
        }
    }

    for (unsigned int szm = 0; szm < N; ++szm)
    {
        unsigned int u = 0;
        while (u < N && mx[u] != std::numeric_limits<unsigned int>::max())
        {
            u++;
        }
        std::fill(S, S + N, false);
        S[u] = true;
        std::fill(py, py + N, std::numeric_limits<std::size_t>::max());
        for (std::size_t y = 0; y < N; y++)
        {
            sy[y] = ly[y] + lx[u] - weights[u][y];
        }
        for (;;)
        {
            std::size_t y = 0;
            while (y < N &&
                   !(equal(sy[y], 0.0) &&
                     py[y] == std::numeric_limits<std::size_t>::max()))
            {
                y++;
            }
            if (y == N)
            {
                double a = 1.0 / 0.0;
                for (std::size_t i = 0; i < N; i++)
                {
                    if (py[i] == std::numeric_limits<std::size_t>::max())
                    {
                        a = std::min(a, sy[i]);
                    }
                }
                for (std::size_t v = 0; v < N; v++)
                {
                    if (S[v])
                    {
                        lx[v] -= a;
                    }
                    if (py[v] != std::numeric_limits<std::size_t>::max())
                    {
                        ly[v] += a;
                    }
                    else
                    {
                        sy[v] -= a;
                    }
                }
            }
            else
            {
                for (std::size_t x = 0; x < N; x++)
                {
                    if (S[x] && equal(lx[x] + ly[y], weights[x][y]))
                    {
                        py[y] = x;
                        break;
                    }
                }
                if (my[y] != std::numeric_limits<unsigned int>::max())
                {
                    S[my[y]] = true;
                    for (std::size_t z = 0; z < N; z++)
                    {
                        sy[z] = std::min(
                            sy[z], lx[my[y]] + ly[z] - weights[my[y]][z]);
                    }
                }
                else
                {
                    while (y < N)
                    {
                        std::size_t p, ny;
                        p     = py[y];
                        ny    = mx[p];
                        my[y] = p;
                        mx[p] = y;
                        y     = ny;
                    }
                    break;
                }
            }
        }
    }
}
