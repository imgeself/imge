#pragma once

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
struct Vector2 {
    f32 x, y = 0.0f;

    Vector2();
    Vector2(f32 v);
    Vector2(f32 x, f32 y);

    float& operator[](const u32 index);
};

inline Vector2::Vector2() {
}

inline Vector2::Vector2(f32 v) {
    this->x = v;
    this->y = v;
}

inline Vector2::Vector2(f32 x, f32 y) {
    this->x = x;
    this->y = y;
}

inline float& Vector2::operator[](const u32 index) {
    return (&x)[index];
}

inline Vector2 operator-(Vector2 v) {
    return Vector2 {-v.x, -v.y};
}

inline Vector2 operator+=(Vector2 v1, Vector2 v2) {
    return Vector2 { v1.x + v2.x, v1.y + v2.y};
}

inline Vector2 operator*=(Vector2 v1, Vector2 v2) {
    return Vector2 { v1.x * v2.x, v1.y * v2.y};
}

inline Vector2 operator-=(Vector2 v1, Vector2 v2) {
    return Vector2 { v1.x - v2.x, v1.y - v2.y};
}

inline Vector2 operator/=(Vector2 v1, Vector2 v2) {
    return Vector2 { v1.x / v2.x, v1.y / v2.y};
}

inline Vector2 operator*(Vector2 v1, Vector2 v2) {
    return Vector2 {v1.x * v2.x, v1.y * v2.y};
}

inline Vector2 operator/(Vector2 v1, Vector2 v2) {
    return Vector2 {v1.x / v2.x, v1.y / v2.y};
}

inline Vector2 operator+(Vector2 v1, Vector2 v2) {
    return Vector2 {v1.x + v2.x, v1.y + v2.y};
}

inline Vector2 operator-(Vector2 v1, Vector2 v2) {
    return Vector2 {v1.x - v2.x, v1.y - v2.y};
}

inline Vector2 operator*(Vector2 v1, f32 factor) {
    return Vector2 {v1.x * factor, v1.y * factor};
}

inline Vector2 operator+(Vector2 v1, f32 factor) {
    return Vector2 {v1.x + factor, v1.y + factor};
}

inline Vector2 operator-(Vector2 v1, f32 factor) {
    return Vector2 {v1.x - factor, v1.y - factor};
}

inline Vector2 operator/(Vector2 v1, f32 factor) {
    return Vector2 {v1.x / factor, v1.y / factor};
}

inline bool operator==(Vector2 v1, Vector2 v2) {
    return ((v1.x == v2.x) && (v1.y == v2.y));
}

inline bool operator!=(Vector2 v1, Vector2 v2) {
    return ((v1.x != v2.x) || (v1.y != v2.y));
}

inline float DotProduct(Vector2 v1, Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

inline float Lenght(Vector2 v) {
    return sqrtf(DotProduct(v, v));
}

inline Vector2 Normalize(Vector2 v) {
    const float dot = DotProduct(v, v);
    const float factor = 1 / sqrtf(dot);
    return Vector2 {v.x * factor, v.y * factor};
}

inline Vector2 Floor(Vector2 v) {
    return Vector2 {floorf(v.x), floorf(v.y)};
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
struct Vector3 {
    f32 x, y, z = 0.0f;

    Vector3();
    Vector3(f32 v);
    Vector3(f32 x, f32 y, f32 z);
    Vector3(Vector2 v, float z);

    Vector2 xy();
    float& operator[](const u32 index);
};

inline Vector3::Vector3() {
}

inline Vector3::Vector3(f32 v) {
    this->x = v;
    this->y = v;
    this->z = v;
}

inline Vector3::Vector3(f32 x, f32 y, f32 z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

inline Vector3::Vector3(Vector2 v, f32 z) {
    this->x = v.x;
    this->y = v.y;
    this->z = z;
}

inline float& Vector3::operator[](const u32 index) {
    return (&x)[index];
}

inline Vector2 Vector3::xy() {
    return Vector2 {x, y};
}

inline Vector3 operator-(Vector3 v) {
    return Vector3 {-v.x, -v.y, -v.z};
}

inline Vector3 operator+=(Vector3 v1, Vector3 v2) {
    return Vector3 { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

inline Vector3 operator*=(Vector3 v1, Vector3 v2) {
    return Vector3 { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

inline Vector3 operator-=(Vector3 v1, Vector3 v2) {
    return Vector3 { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

inline Vector3 operator/=(Vector3 v1, Vector3 v2) {
    return Vector3 { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

inline Vector3 operator*(Vector3 v1, Vector3 v2) {
    return Vector3 {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

inline Vector3 operator/(Vector3 v1, Vector3 v2) {
    return Vector3 {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

inline Vector3 operator+(Vector3 v1, Vector3 v2) {
    return Vector3 {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

inline Vector3 operator-(Vector3 v1, Vector3 v2) {
    return Vector3 {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

inline Vector3 operator*(Vector3 v1, f32 factor) {
    return Vector3 {v1.x * factor, v1.y * factor, v1.z * factor};
}

inline Vector3 operator+(Vector3 v1, f32 factor) {
    return Vector3 {v1.x + factor, v1.y + factor, v1.z + factor};
}

inline Vector3 operator-(Vector3 v1, f32 factor) {
    return Vector3 {v1.x - factor, v1.y - factor, v1.z - factor};
}

inline Vector3 operator/(Vector3 v1, f32 factor) {
    return Vector3 {v1.x / factor, v1.y / factor, v1.z / factor};
}

inline bool operator==(Vector3 v1, Vector3 v2) {
    return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z));
}

inline bool operator!=(Vector3 v1, Vector3 v2) {
    return ((v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z));
}

inline float DotProduct(Vector3 v1, Vector3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline float Lenght(Vector3 v) {
    return sqrtf(DotProduct(v, v));
}

inline Vector3 CrossProduct(const Vector3 v1, const Vector3 v2) {
    return Vector3 {v1.y * v2.z - v1.z * v2.y,
                    v1.z * v2.x - v1.x * v2.z,
                    v1.x * v2.y - v1.y * v2.x};
}

inline Vector3 Normalize(Vector3 v) {
    const float dot = DotProduct(v, v);
    const float factor = 1 / sqrtf(dot);
    return Vector3 {v.x * factor, v.y * factor, v.z * factor};
}

inline Vector3 Floor(Vector3 v) {
    return Vector3 {floorf(v.x), floorf(v.y), floorf(v.z)};
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
struct Vector4 {
    f32 x, y, z, w = 0.0f;

    Vector4();
    Vector4(f32 v);
    Vector4(f32 x, f32 y, f32 z, f32 w);
    Vector4(Vector3 v, float w);

    Vector3 xyz();
    float& operator[](const u32 index);
};

inline Vector4::Vector4() {
}

inline Vector4::Vector4(f32 v) {
    this->x = v;
    this->y = v;
    this->z = v;
    this->w = v;
}

inline Vector4::Vector4(f32 x, f32 y, f32 z, f32 w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

inline Vector4::Vector4(Vector3 v, float w) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = w;
}

inline float& Vector4::operator[](const u32 index) {
    return (&x)[index];
}

inline Vector3 Vector4::xyz() {
    return Vector3 {x, y, z};
}

inline Vector4 operator-(Vector4 v) {
    return Vector4 {-v.x, -v.y, -v.z, -v.w};
}

inline Vector4 operator+=(Vector4 v1, Vector4 v2) {
    return Vector4 { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline Vector4 operator*=(Vector4 v1, Vector4 v2) {
    return Vector4 { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

inline Vector4 operator-=(Vector4 v1, Vector4 v2) {
    return Vector4 { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline Vector4 operator/=(Vector4 v1, Vector4 v2) {
    return Vector4 { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w};
}

inline Vector4 operator*(Vector4 v1, Vector4 v2) {
    return Vector4 {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

inline Vector4 operator/(Vector4 v1, Vector4 v2) {
    return Vector4 {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w};
}

inline Vector4 operator+(Vector4 v1, Vector4 v2) {
    return Vector4 {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline Vector4 operator-(Vector4 v1, Vector4 v2) {
    return Vector4 {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline Vector4 operator*(Vector4 v1, float factor) {
    return Vector4 {v1.x * factor, v1.y * factor, v1.z * factor, v1.w * factor};
}

inline Vector4 operator+(Vector4 v1, float factor) {
    return Vector4 {v1.x + factor, v1.y + factor, v1.z + factor, v1.w + factor};
}

inline Vector4 operator-(Vector4 v1, float factor) {
    return Vector4 {v1.x - factor, v1.y - factor, v1.z - factor, v1.w - factor};
}

inline Vector4 operator/(Vector4 v1, float factor) {
    return Vector4 {v1.x / factor, v1.y / factor, v1.z / factor, v1.w / factor};
}

inline bool operator==(Vector4 v1, Vector4 v2) {
    return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w));
}

inline bool operator!=(Vector4 v1, Vector4 v2) {
    return ((v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w));
}

inline float DotProduct(Vector4 v1, Vector4 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline float Lenght(Vector4 v) {
    return sqrtf(DotProduct(v, v));
}

inline Vector4 Normalize(Vector4 v) {
    const float dot = DotProduct(v, v);
    const float factor = 1 / sqrtf(dot);
    return Vector4 {v.x * factor, v.y * factor, v.z * factor, v.w * factor};
}

inline Vector4 Floor(Vector4 v) {
    return Vector4 {floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)};
}
