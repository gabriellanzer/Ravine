#ifndef APP_WAVELET_LIFTING_H
#define APP_WAVELET_LIFTING_H

#include <eastl/unordered_map.h>
using eastl::unordered_map;
#include <eastl/vector.h>
using eastl::vector;
#include <eastl/unordered_set.h>
using eastl::unordered_set;

#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/mat4x4.hpp>
using glm::mat4;

#include "RvDataTypes.h"

struct LinkVertex
{
	/**
	 * \brief Mesh vertices that share the same location in space.
	 */
	vector<uint32_t> boundVertices;

	/**
	 * \brief The two neighbors for each bound vertex.
	 */
	vector<uint32_t[2]> boundNeighbors;
	
	/**
	 * \brief Vertices that are considered neighbors of all bound linkVertices;
	 */
	unordered_set<LinkVertex*> neighborVertices;

	/**
	 * \brief Quadric error matrix, calculated from neighbor planes.
	 */
	mat4 quadric;
};

struct EdgeContraction
{
	LinkVertex* even, *odd;
	double cost;

	EdgeContraction();

	EdgeContraction(LinkVertex* even, LinkVertex* odd, const double& cost);
};

class WaveletApp
{
private:
	const RvSkinnedMeshColored* mesh = nullptr;

	vector<LinkVertex*> linkVertices;
	/**
	 * \brief Maps every vertex ID to it's Link owner.
	 */
	LinkVertex** linkVertexMap;
	/**
	 * \brief Maps every vertex position to it's Link owner.
	 */
	map<uint128_t, LinkVertex*> linkPosMap;

	vector<EdgeContraction> contractions;
	WaveletApp() = default;
public:
	vector<EdgeContraction*> contractionsToPerform;

	explicit WaveletApp(const RvSkinnedMeshColored& mesh);
	~WaveletApp();
	void generateLinkVertices();
	void calculateEdgeContractions();
	void splitPhase();
};

#endif
