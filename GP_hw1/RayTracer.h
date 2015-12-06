#ifndef _RAYTRACER_H_
#define _RAYTRACER_H_


/**
*	Do ray tracing algorithm 
*/
class RayTracer{
public:
	RayTracer(){};
	~RayTracer(){};

	// isInterset check if a ray is interset with a axis aligned bounding box
	// rayDir is direction of rthe ray , t0 and t1 are minimal and maximal range for valid intersection
	// min and max are points defining the axis aligned bounding box
	bool isInterset(float* rayOrigin, float* rayDir, float t0, float t1, float* min, float *max);

private:
	float lastTracedRay[3];
	float precompute;


};




#endif