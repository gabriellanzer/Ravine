#ifndef PROGRESSIVE_MESH_H
#define PROGRESSIVE_MESH_H

#include <eastl/unordered_set.h>
using eastl::unordered_set;
#include <eastl/vector.h>
using eastl::vector;

#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/vec2.hpp>
using glm::vec2;

/*================================================================================*/
/*== Structs From Efficient PM (from Hoppe) - http://hhoppe.com/efficientpm.pdf ==*/
/*================================================================================*/

struct VertexAttrib { // Attributes at a vertex
	vec3 point; // (x y z) coordinates
};

struct WedgeAttrib { // Attributes at a wedge/corner
	vec3 normal; // (nx  ny  nz) normal vector
	vec2 uv; // (u v) texture coordinates
};

struct Vertex {
	VertexAttrib attrib;
};

struct Wedge {
	uint32_t vertex; // vertex to which wedge belongs
	WedgeAttrib attrib;
};

struct Face {
	uint32_t wedges[3]; // wedges at corners of the face
	uint32_t fnei[3]; // 3 face neighbors
	//short matid; // material identifier
};

struct Mesh {
	vector<Vertex> vertices;
	vector<Wedge> wedges;
	vector<Face> faces;
	//vector<Material> materials;
};

struct VertexAttribD { // Delta applied to vertex attributes
	vec3 dpoint; // VertexAttrib.point
};

struct WedgeAttribD { // Delta applied to wedge attributes
	vec3 dnormal; // WedgeAttrib.normal
	vec2 duv; // WedgeAttrib.uv
};

struct Vsplit {
	int flclw; // a face in neighborhood of vsplit
	short vlr_rot; // encoding of vertex vr

	struct {
		short vs_index : 2; // index (0..2) of vs within flclw
		short corners : 10; // corner continuities in Figure 9
		short ii : 2; // geometry prediction of Figure 10
		short matid_predict : 2; // are fl matid,fr matid required?
	} code; // set of 4 bit-fields (16-bit total)

	//short fl_matid; // matid of face fl if not predicted
	//short fr_matid; // matid of face fr if not predicted
	VertexAttribD vad_l, vad_s;
	vector<WedgeAttribD> wads;
};

struct PMesh {
	Mesh base_mesh; // base mesh M0
	vector<Vsplit> vsplits; // fvsplit0  vsplitn1g
	uint32_t full_nvertices; // number of vertices in Mn
	uint32_t full_nwedges; // number of wedges in Mn
	uint32_t full_nfaces; // number of faces 
};

#endif
