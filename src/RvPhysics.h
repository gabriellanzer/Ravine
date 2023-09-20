#ifndef RV_PHYSICS_H
#define RV_PHYSICS_H

#include <glm/vec4.hpp>
using glm::vec4;
#include <glm/vec3.hpp>
using glm::vec3;

class RvPhysics
{
	constexpr static double EPSILON = 0.0000001;

	struct Ray
	{
		vec3 orig;
		vec3 dir;
	};

	struct Tri
	{
		vec3 p0, p1, p2;
	};

	static bool rayTriangleIntersection(const Ray& ray, const Tri& tri, vec3& hit);
};

#endif