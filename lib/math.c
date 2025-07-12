#include <lib/math.h>

int lib_abs(int x) {
    return x < 0 ? -x : x;
}

double lib_sin(double x) {
    // Reduce x to [-M_PI, M_PI]
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;
    // Taylor series expansion for sin(x) around 0, up to x^13
    double x2 = x * x;
    double result = x;
    double term = x;
    term *= -x2 / (2 * 3); result += term;
    term *= -x2 / (4 * 5); result += term;
    term *= -x2 / (6 * 7); result += term;
    term *= -x2 / (8 * 9); result += term;
    term *= -x2 / (10 * 11); result += term;
    term *= -x2 / (12 * 13); result += term;
    return result;
}

double lib_cos(double x) {
    // Reduce x to [-M_PI, M_PI]
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;
    // Taylor series expansion for cos(x) around 0, up to x^12
    double x2 = x * x;
    double result = 1.0;
    double term = 1.0;
    term *= -x2 / (1 * 2); result += term;
    term *= -x2 / (3 * 4); result += term;
    term *= -x2 / (5 * 6); result += term;
    term *= -x2 / (7 * 8); result += term;
    term *= -x2 / (9 * 10); result += term;
    term *= -x2 / (11 * 12); result += term;
    return result;
}

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
