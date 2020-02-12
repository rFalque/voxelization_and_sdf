#ifndef IO_STRUCTURES_H
#define IO_STRUCTURES_H

#include<vector>
#include<stdint.h>

struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct double3 { double x, y, z; };
struct uint3 { uint32_t x, y, z; };
struct uint4 { uint32_t x, y, z, w; };
struct uchar3 { uint8_t r, g, b;};

struct geometry
{
    std::vector<float3> vertices;
    std::vector<float3> normals;
    std::vector<float2> texcoords;
    std::vector<uchar3> rgb;
    std::vector<uint3> triangles;
};

#endif