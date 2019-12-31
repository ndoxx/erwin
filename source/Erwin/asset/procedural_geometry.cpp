#include "asset/procedural_geometry.h"
#include "core/core.h"
#include "core/intern_string.h"
#include "glm/glm.hpp"

#include <numeric>
#include <algorithm>

namespace erwin
{
namespace pg
{

static constexpr uint32_t k_max_vertices = 1024;
static constexpr uint32_t k_max_indices  = 8*1024;

// Thread-local storage struct to hold temporary data for procedural mesh construction
thread_local struct PGStorage
{
	glm::vec3 position[k_max_vertices];
	glm::vec3 normal[k_max_vertices];
	glm::vec3 tangent[k_max_vertices];
	glm::vec2 uv[k_max_vertices];
	uint32_t triangles[k_max_indices];
	uint32_t lines[k_max_indices];
	uint32_t vertex_count;
	uint32_t triangle_count;
	uint32_t line_count;
} tl_storage;

inline void reset_storage()
{
	tl_storage.vertex_count = 0;
	tl_storage.triangle_count = 0;
	tl_storage.line_count = 0;
}

inline void add_vertex(const glm::vec3& position, const glm::vec2& uv)
{
	// W_ASSERT(tl_storage.vertex_count+1<k_max_vertices, "Vertex data is full.");
	tl_storage.position[tl_storage.vertex_count] = position;
	tl_storage.uv[tl_storage.vertex_count] = uv;
	++tl_storage.vertex_count;
}

inline void add_vertex(glm::vec3&& position, glm::vec2&& uv)
{
	// W_ASSERT(tl_storage.vertex_count+1<k_max_vertices, "Vertex data is full.");
	tl_storage.position[tl_storage.vertex_count] = std::move(position);
	tl_storage.uv[tl_storage.vertex_count] = std::move(uv);
	++tl_storage.vertex_count;
}

inline void add_triangle(uint32_t a, uint32_t b, uint32_t c)
{
	tl_storage.triangles[3*tl_storage.triangle_count+0] = a;
	tl_storage.triangles[3*tl_storage.triangle_count+1] = b;
	tl_storage.triangles[3*tl_storage.triangle_count+2] = c;
	++tl_storage.triangle_count;
}

inline void add_line(uint32_t a, uint32_t b)
{
	tl_storage.lines[2*tl_storage.line_count+0] = a;
	tl_storage.lines[2*tl_storage.line_count+1] = b;
	++tl_storage.line_count;
}

static void build_normals()
{
    // For each triangle in indices list
    for(uint32_t ii=0; ii<tl_storage.triangle_count; ++ii)
    {
    	uint32_t A = tl_storage.triangles[3*ii+0];
    	uint32_t B = tl_storage.triangles[3*ii+1];
    	uint32_t C = tl_storage.triangles[3*ii+2];

    	// Get vertex position
        const glm::vec3& p1 = tl_storage.position[A];
        const glm::vec3& p2 = tl_storage.position[B];
        const glm::vec3& p3 = tl_storage.position[C];

        // Compute local normal using cross product
        glm::vec3 U(p2-p1);
        glm::vec3 V(p3-p1);
        glm::vec3 normal = glm::normalize(glm::cross(U,V));

        // Assign normal to each vertex
        tl_storage.normal[A] = normal;
        tl_storage.normal[B] = normal;
        tl_storage.normal[C] = normal;
    }
}

static void build_tangents()
{
    // For each triangle in indices list
    for(uint32_t ii=0; ii<tl_storage.triangle_count; ++ii)
    {
    	uint32_t A = tl_storage.triangles[3*ii+0];
    	uint32_t B = tl_storage.triangles[3*ii+1];
    	uint32_t C = tl_storage.triangles[3*ii+2];

        // Get positions, UVs and their deltas
        const glm::vec3& p1 = tl_storage.position[A];
        const glm::vec3& p2 = tl_storage.position[B];
        const glm::vec3& p3 = tl_storage.position[C];
        glm::vec3 e1(p2-p1);
        glm::vec3 e2(p3-p1);

        const glm::vec2& uv1 = tl_storage.uv[A];
        const glm::vec2& uv2 = tl_storage.uv[B];
        const glm::vec2& uv3 = tl_storage.uv[C];
        glm::vec2 delta_uv1(uv2-uv1);
        glm::vec2 delta_uv2(uv3-uv1);

        // Compute tangents
        float det_inv = 1.0f/(delta_uv1.x*delta_uv2.y - delta_uv2.x*delta_uv1.y);
        glm::vec3 tangent = det_inv * (e1*delta_uv2.y - e2*delta_uv1.y);

        // Assign tangent to each vertex
        tl_storage.tangent[A] = tangent;
        tl_storage.tangent[B] = tangent;
        tl_storage.tangent[C] = tangent;
    }
}

static void build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params)
{
	// Indirection tables for data interleaving
	constexpr uint32_t k_max_attrib = 4;
	float* containers[k_max_attrib];
	uint32_t components_count[k_max_attrib];
	std::fill(containers, containers+k_max_attrib, nullptr);
	std::fill(components_count, components_count+k_max_attrib, 0);

	// Check what we need to compute
	bool has_position = false;
	bool has_normal   = false;
	bool has_tangent  = false;
	bool has_uv       = false;
	uint32_t elements_count = 0;
	for(const BufferLayoutElement& elt: layout)
	{
		switch(elt.name)
		{
		  	case "a_position"_h:
		  	{
		  		has_position = true;
		  		containers[elements_count] = reinterpret_cast<float*>(tl_storage.position);
		  		components_count[elements_count] = 3;
		  		break;
		  	}
		  	case "a_normal"_h:
		  	{
		  		has_normal = true;
		  		containers[elements_count] = reinterpret_cast<float*>(tl_storage.normal);
		  		components_count[elements_count] = 3;
		  		break;
		  	}
		  	case "a_tangent"_h:
		  	{
		  		has_tangent = true;
		  		containers[elements_count] = reinterpret_cast<float*>(tl_storage.tangent);
		  		components_count[elements_count] = 3;
		  		break;
		  	}
		  	case "a_uv"_h:
		  	{
		  		has_uv = true;
		  		containers[elements_count] = reinterpret_cast<float*>(tl_storage.uv);
		  		components_count[elements_count] = 2;
		  		break;
		  	}
		  	default: W_ASSERT_FMT(false, "Unknown attribute: %s", istr::resolve(elt.name).c_str());
		}
		++elements_count;
	}

	// Get the total number of components per vertex
	uint32_t vertex_size = std::accumulate(components_count, components_count+k_max_attrib, 0);

	W_ASSERT(has_position, "Meshes must have a position attribute.");

	// Build attributes that need to be built
	if(has_normal)  build_normals();
	if(has_tangent) build_tangents();

	// Export
	vdata.resize(tl_storage.vertex_count*vertex_size);
	idata.resize(tl_storage.triangle_count*3);
	// Interleave vertex data
	// For each vertex
	for(int ii=0; ii<tl_storage.vertex_count; ++ii)
	{
		uint32_t offset = 0;
		// For each layout element
		for(int jj=0; jj<elements_count; ++jj)
		{
			uint32_t ccount = components_count[jj];
			float* data = containers[jj] + ii*ccount;
			// For each component of this layout element
			for(int kk=0; kk<ccount; ++kk)
			{
				vdata[ii*vertex_size+offset+kk] = data[kk];
			}
			offset += ccount;
		}
	}
	// Copy index data (lines not supported for now)
	std::copy(tl_storage.triangles, tl_storage.triangles + tl_storage.triangle_count*3, idata.data());

	reset_storage();
}

void make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params)
{
	// Ignore parameters for now
	W_ASSERT(params==nullptr, "Parameters unsupported for now.");

	// Setup position and UV vertex data, all th rest can be computed from this
    add_vertex({ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f});
    add_vertex({ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f});
    add_vertex({ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f});
    add_vertex({ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});
    add_vertex({-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f});
    add_vertex({ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f});
    add_vertex({ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});

    // Setup index data
	add_triangle(0,  1,  2); 
	add_triangle(0,  2,  3); 
	add_triangle(4,  5,  6); 
	add_triangle(4,  6,  7); 
	add_triangle(8,  9,  10);
	add_triangle(8,  10, 11);
	add_triangle(12, 13, 14);
	add_triangle(12, 14, 15);
	add_triangle(16, 17, 18);
	add_triangle(16, 18, 19);
	add_triangle(20, 21, 22);
	add_triangle(20, 22, 23);

	// Build interleaved vertex data according to input specifications
	build_shape(layout, vdata, idata, params);
}

void make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params)
{
	// Ignore parameters for now, only z-plane available
	W_ASSERT(params==nullptr, "Parameters unsupported for now.");

	add_vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f});
	add_vertex({ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f});
	add_vertex({ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f});
	add_vertex({-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f});

	add_triangle(0, 1, 2);
	add_triangle(2, 3, 0);

	build_shape(layout, vdata, idata, params);
}

} // namespace pg
} // namespace erwin