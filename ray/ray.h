#include <stdint.h>
#include "raymath.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma pack(push, 1)
struct bitmap_header {
	uint16_t FileType;
	uint32_t FileSize;
	uint16_t Reserved1;
	uint16_t Reserved2;
	uint32_t BitmapOffset;
	uint32_t Size;
	int32_t Width;
	int32_t Height;
	uint16_t Planes;
	uint16_t BitsPerPixel;
	uint32_t Compression;
	uint32_t SizeOfBitmap;
	int32_t HorzResolution;
	int32_t VertResolution;
	uint32_t ColorsUsed;
	uint32_t ColorsImportant;
};
#pragma pack(pop)

struct Image {
	uint32_t Width;
	uint32_t Height;
	uint32_t *Pixels;
};

struct Material {
	float Scatter;
	v3 EmitColor;
	v3 RefColor;
};
struct Plane {
	v3 N;
	float d;
	uint32_t matIndex;
};

struct Sphere {
	v3 P;
	float r;
	uint32_t matIndex;
};

struct World {
	uint32_t materialCount;
	Material *materials;

	uint32_t planeCount;
	Plane *planes;

	uint32_t sphereCount;
	Sphere *spheres;
};