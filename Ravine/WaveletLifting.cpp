#include "WaveletLifting.h"

#include <glm/vec4.hpp>
using glm::vec4;

#include <eastl/sort.h>
using eastl::sort;

#include "crc32.hpp"

EdgeContraction::EdgeContraction() : even(nullptr), odd(nullptr), cost(FLT_MAX)
{
}

EdgeContraction::EdgeContraction(LinkVertex* even, LinkVertex* odd, const double& cost) : even(even), odd(odd), cost(cost)
{
}

WaveletApp::WaveletApp(const RvSkinnedMeshColored& mesh) : mesh(&mesh)
{
	//Initialize linkVertexMap (one entry for each vertex)
	linkVertexMap = new LinkVertex*[mesh.vertex_count];
}

WaveletApp::~WaveletApp()
{
	mesh = nullptr;
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

uint32_t hashVec3(vec3& vector)
{
	char* dataIni = reinterpret_cast<char*>(&vector.x);
	return crc(dataIni, 3/*x,y,z*/*4/*float=4bytes=4char*/);
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
		vec3& a = mesh->vertices[aId].pos;
		vec3& b = mesh->vertices[bId].pos;
		vec3& c = mesh->vertices[cId].pos;
		vec3 ba = b - a;
		vec3 ca = c - a;
		vec3 nor = normalize(cross(ba, ca));
		vec4 plane = vec4(nor, dot(nor, -a)); //ABCD planar coefficients

		//Calculate error quadric influence of this face on each vertex
		mat4 errorQuadric;
		errorQuadric[0][0] = plane[0] * plane[0];						//A²
		errorQuadric[1][0] = errorQuadric[0][1] = plane[0] * plane[1];	//AB
		errorQuadric[2][0] = errorQuadric[0][2] = plane[0] * plane[2];	//AC
		errorQuadric[3][0] = errorQuadric[0][3] = plane[0] * plane[3];	//AD
		errorQuadric[1][1] = plane[1] * plane[1];						//B²
		errorQuadric[1][2] = errorQuadric[2][1] = plane[1] * plane[2];	//BC
		errorQuadric[1][3] = errorQuadric[3][1] = plane[1] * plane[3];	//BD
		errorQuadric[2][2] = plane[2] * plane[2];						//C²
		errorQuadric[2][3] = errorQuadric[3][2] = plane[2] * plane[3];	//CD
		errorQuadric[3][3] = plane[3] * plane[3];						//D²

		//Accumulate the error quadric for each vertex
		LinkVertex* aLink = linkVertexMap[aId];
		LinkVertex* bLink = linkVertexMap[bId];
		LinkVertex* cLink = linkVertexMap[cId];
		aLink->quadric += errorQuadric;
		bLink->quadric += errorQuadric;
		cLink->quadric += errorQuadric;

		//Hold Neighbors for later
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
		const vec4 a = vec4(mesh->vertices[aId].pos, 1);
		const vec4 b = vec4(mesh->vertices[bId].pos, 1);
		const vec4 c = vec4(mesh->vertices[cId].pos, 1);

		//Cost for 'ab'
		{
			const mat4 combinedError = aLink->quadric + bLink->quadric;
			const double costA = dot(a, combinedError * a);
			const double costB = dot(b, combinedError * b);
			if (costA < costB) //'a' is even, 'b' is odd
			{
				contractions[indexIt + 0] = { aLink, bLink, costA };
			}
			else //'b' is even, 'a' is odd
			{
				contractions[indexIt + 0] = { bLink, aLink, costB };
			}
		}

		//Cost for 'bc'
		{
			const mat4 combinedError = bLink->quadric + cLink->quadric;
			const double costB = dot(b, combinedError * b);
			const double costC = dot(c, combinedError * c);
			if (costB < costC) //'b' is even, 'c' is odd
			{
				contractions[indexIt + 1] = { bLink, cLink, costB };
			}
			else //'c' is even, 'b' is odd
			{
				contractions[indexIt + 1] = { cLink, bLink, costC };
			}
		}

		//Cost for 'ca'
		{
			const mat4 combinedError = cLink->quadric + aLink->quadric;
			const double costC = dot(c, combinedError * c);
			const double costA = dot(a, combinedError * a);
			if (costC < costA) //'c' is even, 'a' is odd
			{
				contractions[indexIt + 2] = { cLink, aLink, costC };
			}
			else //'a' is even, 'c' is odd
			{
				contractions[indexIt + 2] = { aLink, cLink, costA };
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
	unordered_set<LinkVertex*> oddsList;
	//vector<LinkVertex*> edgesToCollapse;
	//Reserve estimation
	//edgesToCollapse.reserve(contractions.size() / 3);
	contractionsToPerform.reserve(contractions.size()/3);
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
			//edgesToCollapse.push_back(contraction.odd);
			//edgesToCollapse.push_back(contraction.even);
			contractionsToPerform.push_back(&contraction);
			contraction.cost = FLT_MAX;
		}
	}
	//Order based on smallest costs first (will set contractions to perform to end)
	sort(contractions.begin(), contractions.end(),
		[](const EdgeContraction& a, const EdgeContraction& b) -> bool
	{
		return a.cost < b.cost;
	});
	contractions.resize(contractions.size()-contractionsToPerform.size());
}
