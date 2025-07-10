#ifndef _MATH_H
#define _MATH_H

#define M_PI 3.141592653589793

int abs(int x);
double sin(double x);
double cos(double x);
double rad2deg(double radians);
double deg2rad(double degrees);

typedef struct {
    double x, y;
} vector2;

typedef struct {
    double m[2][2];
} matrix2x2;

vector2 vector2_add(vector2 a, vector2 b);
matrix2x2 matrix2x2_add(matrix2x2 a, matrix2x2 b);
matrix2x2 matrix2x2_mul(matrix2x2 a, matrix2x2 b);
vector2 matrix2x2_mul_vector2(matrix2x2 m, vector2 v);

#endif
