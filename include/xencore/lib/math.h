#ifndef _MATH_H
#define _MATH_H

double lib_rad2deg(double radians);
double lib_deg2rad(double degrees);

typedef struct {
    double x, y;
} vector2;

typedef struct {
    double m[2][2];
} matrix2x2;

vector2 lib_vector2_add(vector2 a, vector2 b);
matrix2x2 lib_matrix2x2_add(matrix2x2 a, matrix2x2 b);
matrix2x2 lib_matrix2x2_mul(matrix2x2 a, matrix2x2 b);
vector2 lib_matrix2x2_mul_vector2(matrix2x2 m, vector2 v);

#endif
