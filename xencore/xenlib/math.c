#include <math.h>

#include <xencore/xenlib/math.h>

double lib_rad2deg(double radians) {
    return radians * (180.0 / M_PI);
}

double lib_deg2rad(double degrees) {
    return degrees * (M_PI / 180.0);
}

vector2 lib_vector2_add(vector2 a, vector2 b) {
    vector2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

matrix2x2 lib_matrix2x2_add(matrix2x2 a, matrix2x2 b) {
    matrix2x2 result;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            result.m[i][j] = a.m[i][j] + b.m[i][j];
    return result;
}

matrix2x2 lib_matrix2x2_mul(matrix2x2 a, matrix2x2 b) {
    matrix2x2 result;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            result.m[i][j] = 0.0;
            for (int k = 0; k < 2; ++k) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}

vector2 lib_matrix2x2_mul_vector2(matrix2x2 m, vector2 v) {
    vector2 result;
    result.x = m.m[0][0] * v.x + m.m[0][1] * v.y;
    result.y = m.m[1][0] * v.x + m.m[1][1] * v.y;
    return result;
}
