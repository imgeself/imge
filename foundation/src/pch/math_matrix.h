#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdint.h>
#include <string.h>

// We use column-major vector.
// We layout the matrix like column-major matrix (mathematically), we do post multiplication, etc.
// But our memory layout is row-major.
struct Matrix4 {
    Vector4 data[4];

    Matrix4();
    Matrix4(Vector4 v1, Vector4 v2, Vector4 v3, Vector4 v4);
    Matrix4(float array[4][4]);
    Matrix4(const Matrix4& m);

    Vector4& operator[](int index);
};

inline Matrix4::Matrix4() {
    data[0] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    data[1] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    data[2] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    data[3] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
}

inline Matrix4::Matrix4(Vector4 v1, Vector4 v2, Vector4 v3, Vector4 v4) {
    data[0] = v1;
    data[1] = v2;
    data[2] = v3;
    data[3] = v4;
}

inline Matrix4::Matrix4(float arr[4][4]) {
    memcpy(data, arr, 4 * 4 * sizeof(float));
}

inline Matrix4::Matrix4(const Matrix4& m) {
    data[0] = m.data[0];
    data[1] = m.data[1];
    data[2] = m.data[2];
    data[3] = m.data[3];
}

inline Vector4& Matrix4::operator[](int index) {
    return data[index];
}

inline Vector4 operator*(Matrix4& left, Vector4& right) {
    Vector4 result;
    result.x = DotProduct(left[0], right);
    result.y = DotProduct(left[1], right);
    result.z = DotProduct(left[2], right);
    result.w = DotProduct(left[3], right);

    return result;
}

inline Vector4 operator*(const Matrix4& left, const Vector4& right) {
    Vector4 result;
    result.x = DotProduct(left.data[0], right);
    result.y = DotProduct(left.data[1], right);
    result.z = DotProduct(left.data[2], right);
    result.w = DotProduct(left.data[3], right);

    return result;
}

inline Matrix4 operator*(Matrix4 left, Matrix4 right) {
    Matrix4 result;
    for (uint32_t row = 0; row < 4; ++row) {
        Vector4 rowVector = left[row];
        for (uint32_t column = 0; column < 4; ++column) {
            Vector4 columnVector = Vector4(right[0][column], right[1][column], right[2][column], right[3][column]);
            float value = DotProduct(rowVector, columnVector);

            result[row][column] = value;
        }
    }

    return result;
}

inline Matrix4 Transpose(Matrix4& mat) {
    Matrix4 result;
    for (uint32_t row = 0; row < 4; ++row) {
        for (uint32_t column = 0; column < 4; ++column) {
            result[row][column] = mat[column][row];
        }
    }
    return result;
}

inline Matrix4 PerspectiveMatrixLH(uint32_t width, uint32_t height, float nearDistance, float farDistance, float fov) {
    Matrix4 result;
    const float ar = (float) width / (float) height;
    const float zNear = nearDistance;
    const float zFar = farDistance;
    const float zRange = zNear - zFar;
    const float tanHalfFOV = tanf(fov / 2.0f);
    const float fRange = zFar / (zFar - zNear);

    result[0][0] = 1.0f / (tanHalfFOV * ar);
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = 1.0f / tanHalfFOV;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = fRange;
    result[2][3] = -fRange * zNear;

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 1.0f;
    result[3][3] = 0.0f;

    return result;
}

inline Matrix4 OrthoMatrixLH(uint32_t width, uint32_t height, float nearDistance, float farDistance) {
    Matrix4 result;
    result[0][0] = 2.0f / (float) width;
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = 2.0f / (float) height;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = 1 / (farDistance - nearDistance);
    result[2][3] = -nearDistance / (farDistance - nearDistance);

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 OrthoMatrixOffCenterLH(float left, float right, float top, float bottom, float nearDistance, float farDistance) {
    float width = right - left;
    float height = top - bottom;
    Matrix4 result;
    result[0][0] = 2.0f / (float)width;
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = (left + right) / (left - right);

    result[1][0] = 0.0f;
    result[1][1] = 2.0f / (float)height;
    result[1][2] = 0.0f;
    result[1][3] = (top + bottom) / (bottom - top);

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = 1 / (farDistance - nearDistance);
    result[2][3] = nearDistance / (nearDistance - farDistance);

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 OrthoMatrixLH(float width, float height, float nearDistance, float farDistance) {
    Matrix4 result;
    result[0][0] = 2.0f / (float) width;
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = 2.0f / (float)height;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = 1 / (farDistance - nearDistance);
    result[2][3] = nearDistance / (nearDistance - farDistance);

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 CameraLookAtLH(const Vector3& position, const Vector3& target, const Vector3& upVector) {
    Matrix4 result;

    const Vector3 cameraZ = Normalize(target - position);
    const Vector3 cameraX = Normalize(CrossProduct(upVector, cameraZ));
    const Vector3 cameraY = CrossProduct(cameraZ, cameraX);
    result[0] = Vector4(cameraX, -DotProduct(position, cameraX));
    result[1] = Vector4(cameraY, -DotProduct(position, cameraY));
    result[2] = Vector4(cameraZ, -DotProduct(position, cameraZ));
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 TransformLookAtLH(const Vector3& position, const Vector3& target, const Vector3& upVector) {
    Matrix4 result;

    const Vector3 cameraZ = Normalize(target - position);
    const Vector3 cameraX = Normalize(CrossProduct(Vector3(0.0f, 1.0f, 0.0f), cameraZ));
    const Vector3 cameraY = CrossProduct(cameraZ, cameraX);

    result[0][0] = cameraX.x;
    result[0][1] = cameraY.x;
    result[0][2] = cameraZ.x;
    result[0][3] = position.x;

    result[1][0] = cameraX.y;
    result[1][1] = cameraY.y;
    result[1][2] = cameraZ.y;
    result[1][3] = position.y;

    result[2][0] = cameraX.z;
    result[2][1] = cameraY.z;
    result[2][2] = cameraZ.z;
    result[2][3] = position.z;

    result[3][3] = 1.0f;

    return result;
}



inline Matrix4 Inverse(Matrix4& mat) {
    // Matrix inverse code copied from Doom3 source
    // Using adjugate formula to calculate inverse of matrix
    // TODO: We should check the matrix is invertible!!
    Matrix4 result;
    float det, invDet;

    // 2x2 sub-determinants required to calculate 4x4 determinant
    float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
    float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
    float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
    float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

    // 3x3 sub-determinants required to calculate 4x4 determinant
    float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
    float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
    float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
    float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

    det = (-det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3]);
    invDet = 1.0f / det;

    // remaining 2x2 sub-determinants
    float det2_03_01 = mat[0][0] * mat[3][1] - mat[0][1] * mat[3][0];
    float det2_03_02 = mat[0][0] * mat[3][2] - mat[0][2] * mat[3][0];
    float det2_03_03 = mat[0][0] * mat[3][3] - mat[0][3] * mat[3][0];
    float det2_03_12 = mat[0][1] * mat[3][2] - mat[0][2] * mat[3][1];
    float det2_03_13 = mat[0][1] * mat[3][3] - mat[0][3] * mat[3][1];
    float det2_03_23 = mat[0][2] * mat[3][3] - mat[0][3] * mat[3][2];

    float det2_13_01 = mat[1][0] * mat[3][1] - mat[1][1] * mat[3][0];
    float det2_13_02 = mat[1][0] * mat[3][2] - mat[1][2] * mat[3][0];
    float det2_13_03 = mat[1][0] * mat[3][3] - mat[1][3] * mat[3][0];
    float det2_13_12 = mat[1][1] * mat[3][2] - mat[1][2] * mat[3][1];
    float det2_13_13 = mat[1][1] * mat[3][3] - mat[1][3] * mat[3][1];
    float det2_13_23 = mat[1][2] * mat[3][3] - mat[1][3] * mat[3][2];

    // remaining 3x3 sub-determinants
    float det3_203_012 = mat[2][0] * det2_03_12 - mat[2][1] * det2_03_02 + mat[2][2] * det2_03_01;
    float det3_203_013 = mat[2][0] * det2_03_13 - mat[2][1] * det2_03_03 + mat[2][3] * det2_03_01;
    float det3_203_023 = mat[2][0] * det2_03_23 - mat[2][2] * det2_03_03 + mat[2][3] * det2_03_02;
    float det3_203_123 = mat[2][1] * det2_03_23 - mat[2][2] * det2_03_13 + mat[2][3] * det2_03_12;

    float det3_213_012 = mat[2][0] * det2_13_12 - mat[2][1] * det2_13_02 + mat[2][2] * det2_13_01;
    float det3_213_013 = mat[2][0] * det2_13_13 - mat[2][1] * det2_13_03 + mat[2][3] * det2_13_01;
    float det3_213_023 = mat[2][0] * det2_13_23 - mat[2][2] * det2_13_03 + mat[2][3] * det2_13_02;
    float det3_213_123 = mat[2][1] * det2_13_23 - mat[2][2] * det2_13_13 + mat[2][3] * det2_13_12;

    float det3_301_012 = mat[3][0] * det2_01_12 - mat[3][1] * det2_01_02 + mat[3][2] * det2_01_01;
    float det3_301_013 = mat[3][0] * det2_01_13 - mat[3][1] * det2_01_03 + mat[3][3] * det2_01_01;
    float det3_301_023 = mat[3][0] * det2_01_23 - mat[3][2] * det2_01_03 + mat[3][3] * det2_01_02;
    float det3_301_123 = mat[3][1] * det2_01_23 - mat[3][2] * det2_01_13 + mat[3][3] * det2_01_12;

    result[0][0] = -det3_213_123 * invDet;
    result[1][0] = +det3_213_023 * invDet;
    result[2][0] = -det3_213_013 * invDet;
    result[3][0] = +det3_213_012 * invDet;

    result[0][1] = +det3_203_123 * invDet;
    result[1][1] = -det3_203_023 * invDet;
    result[2][1] = +det3_203_013 * invDet;
    result[3][1] = -det3_203_012 * invDet;

    result[0][2] = +det3_301_123 * invDet;
    result[1][2] = -det3_301_023 * invDet;
    result[2][2] = +det3_301_013 * invDet;
    result[3][2] = -det3_301_012 * invDet;

    result[0][3] = -det3_201_123 * invDet;
    result[1][3] = +det3_201_023 * invDet;
    result[2][3] = -det3_201_013 * invDet;
    result[3][3] = +det3_201_012 * invDet;

    return result;
}

// Transform operations
inline Matrix4 ScaleMatrix(const Vector3& scaleFactors) {
    Matrix4 result;
    result[0][0] = scaleFactors.x;
    result[1][1] = scaleFactors.y;
    result[2][2] = scaleFactors.z;
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 TranslateMatrix(const Vector3& translateVector) {
    Matrix4 result;
    result[0][0] = 1.0f;
    result[0][3] = translateVector.x;
    result[1][1] = 1.0f;
    result[1][3] = translateVector.y;
    result[2][2] = 1.0f;
    result[2][3] = translateVector.z;
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 RotateMatrixXAxis(float radians) {
    Matrix4 result;
    result[0][0] = 1.0f;
    result[1][1] = cosf(radians);
    result[1][2] = -sinf(radians);
    result[2][1] = sinf(radians);
    result[2][2] = cosf(radians);
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 RotateMatrixYAxis(float radians) {
    Matrix4 result;
    result[0][0] = cosf(radians);
    result[0][2] = sinf(radians);
    result[1][1] = 1.0f;
    result[2][0] = -sinf(radians);
    result[2][2] = cosf(radians);
    result[3][3] = 1.0f;

    return result;
}

inline Matrix4 RotateMatrixZAxis(float radians) {
    Matrix4 result;
    result[0][0] = cosf(radians);
    result[0][1] = -sinf(radians);
    result[1][0] = sinf(radians);
    result[1][1] = cosf(radians);
    result[2][2] = 1.0f;
    result[3][3] = 1.0f;

    return result;
}

static const Matrix4 IdentityMatrix(Vector4(1.0f, 0.0f, 0.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));

#endif