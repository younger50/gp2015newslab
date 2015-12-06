#include "RayTracer.h"

bool RayTracer::isInterset(float* rayOrigin, float* rayDir, float t0, float t1, float* min, float *max)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	if (rayDir[0] >= 0) {
		tmin = (min[0] - rayOrigin[0]) / rayDir[0];
		tmax = (max[0] - rayOrigin[0]) / rayDir[0];
	}
	else {
		tmin = (max[0] - rayOrigin[0]) / rayDir[0];
		tmax = (min[0] - rayOrigin[0]) / rayDir[0];
	}
	if (rayDir[1] >= 0) {
		tymin = (min[1] - rayOrigin[1]) / rayDir[1];
		tymax = (max[1] - rayOrigin[1]) / rayDir[1];
	}
	else {
		tymin = (max[1] - rayOrigin[1]) / rayDir[1];
		tymax = (min[1] - rayOrigin[1]) / rayDir[1];
	}
	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	if (rayDir[2] >= 0) {
		tzmin = (min[2] - rayOrigin[2]) / rayDir[2];
		tzmax = (max[2] - rayOrigin[2]) / rayDir[2];
	}
	else {
		tzmin = (max[2] - rayOrigin[2]) / rayDir[2];
		tzmax = (min[2] - rayOrigin[2]) / rayDir[2];
	}
	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	return ((tmin < t1) && (tmax > t0));
}