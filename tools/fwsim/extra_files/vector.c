#include "vector.h"
#include <math.h>

Vec vecInit(float x, float y)
{
    Vec ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

Vec sub(Vec a, Vec b)
{
    Vec ret;
    ret.x = a.x - b.x;
    ret.y = a.y - b.y;
    return ret;
}

Vec add(Vec a, Vec b)
{
    Vec ret;
    ret.x = a.x + b.x;
    ret.y = a.y + b.y;
    return ret;
}

float dot(Vec a, Vec b)
{
    float ret = a.x * b.x + a.y * b.y;
    return ret;
}

float cross(Vec a, Vec b)
{
    float ret = a.x * b.y - a.y * b.x;
    return ret;
}
float mag(Vec a)
{
    float ret = sqrtf(a.x * a.x + a.y * a.y);
    return ret;
}

Vec norm(Vec a)
{
    Vec ret;
    float m = mag(a);
    ret.x   = a.x / m;
    ret.y   = a.y / m;
    return ret;
}

Vec mult(Vec a, float scale)
{
    Vec ret;
    ret.x = a.x * scale;
    ret.y = a.y * scale;
    return ret;
}

Vec rotateVec(Vec a, float angle)
{
    Vec ret;
    ret.x = cosf(angle) * a.x - sinf(angle) * a.y;
    ret.y = sinf(angle) * a.x + cosf(angle) * a.y;
    return ret;
}
