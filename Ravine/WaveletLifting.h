#ifndef APP_WAVELET_LIFTING_H
#define APP_WAVELET_LIFTING_H

#include <eastl/unordered_map.h>
struct RvPersistentBuffer;
using eastl::unordered_map;
#include <eastl/vector.h>
using eastl::vector;
#include <eastl/unordered_set.h>
using eastl::unordered_set;
#include <eastl/vector_multimap.h>
using eastl::vector_multimap;

#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/vec4.hpp>
using glm::vec4;
#include <glm/mat4x4.hpp>
using glm::mat4;

#include "RvDataTypes.h"

struct HalfFace
{
	/**
	 * \brief Index of the vertex opposing this half-edge.
	 * This index is located on the tip of the face whose
	 * side is this half-edge.
	 */
	uint32_t oppositeIndex;

	/**
	 * \brief Mesh indices that compose this half-edge.
	 */
	uint32_t composingIndices[2];

	HalfFace();
	explicit HalfFace(const uint32_t& opIndex, const uint32_t& compId1, const uint32_t& compId2);
};

struct EdgeContraction;

struct LinkVertex
{
	/**
	 * \brief Mesh vertices that share the same location in space.
	 */
	vector<uint32_t> boundVertices;

	/**
	 * \brief Vertices that are considered neighbors of all bound linkVertices;
	 */
	unordered_set<LinkVertex*> neighborVertices;

	/**
	 * \brief Position of bound vertices.
	 */
	vec4 boundPos;

	/**
	 * \brief Average neighbors delta vector.
	 */
	vec4 neighborsDelta;

	/**
	 * \brief Quadric error matrix, calculated from neighbor planes.
	 */
	mat4 quadric;

	/**
	 * \brief Associated EdgeContraction Index for each neighbor LinkVertex.
	 */
	map<LinkVertex*, uint32_t> contractions;
};

struct EdgeContraction
{
	LinkVertex *even, *odd;
	double cost;
	HalfFace halfFaces[2];

	EdgeContraction();

	EdgeContraction(LinkVertex* even, LinkVertex* odd, const double& cost);
	~EdgeContraction() = default;
};


typedef eastl::pair<const LinkVertex*, const LinkVertex*> EdgeKey;

struct Tri
{
	uint32_t v0, v1, v2;
};

class WaveletApp
{
private:
	RvSkinnedMeshColored* mesh = nullptr;

	unordered_set<LinkVertex*> boundaryVertices;
	vector<LinkVertex*> linkVertices;
	/**
	 * \brief Maps every vertex ID to it's Link owner.
	 */
	LinkVertex** linkVertexMap = nullptr;
	/**
	 * \brief Maps every vertex position to it's Link owner.
	 */
	map<uint128_t, LinkVertex*> linkPosMap;
	vector<EdgeContraction> contractions;

	WaveletApp() = default;

public:
	static const uint32_t LIFTING_STEPS = 10;
	vector<EdgeContraction*> contractionsToPerform;
	unordered_set<LinkVertex*> oddsList;

	explicit WaveletApp(RvSkinnedMeshColored& mesh);
	~WaveletApp();
	void generateLinkVertices();
	void calculateEdgeContractions();
	void splitPhase();
	void updatePhase();
	void performContractions();
	void cleanUp();
	void generateOddsFeedback(vector<uint32_t>& oddsIndexBuffer, vector<vec4>& oddsVertexBuffer);
	void generateContractionsFeedback(vector<uint32_t>& oddsIndexBuffer, vector<vec4>& oddsVertexBuffer);
};

#endif
