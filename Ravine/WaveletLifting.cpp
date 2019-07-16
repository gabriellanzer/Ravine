#include "WaveletLifting.h"

#include <glm/vec4.hpp>
using glm::vec4;

#include <eastl/sort.h>
using eastl::sort;

#include <eastl/unordered_set.h>
using eastl::unordered_set;

HalfFace::HalfFace() : oppositeIndex(UINT32_MAX), composingIndices{ UINT32_MAX, UINT32_MAX }
{
}

HalfFace::HalfFace(const uint32_t& opIndex, const uint32_t& compId1, const uint32_t& compId2) :
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

}

WaveletApp::~WaveletApp()
{
	mesh = nullptr;
	cleanUp();
}

void WaveletApp::generateLinkVertices()
{
	//Initialize linkVertexMap (one entry for each vertex)
	linkVertexMap = new LinkVertex*[mesh->vertex_count];

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
		const uint32_t aId = mesh->indices[indexIt + 0];
		const uint32_t bId = mesh->indices[indexIt + 1];
		const uint32_t cId = mesh->indices[indexIt + 2];

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
			uint32_t ctrId = UINT32_MAX;
			auto it = aLink->contractions.find(bLink);
			if (it == aLink->contractions.end())
			{
				ctrId = contractions.size();
				const mat4 combinedError = aLink->quadric + bLink->quadric;
				const double costA = dot(a, combinedError * a);
				const double costB = dot(b, combinedError * b);
				if (costA < costB) //'a' is even, 'b' is odd
				{
					contractions.push_back({ aLink, bLink, costA });
				}
				else //'b' is even, 'a' is odd
				{
					contractions.push_back({ bLink, aLink, costB });
				}

				//Setup chained link
				aLink->contractions.emplace(bLink, ctrId);
				bLink->contractions.emplace(aLink, ctrId);

				//Setup contraction halfFaces
				contractions[ctrId].halfFaces[0] = HalfFace(cId, aId, bId);
			}
			else
			{
				ctrId = it->second;
				contractions[ctrId].halfFaces[1] = HalfFace(cId, aId, bId);
			}
		}

		//Cost for 'bc'
		{
			uint32_t ctrId = UINT32_MAX;
			auto it = bLink->contractions.find(cLink);
			if (it == bLink->contractions.end())
			{
				ctrId = contractions.size();
				const mat4 combinedError = bLink->quadric + cLink->quadric;
				const double costB = dot(b, combinedError * b);
				const double costC = dot(c, combinedError * c);
				if (costB < costC) //'b' is even, 'c' is odd
				{
					contractions.push_back({ bLink, cLink, costB });
				}
				else //'c' is even, 'b' is odd
				{
					contractions.push_back({ cLink, bLink, costC });
				}

				//Setup chained link
				bLink->contractions.emplace(cLink, ctrId);
				cLink->contractions.emplace(bLink, ctrId);

				//Setup contraction halfFaces
				contractions[ctrId].halfFaces[0] = HalfFace(aId, bId, cId);
			}
			else
			{
				ctrId = it->second;
				contractions[ctrId].halfFaces[1] = HalfFace(aId, bId, cId);
			}
		}

		//Cost for 'ca'
		{
			uint32_t ctrId = UINT32_MAX;
			auto it = cLink->contractions.find(aLink);
			if (it == cLink->contractions.end())
			{
				ctrId = contractions.size();
				const mat4 combinedError = cLink->quadric + aLink->quadric;
				const double costC = dot(c, combinedError * c);
				const double costA = dot(a, combinedError * a);
				if (costC < costA) //'c' is even, 'a' is odd
				{
					contractions.push_back({ cLink, aLink, costC });
				}
				else //'a' is even, 'c' is odd
				{
					contractions.push_back({ aLink, cLink, costA });
				}

				//Setup chained link
				cLink->contractions.emplace(aLink, ctrId);
				aLink->contractions.emplace(cLink, ctrId);

				//Setup contraction halfFaces
				contractions[ctrId].halfFaces[0] = HalfFace(bId, aId, cId);
			}
			else
			{
				ctrId = it->second;
				contractions[ctrId].halfFaces[1] = HalfFace(bId, aId, cId);
			}
		}
	}

	//Set boundary conditions
	for (EdgeContraction& edge : contractions)
	{
		if (edge.halfFaces[1].oppositeIndex != UINT32_MAX)
		{
			continue;
		}

		//This edge is a margin, mark it's vertices as excluded ones
		boundaryVertices.insert(edge.odd);
		boundaryVertices.insert(edge.even);
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

		//Skip boundary vertices
		if (boundaryVertices.find(contraction.odd) != boundaryVertices.end())
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
		odd->neighborsDelta.w = 0;

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
	for (EdgeContraction* ctr : contractionsToPerform)
	{
		//Should not collapse border edges
		if (ctr->halfFaces[1].oppositeIndex == UINT32_MAX)
		{
			continue;
		}

		//Dragging step
		const vec4& evenPos = mesh->vertices[ctr->even->boundVertices[0]].pos;
		for (uint32_t& oddId : ctr->odd->boundVertices)
		{
			mesh->vertices[oddId].pos = evenPos;
		}

		//Mark indices to be removed (of both half-edges - two faces)
		HalfFace& hEdge = ctr->halfFaces[0];
		mesh->indices[hEdge.oppositeIndex]		 = 0;
		mesh->indices[hEdge.composingIndices[0]] = 0;
		mesh->indices[hEdge.composingIndices[1]] = 0;
		hEdge = ctr->halfFaces[1];
		mesh->indices[hEdge.oppositeIndex]		 = 0;
		mesh->indices[hEdge.composingIndices[0]] = 0;
		mesh->indices[hEdge.composingIndices[1]] = 0;
		removedIds += 6; //Two faces = 6 Ids
	}

	////Sort all invalidated triangles to the end of the array
	//uint32_t size = mesh->index_count;
	//for (uint32_t i = 0; i < mesh->index_count; i += 3)
	//{
	//	if (mesh->indices[i] == UINT32_MAX)
	//	{
	//		uint32_t swapIt = size - 3;
	//		if (i >= swapIt)
	//		{
	//			size -= 3;
	//			break;
	//		}

	//		//Find a face whose indices are valid
	//		while (mesh->indices[swapIt] == UINT32_MAX)
	//		{
	//			swapIt -= 3;
	//		}
	//		mesh->indices[i + 0] = mesh->indices[swapIt + 0];
	//		mesh->indices[i + 1] = mesh->indices[swapIt + 1];
	//		mesh->indices[i + 2] = mesh->indices[swapIt + 2];
	//		size -= 3;
	//	}
	//}
	//mesh->index_count = size;
}

void WaveletApp::cleanUp()
{
	for (LinkVertex* vert : linkVertices)
	{
		delete vert;
	}
	boundaryVertices.clear();
	linkVertices.clear();
	delete[] linkVertexMap;
	linkVertexMap = nullptr;
	linkPosMap.clear();
	contractions.clear();
	contractionsToPerform.clear();
}

void WaveletApp::generateOddsFeedback(vector<uint32_t>& oddsIndexBuffer, vector<vec4>& oddsVertexBuffer)
{
	oddsIndexBuffer.reserve(oddsList.size() * 8);
	oddsVertexBuffer.reserve(oddsList.size() * 5);

	uint32_t oddIt = 0;
	for (LinkVertex* odd : oddsList)
	{
		//Get position where the odd vertex will be added to
		oddIt = oddsVertexBuffer.size();

		//One vertex for the Odd, many for the even neighbors
		oddsVertexBuffer.push_back(odd->boundPos);
		for (LinkVertex* even : odd->neighborVertices)
		{
			oddsVertexBuffer.push_back(even->boundPos);
		}

		//One line (2 indices) for each neighbor
		for (uint32_t i = 0, size = odd->neighborVertices.size(); i < size; i++)
		{
			oddsIndexBuffer.push_back(oddIt); //Odd vertex
			oddsIndexBuffer.push_back(oddIt + 1 + i); //Neighbor even vertex
		}
	}

}

void WaveletApp::generateContractionsFeedback(vector<uint32_t>& oddsIndexBuffer, vector<vec4>& oddsVertexBuffer)
{
	oddsIndexBuffer.reserve(contractionsToPerform.size() * 2);
	oddsVertexBuffer.reserve(contractionsToPerform.size() * 2);
	uint32_t it = 0;
	for (EdgeContraction* ctr : contractionsToPerform)
	{
		oddsVertexBuffer.push_back(mesh->vertices[ctr->odd->boundVertices[0]].pos);
		oddsVertexBuffer.push_back(mesh->vertices[ctr->even->boundVertices[0]].pos);
		oddsIndexBuffer.push_back(it++);
		oddsIndexBuffer.push_back(it++);
	}
}
