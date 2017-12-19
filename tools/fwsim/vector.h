#ifndef VECTOR_H
#define VECTOR_H

typedef struct
{
    float x;
    float y;
} Vec;

Vec vecInit(float x, float y);
Vec sub(Vec a, Vec b);
Vec add(Vec a, Vec b);
float dot(Vec a, Vec b);
float cross(Vec a, Vec b);
float mag(Vec a);
Vec norm(Vec a);
Vec mult(Vec a, float scale);
Vec rotateVec(Vec a, float angle);
#endif
