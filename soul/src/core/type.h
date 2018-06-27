//
// Created by Kevin Yudi Utama on 2/8/18.
//

#ifndef SOUL_TYPE_H
#define SOUL_TYPE_H

#pragma once

#include <cstdint>
#include <cstddef>
#include <climits>
#include <cfloat>

#include "extern/glad.h"
#include <GLFW/glfw3.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

typedef int8 s8;
typedef int8 s08;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint8 byte;
typedef uint8 u08;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef real32 r32;
typedef real64 r64;
typedef real32 f32;
typedef real64 f64;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef b32 b32x;
typedef u32 u32x;

struct Vec2f;
struct Vec3f;
struct Vec4f;

struct Vec2f {
    float x;
    float y;

    Vec2f() : x(0), y(0) {}
    Vec2f(float x, float y) : x(x), y(y) {}
};

struct Vec3f {
    float x;
    float y;
    float z;

    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec4f {
    float x;
    float y;
    float z;
    float w;

    Vec4f() : x(0), y(0), z(0), w(0) {}
    Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4f(Vec3f xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
    Vec3f xyz() { return Vec3f(x, y, z); }
};

struct Mat3 {

    float elem[3][3];

    Mat3() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                elem[i][j] = 0;
            }
        }
    }
};

struct Mat4 {

    union {
        float elem[4][4];
        float mem[16];
    };


    Mat4() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                elem[i][j] = 0;
            }
        }
    }
};

struct Quaternion {

    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(0) {};
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

};

struct Transform {
    Mat3 rotation;
    Vec3f position;
    Vec3f scale;
};

#endif //SOUL_TYPE_H
