#include "WaveletLifting.h"

#include <glm/vec4.hpp>
using glm::vec4;

#include <eastl/sort.h>
using eastl::sort;

#include <eastl/unordered_set.h>
using eastl::unordered_set;

#include "crc32.hpp"

HalfFace::HalfFace(uint32_t opIndex, uint32_t compId1, uint32_t compId2) :
	oppositeIndex(opIndex), composingIndices{ compId1, compId2 }
{

}

EdgeContraction::EdgeContraction() : even(nullptr), odd(nullptr), cost(FLT_MAX)
{

}

EdgeContraction::EdgeContraction(LinkVertex* even, LinkVertex* odd, const double& cost) : even(even), odd(odd), cost(cost)
{
}

WaveletApp::WaveletApp(RvSkinnedMeshColored& mesh) : mesh(&mesh)
{
	//Initialize linkVertexMap (one entry for each vertex)
	linkVertexMap = new LinkVertex*[mesh.vertex_count];
}

WaveletApp::~WaveletApp()
{
	mesh = nullptr;
	cleanUp();
}

uint32_t hashVec3(vec3& vector)
{
	char* dataIni = reinterpret_cast<char*>(&vector.x);
	return crc(dataIni, 3/*x,y,z*/ * 4/*float=4bytes=4char*/);
}

void WaveletApp::generateLinkVertices()
{
	//Go through every vertex in the mesh and generates link linkVertices
	for (uint32_t vertIt = 0; vertIt < mesh->vertex_count; vertIt++)
	{
		RvSkinnedVertexColored& vert = mesh->vertices[vertIt];
		uint128_t vertKey = *reinterpret_cast<uint128_t*>(&vert.pos);
		auto vertLinkIt = linkPosMap.find(vertKey);

		LinkVertex* link;

		//No Vertex link yet, create a new one
		if (vertLinkIt == linkPosMap.end())
		{
			link = new LinkVertex();
			link->boundVertices.reserve(8);
			link->boundPos = vert.pos;
			linkVertices.push_back(link);
			linkPosMap.emplace(vertKey, link);
		}
		else //Get existing link
		{
			link = vertLinkIt->second;
		}

		//Append vertex data to VertexLink
		link->boundVertices.push_back(vertIt);
		linkVertexMap[vertIt] = link;
	}

	//Calculate vertex quadratic matrix and neighbors based on each face
	for (uint32_t indexIt = 0; indexIt < mesh->index_count; indexIt += 3) //For each face (3 ids)
	{
		uint32_t& aId = mesh->indices[indexIt + 0];
		uint32_t& bId = mesh->indices[indexIt + 1];
		uint32_t& cId = mesh->indices[indexIt + 2];

		//Calculate plane coefficients
		vec3 a = mesh->vertices[aId].pos;
		vec3 b = mesh->vertices[bId].pos;
		vec3 c = mesh->vertices[cId].pos;
		vec3 ba = b - a;
		vec3 ca = c - a;
		vec3 nor = normalize(cross(ba, ca));
		vec4 plane = vec4(nor, dot(nor, -a)); //ABCD planar coefficients

		//Calculate error quadric influence of this face on each vertex
		mat4 errorQuadric;
		errorQuadric[0][0] = plane[0] * plane[0];						//A�
		errorQuadric[1][0] = errorQuadric[0][1] = plane[0] * plane[1];	//AB
		errorQuadric[2][0] = errorQuadric[0][2] = plane[0] * plane[2];	//AC
		errorQuadric[3][0] = errorQuadric[0][3] = plane[0] * plane[3];	//AD
		errorQuadric[1][1] = plane[1] * plane[1];						//B�
		errorQuadric[1][2] = errorQuadric[2][1] = plane[1] * plane[2];	//BC
		errorQuadric[1][3] = errorQuadric[3][1] = plane[1] * plane[3];	//BD
		errorQuadric[2][2] = plane[2] * plane[2];						//C�
		errorQuadric[2][3] = errorQuadric[3][2] = plane[2] * plane[3];	//CD
		errorQuadric[3][3] = plane[3] * plane[3];						//D�

		//Accumulate the error quadric for each vertex
		LinkVertex* aLink = linkVertexMap[aId];
		LinkVertex* bLink = linkVertexMap[bId];
		LinkVertex* cLink = linkVertexMap[cId];
		aLink->quadric += errorQuadric;
		bLink->quadric += errorQuadric;
		cLink->quadric += errorQuadric;

		//Hold Neighbors and generate HalfEdges
		aLink->neighborVertices.insert(bLink);
		bLink->neighborVertices.insert(aLink);
		bLink->neighborVertices.insert(cLink);
		cLink->neighborVertices.insert(bLink);
		cLink->neighborVertices.insert(aLink);
		aLink->neighborVertices.insert(cLink);
	}
}

void WaveletApp::calculateEdgeContractions()
{
	//One contraction for each edge
	contractions.resize(mesh->index_count);
	for (uint32_t indexIt = 0; indexIt < mesh->index_count; indexIt += 3) //For each face (3 ids)
	{
		//Calculate triangle equation
		const uint32_t aId = mesh->indices[indexIt + 0];
		const uint32_t bId = mesh->indices[indexIt + 1];
		const uint32_t cId = mesh->indices[indexIt + 2];

		LinkVertex* aLink = linkVertexMap[aId];
		LinkVertex* bLink = linkVertexMap[bId];
		LinkVertex* cLink = linkVertexMap[cId];

		//Edges are 'ab', 'bc' and 'ca'
		const vec4 a = mesh->vertices[aId].pos;
		const vec4 b = mesh->vertices[bId].pos;
		const vec4 c = mesh->vertices[cId].pos;

		//Cost for 'ab'
		{
			const mat4 combinedError = aLink->quadric + bLink->quadric;
			const double costA = dot(a, combinedError * a);
			const double costB = dot(b, combinedError * b);
			EdgeContraction& ctr = contractions[indexIt + 0];
			if (costA < costB) //'a' is even, 'b' is odd
			{
				ctr = { aLink, bLink, costA };
			}
			else //'b' is even, 'a' is odd
			{
				ctr = { bLink, aLink, costB };
			}

			//Setup contraction halfFaces
			if (ctr.halfFaces[0].oppositeIndex == UINT32_MAX)
			{
				ctr.halfFaces[0] = HalfFace(cId, aId, bId);
			}
			else
			{
				ctr.halfFaces[1] = HalfFace(cId, aId, bId);
			}
		}

		//Cost for 'bc'
		{
			const mat4 combinedError = bLink->quadric + cLink->quadric;
			const double costB = dot(b, combinedError * b);
			const double costC = dot(c, combinedError * c);
			EdgeContraction& ctr = contractions[indexIt + 1];
			if (costB < costC) //'b' is even, 'c' is odd
			{
				ctr = { bLink, cLink, costB };
			}
			else //'c' is even, 'b' is odd
			{
				ctr = { cLink, bLink, costC };
			}

			//Setup contraction halfFaces
			if (ctr.halfFaces[0].oppositeIndex == UINT32_MAX)
			{
				ctr.halfFaces[0] = HalfFace(aId, bId, cId);
			}
			else
			{
				ctr.halfFaces[1] = HalfFace(aId, bId, cId);
			}
		}

		//Cost for 'ca'
		{
			const mat4 combinedError = cLink->quadric + aLink->quadric;
			const double costC = dot(c, combinedError * c);
			const double costA = dot(a, combinedError * a);
			EdgeContraction& ctr = contractions[indexIt + 2];
			if (costC < costA) //'c' is even, 'a' is odd
			{
				ctr = { cLink, aLink, costC };
			}
			else //'a' is even, 'c' is odd
			{
				ctr = { aLink, cLink, costA };
			}

			//Setup contraction halfFaces
			if (ctr.halfFaces[0].oppositeIndex == UINT32_MAX)
			{
				ctr.halfFaces[0] = HalfFace(bId, aId, cId);
			}
			else
			{
				ctr.halfFaces[1] = HalfFace(bId, aId, cId);
			}
		}
	}

	//Order based on smallest costs first
	sort(contractions.begin(), contractions.end(),
		[](const EdgeContraction& a, const EdgeContraction& b) -> bool
	{
		return a.cost < b.cost;
	});
}

void WaveletApp::splitPhase()
{
	//To perform split phase one must:
	// - Calculate unique odds vertices for this lifting step

	//Reserve estimation
	contractionsToPerform.reserve(contractions.size() / 3);
	for (EdgeContraction& contraction : contractions)
	{
		//Check if this is actually an odd and not an even vertex
		unordered_set<LinkVertex*>& neighbors = contraction.odd->neighborVertices;
		if (oddsList.find(contraction.odd) != oddsList.end()) //Check already added
		{
			continue;
		}

		//Check if any neighbor is odd
		bool isOdd = true;
		for (LinkVertex* neighbor : neighbors)
		{
			if (oddsList.find(neighbor) != oddsList.end())
			{
				isOdd = false;
				break;
			}
		}

		//Found an odd!
		if (isOdd)
		{
			//Add this contraction to the list
			oddsList.insert(contraction.odd);
			contractionsToPerform.push_back(&contraction);
		}
	}
}

void WaveletApp::updatePhase()
{
	//To perform update phase one must:
	// - Get neighbors average delta vector constant for odd vertex
	// - Update every even vertex by summing delta constant to it
	for (LinkVertex* odd : oddsList)
	{
		vec4 oddPos = mesh->vertices[odd->boundVertices[0]].pos;
		const float neighborsNum = odd->neighborVertices.size();
		vec4 evenSum;
		for (LinkVertex* neighbor : odd->neighborVertices)
		{
			evenSum += neighbor->boundPos;
		}
		odd->neighborsDelta = oddPos / (neighborsNum + 1) - evenSum / (neighborsNum*neighborsNum + neighborsNum);

		for (LinkVertex* neighbor : odd->neighborVertices)
		{
			for (uint32_t& boundId : neighbor->boundVertices)
			{
				mesh->vertices[boundId].pos += odd->neighborsDelta;
			}
		}
	}
}

void WaveletApp::performContractions()
{
	//To perform a contraction one must:
	// - Drag all odd vertices to the correct even pair location
	// - Delete indices the two faces that will be collapsed
	uint32_t removedIds = 0;
	for (EdgeContraction* contr : contractionsToPerform)
	{
		//Dragging step
		const vec4& evenPos = mesh->vertices[contr->even->boundVertices[0]].pos;
		for (uint32_t& oddId : contr->odd->boundVertices)
		{
			mesh->vertices[oddId].pos = evenPos;
		}

		//Should not collapse border edges
		if (contr->halfFaces[1].oppositeIndex == UINT32_MAX)
		{
			continue;
		}

		//Mark indices to be removed (of both half-edges - two faces)
		HalfFace& hEdge = contr->halfFaces[0];
		mesh->indices[hEdge.oppositeIndex] = UINT32_MAX;
		mesh->indices[hEdge.composingIndices[0]] = UINT32_MAX;
		mesh->indices[hEdge.composingIndices[1]] = UINT32_MAX;
		hEdge = contr->halfFaces[1];
		mesh->indices[hEdge.oppositeIndex] = UINT32_MAX;
		mesh->indices[hEdge.composingIndices[0]] = UINT32_MAX;
		mesh->indices[hEdge.composingIndices[1]] = UINT32_MAX;
		removedIds += 6; //Two faces = 6 Ids
	}

	//Sort all invalidated triangles to the end of the array
	size_t remIt = 0;
	const uint32_t size = mesh->index_count;
	for (uint32_t i = 0; i < size; i += 3)
	{
		if (mesh->indices[i] == UINT32_MAX)
		{
			mesh->indices[i + 0] = mesh->indices[size - remIt - 3];
			mesh->indices[i + 1] = mesh->indices[size - remIt - 2];
			mesh->indices[i + 2] = mesh->indices[size - remIt - 1];
		}
		remIt += 3;
	}
	mesh->index_count = size - removedIds;
}

void WaveletApp::cleanUp()
{
	for (LinkVertex* vert : linkVertices)
	{
		delete vert;
	}
	linkVertices.clear();
	delete[] linkVertexMap;
	linkPosMap.clear();
	contractions.clear();
	contractionsToPerform.clear();
}
