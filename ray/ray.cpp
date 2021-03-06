#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "ray.h"
#include "raymath.h"
#include <float.h>

uint32_t GetTotalPixelSize(Image image)
{
	uint32_t Result = image.Width * image.Width * sizeof(uint32_t);
	return(Result);
}

Image CreateImage(int Width, int Height) {
	Image image = {};
	image.Width = Width;
	image.Height = Height;

	uint32_t OutputPixelSize = GetTotalPixelSize(image);
	image.Pixels = (uint32_t *)malloc(OutputPixelSize);

	return(image);
}

void WriteImage(Image image, const char *OutputFileName)
{
	uint32_t OutputPixelSize = GetTotalPixelSize(image);

	bitmap_header Header = {};
	Header.FileType = 0x4D42;
	Header.FileSize = sizeof(Header) + OutputPixelSize;
	Header.BitmapOffset = sizeof(Header);
	Header.Size = sizeof(Header) - 14;
	Header.Width = image.Width;
	Header.Height = image.Height;
	Header.Planes = 1;
	Header.BitsPerPixel = 32;
	Header.Compression = 0;
	Header.SizeOfBitmap = OutputPixelSize;
	Header.HorzResolution = 0;
	Header.VertResolution = 0;
	Header.ColorsUsed = 0;
	Header.ColorsImportant = 0;

	FILE *OutputFile;
	errno_t err = fopen_s(&OutputFile, OutputFileName, "wb");

	if (err == 0)
	{
		fwrite(&Header, sizeof(Header), 1, OutputFile);
		fwrite(image.Pixels, OutputPixelSize, 1, OutputFile);
		fclose(OutputFile);
	}
	else
	{
		printf("[ERROR] Unable to write to output file");
	}
}

static float RandomUnilateral(void)
{
	// TODO(casey): _MUST_ replace this with better entropy later
	float Result = (float)rand() / (float)RAND_MAX;
	return(Result);
}

static float RandomBilateral(void)
{
	float Result = -1.0f + 2.0f*RandomUnilateral();
	return(Result);
}

static float ExactLinearTosRGB(float L)
{
	if (L < 0.0f)
	{
		L = 0.0f;
	}

	if (L > 1.0f)
	{
		L = 1.0f;
	}

	float S = L * 12.92f;
	if (L > 0.0031308f)
	{
		S = 1.055f*Pow(L, 1.0f / 2.4f) - 0.055f;
	}

	return(S);
}

static v3 RayCast(World *world, v3 rayOrigin, v3 rayDirection) {
	
	float tolerance = 0.0001f;
	float minHitDistance = 0.001f;

	v3 result = {};
	v3 attenuation = V3(1, 1, 1);
	for (uint32_t rayCount = 0; rayCount < 8; ++rayCount)
	{
		//v3 nextOrigin = {};
		v3 nextNormal = {};
		uint32_t hitMatIndex = 0;
		float hitDistance = FLT_MAX;

		for (uint32_t planeIndex = 0; planeIndex < world->planeCount; ++planeIndex)
		{
			Plane plane = world->planes[planeIndex];

			float denom1 = Inner(plane.N, rayDirection);

			if ((denom1 < -tolerance) || (denom1 > tolerance))
			{
				float thisDistance = (-plane.d - Inner(plane.N, rayOrigin)) / denom1;

				if ((thisDistance > minHitDistance) && (thisDistance < hitDistance))
				{
					hitDistance = thisDistance;
					hitMatIndex = plane.matIndex;

					nextNormal = plane.N;
				}
			}
		}

		for (uint32_t SphereIndex = 0;
			SphereIndex < world->sphereCount;
			++SphereIndex)
		{
			Sphere sphere = world->spheres[SphereIndex];

			v3 sphereRelativeRayOrigin = rayOrigin - sphere.P;
			float a = Inner(rayDirection, rayDirection);
			float b = 2.0f*Inner(rayDirection, sphereRelativeRayOrigin);
			float c = Inner(sphereRelativeRayOrigin, sphereRelativeRayOrigin) - sphere.r*sphere.r;

			float denom2 = 2.0f*a;
			float rootTerm = SquareRoot(b*b - 4.0f*a*c);
			if (rootTerm > tolerance)
			{
				float tp = (-b + rootTerm) / denom2;
				float tn = (-b - rootTerm) / denom2;

				float thisDistance2 = tp;
				if ((tn > minHitDistance) && (tn < tp))
				{
					thisDistance2 = tn;
				}

				if ((thisDistance2 > minHitDistance) && (thisDistance2 < hitDistance))
				{
					hitDistance = thisDistance2;
					hitMatIndex = sphere.matIndex;

					//nextOrigin = rayOrigin + thisDistance2 * rayDirection;
					nextNormal = NOZ(thisDistance2*rayDirection + sphereRelativeRayOrigin);
				}
			}
		}
		if (hitMatIndex)
		{
			Material mat = world->materials[hitMatIndex];

			result += Hadamard(attenuation, mat.EmitColor);
			float cosAtten = Inner(-rayDirection, nextNormal);
			if (cosAtten < 0)
			{
				cosAtten = 0;
			}
			attenuation = Hadamard(attenuation, cosAtten*mat.RefColor);

			rayOrigin += hitDistance*rayDirection;

			v3 pureBounce = rayDirection - 2.0f * Inner(rayDirection, nextNormal)*nextNormal;
			v3 randomBounce = NOZ(nextNormal + V3(RandomBilateral(), RandomBilateral(), RandomBilateral()));
			rayDirection = NOZ(Lerp(randomBounce, mat.Scatter,pureBounce)); //3:13
		}
		else
		{
			Material mat = world->materials[hitMatIndex];

			result += Hadamard(attenuation, mat.EmitColor);

			break;
		}
	}

	return(result);
}

int main()
{
	printf("Raycasting...");
	Material materials[7] = {};
	materials[0].EmitColor = V3(0.3f, 0.4f, 0.5f);
	materials[1].RefColor = V3(0.5f, 0.5f, 0.5f);
	materials[2].RefColor = V3(0.4f, 0.0f, 0.4f);
	materials[3].EmitColor = V3(4.0f, 0.0f, 0.0f);
	materials[4].RefColor = V3(0.2f, 0.8f, 0.2f);
	materials[4].Scatter = 0.7f;
	materials[5].RefColor = V3(0.4f, 0.8f, 0.9f);
	materials[5].Scatter = 0.85f;
	materials[6].RefColor = V3(0.95f, 0.95f, 0.95f);
	materials[6].Scatter = 1.0f;

	Plane plane[1] = {};
	plane[0].N = V3(0, 0, 1);
	plane[0].d = 0;
	plane[0].matIndex = 1;

	Sphere spheres[5] = {};
	spheres[0].P = V3(-3.0f, -4.5f, 0.5f);
	spheres[0].r = 1.0f;
	spheres[0].matIndex = 2;
	spheres[1].P = V3(3, -2, 0);
	spheres[1].r = 1.0f;
	spheres[1].matIndex = 3;
	spheres[2].P = V3(-2, -1, 2);
	spheres[2].r = 1.0f;
	spheres[2].matIndex = 4;
	spheres[3].P = V3(1, -1, 3);
	spheres[3].r = 1.0f;
	spheres[3].matIndex = 5;
	spheres[4].P = V3(-2, 3, 0);
	spheres[4].r = 2.0f;
	spheres[4].matIndex = 6;

	World world = {};
	world.materialCount = ArrayCount(materials);
	world.materials = materials;
	world.planeCount = ArrayCount(plane);
	world.planes = plane;
	world.sphereCount = ArrayCount(spheres);
	world.spheres = spheres;

	Image image = CreateImage(1280, 720);

	v3 cameraPosition = V3(0, -10, 1);
	v3 cameraZ = NOZ(cameraPosition);
	v3 cameraX = NOZ(Cross(V3(0, 0, 1), cameraZ));
	v3 cameraY = NOZ(Cross(cameraZ, cameraX));

	float filmDist = 1.0f;
	float filmW = 1.0f;
	float filmH = 1.0f;

	if (image.Width > image.Height)
	{
		filmH = filmW * (float)image.Height / (float)image.Width;
	}
	else if (image.Height > image.Width)
	{
		filmW = filmH * (float)image.Width / (float)image.Height;
	}

	float halfFilmW = 0.5f*filmW;
	float halfFilmH = 0.5f*filmH;
	v3 filmCenter = cameraPosition - filmDist*cameraZ;

	float halfPixW = 0.5f / image.Width;
	float halfPixH = 0.5f / image.Height;

	uint32_t raysPerPixel = 64;
	uint32_t *Out = image.Pixels;

	for (uint32_t Y = 0; Y < image.Height; ++Y)
	{
		float filmY = -1.0f + 2.0f*((float)Y / (float)image.Height);
		for (uint32_t X = 0; X < image.Width; ++X)
		{
			float filmX = -1.0f + 2.0f*((float)X / (float)image.Width);

			v3 Color = {};
			float contrib = 1.0f / (float)raysPerPixel;
			for (uint32_t rayIndex = 0; rayIndex < raysPerPixel; ++rayIndex)
			{
				float offX = filmX + RandomBilateral() * halfPixW;
				float offY = filmY + RandomBilateral() * halfPixH;

				v3 filmP = filmCenter + offX * halfFilmW*cameraX + offY * halfFilmH*cameraY;

				v3 rayOrigin = cameraPosition;
				v3 rayDirection = NOZ(filmP - cameraPosition);

				Color += contrib * RayCast(&world, rayOrigin, rayDirection);
			}
			
			v4 BMPColor = 
			{
				255.0f*ExactLinearTosRGB(Color.r),
				255.0f*ExactLinearTosRGB(Color.g),
				255.0f*ExactLinearTosRGB(Color.b),
				255.0f
			};

			uint32_t BMPValue = BGRAPack4x8(BMPColor);
			
			*Out++ = BMPValue;
		}

		if ((Y % 64) == 0)
		{
			printf("\rRaycasting %d%%...", 100 * Y / image.Height);
			fflush(stdout);
		}
	}
	WriteImage(image, "test.bmp");
	printf("Done.");
    return 0;
}

