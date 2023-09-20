#include "RvPhysics.h"
#include <glm/geometric.hpp>

bool RvPhysics::rayTriangleIntersection(const Ray& ray, const Tri& tri, vec3& hit)
{
	vec3 edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = tri.p1 - tri.p0;
	edge2 = tri.p2 - tri.p0;
	h = glm::cross(ray.dir, edge2);
	a = glm::dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this triangle.
	f = 1.0f / a;
	s = ray.orig - tri.p0;
	u = f * glm::dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = glm::cross(s, edge1);
	v = f * glm::dot(ray.dir, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * glm::dot(edge2, q);
	if (t > EPSILON) // ray intersection
	{
		hit = ray.orig + ray.dir * t;
		return true;
	}

	// This means that there is a line intersection but not a ray intersection.
	return false;
}
