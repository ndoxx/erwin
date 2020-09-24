#include "asset/procedural_geometry.h"
#include "asset/mesh_fabricator.h"
#include "core/core.h"
#include "glm/glm.hpp"
#include "utils/constexpr_math.h"


#include "glm/gtx/string_cast.hpp"
#include "debug/logger.h"


namespace erwin
{
namespace pg
{

static MeshFabricator s_m;
static TriangleMeshFabricator s_tm;

Extent make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,[[maybe_unused]] 
                 Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Setup position and UV vertex data, all th rest can be computed from this
    s_m.add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({0.5f, 0.5f, 0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({0.5f, -0.5f, 0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({0.5f, 0.5f, -0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});

    // Setup index data
    s_m.add_triangle(0, 1, 2);
    s_m.add_triangle(0, 2, 3);
    s_m.add_triangle(4, 5, 8);
    s_m.add_triangle(4, 8, 9);
    s_m.add_triangle(6, 7, 10);
    s_m.add_triangle(6, 10, 11);
    s_m.add_triangle(12, 13, 14);
    s_m.add_triangle(12, 14, 15);
    s_m.add_triangle(16, 17, 18);
    s_m.add_triangle(16, 18, 19);
    s_m.add_triangle(20, 21, 22);
    s_m.add_triangle(20, 22, 23);

    // Build interleaved vertex data according to input specifications
    return s_m.build_shape(layout, vdata, idata);
}

Extent make_cube_lines(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                       [[maybe_unused]] Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Setup position and UV vertex data, all th rest can be computed from this
    s_m.add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f});
    s_m.add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f});
    s_m.add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 1.0f});
    s_m.add_vertex({-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    s_m.add_vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});

    // Setup line indices
    s_m.add_line(0, 3);
    s_m.add_line(0, 1);
    s_m.add_line(0, 4);
    s_m.add_line(6, 5);
    s_m.add_line(6, 7);
    s_m.add_line(6, 2);
    s_m.add_line(3, 7);
    s_m.add_line(3, 2);
    s_m.add_line(4, 7);
    s_m.add_line(4, 5);
    s_m.add_line(1, 2);
    s_m.add_line(1, 5);

    // Build interleaved vertex data according to input specifications
    return s_m.build_shape(layout, vdata, idata, true);
}

Extent make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                  [[maybe_unused]] Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    s_m.add_vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f});
    s_m.add_vertex({1.0f, -1.0f, 0.0f}, {1.0f, 0.0f});
    s_m.add_vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f});
    s_m.add_vertex({-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f});

    s_m.add_triangle(0, 1, 2);
    s_m.add_triangle(2, 3, 0);

    return s_m.build_shape(layout, vdata, idata);
}

Extent make_icosahedron(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                        [[maybe_unused]] Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Constants to get normalized vertex positions
    static constexpr float PHI   = (1.0f + utils::fsqrt(5.0f)) / 2.0f;
    static constexpr float ONE_N = 1.0f / (utils::fsqrt(2.0f + PHI)); // norm of any icosahedron vertex position
    static constexpr float PHI_N = PHI * ONE_N;

    s_m.add_vertex({-ONE_N, PHI_N, 0.f});
    s_m.add_vertex({ONE_N, PHI_N, 0.f});
    s_m.add_vertex({-ONE_N, -PHI_N, 0.f});
    s_m.add_vertex({ONE_N, -PHI_N, 0.f});
    s_m.add_vertex({0.f, -ONE_N, PHI_N});
    s_m.add_vertex({0.f, ONE_N, PHI_N});
    s_m.add_vertex({0.f, -ONE_N, -PHI_N});
    s_m.add_vertex({0.f, ONE_N, -PHI_N});
    s_m.add_vertex({PHI_N, 0.f, -ONE_N});
    s_m.add_vertex({PHI_N, 0.f, ONE_N});
    s_m.add_vertex({-PHI_N, 0.f, -ONE_N});
    s_m.add_vertex({-PHI_N, 0.f, ONE_N});

    // Compute UVs
    for(size_t ii = 0; ii < s_m.vertex_count; ++ii)
    {
        // Compute positions in spherical coordinates
        const glm::vec3& pos = s_m.positions[ii];
        float phi   = std::atan2(pos.z, pos.x);
        float theta = std::asin(pos.y); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        s_m.uvs[ii] = {0.5f + phi / (2 * float(M_PI)), 0.5f -theta / float(M_PI)};
    }

    s_m.add_triangle(0, 11, 5);
    s_m.add_triangle(0, 5, 1);
    s_m.add_triangle(0, 1, 7);
    s_m.add_triangle(0, 7, 10);
    s_m.add_triangle(0, 10, 11);
    s_m.add_triangle(1, 5, 9);
    s_m.add_triangle(5, 11, 4);
    s_m.add_triangle(11, 10, 2);
    s_m.add_triangle(10, 7, 6);
    s_m.add_triangle(7, 1, 8);
    s_m.add_triangle(3, 9, 4);
    s_m.add_triangle(3, 4, 2);
    s_m.add_triangle(3, 2, 6);
    s_m.add_triangle(3, 6, 8);
    s_m.add_triangle(3, 8, 9);
    s_m.add_triangle(4, 9, 5);
    s_m.add_triangle(2, 4, 11);
    s_m.add_triangle(6, 2, 10);
    s_m.add_triangle(8, 6, 7);
    s_m.add_triangle(9, 8, 1);

    return s_m.build_shape(layout, vdata, idata);
}


// ---- Functions to fix the UV zipper artifact on icospheres ----
// Thanks to Michael Thygesen: https://mft-dev.dk/uv-mapping-sphere/

// Detect faces with wrapped UVs, return triangle indices in input vector
std::vector<size_t> detect_wrapped_UVs()
{
    // For each triangle in the mesh, compute UV normal, if it is
    // pointing inward, triangle is flipped, meaning UV is wrapped.
    std::vector<size_t> wrapped;
    for(size_t ii=0; ii<s_tm.triangle_count; ++ii)
    {
        size_t tt = 3*ii;
        size_t a  = s_tm.indices[tt+0];
        size_t b  = s_tm.indices[tt+1];
        size_t c  = s_tm.indices[tt+2];
        glm::vec3 uv_a = {s_tm.uvs[a].x, s_tm.uvs[a].y, 0.f};
        glm::vec3 uv_b = {s_tm.uvs[b].x, s_tm.uvs[b].y, 0.f};
        glm::vec3 uv_c = {s_tm.uvs[c].x, s_tm.uvs[c].y, 0.f};
        glm::vec3 norm = glm::cross(uv_b-uv_a, uv_c-uv_a);
        if(norm.z < 0.f)
            wrapped.push_back(tt);
    }
    return wrapped;
}

// Fix the zipper artifact with vertex duplication and UV unwrap
void fix_warped_faces()
{
    std::vector<size_t> wrapped = detect_wrapped_UVs();

    std::map<size_t, size_t> visited;
    for(size_t ii: wrapped)
    {
        std::array<size_t,3> abc =
        {
            s_tm.indices[ii+0], s_tm.indices[ii+1], s_tm.indices[ii+2]
        };
        // For each index in triangle ii detect if corresponding vertex is problematic
        for(size_t jj=0; jj<3; ++jj)
        {
            size_t ind = s_tm.indices[ii+jj];
            const glm::vec2& uv = s_tm.uvs[ind];
            // Problematic vertex has its U coordinate close to 0
            if(uv.x < 0.25f)
            {
                // If not already visited, duplicate vertex but unwrap U by adding 1
                size_t temp;
                auto it = visited.find(ind);
                if(it==visited.end())
                {
                    glm::vec2 new_uv = uv;
                    new_uv.x += 1.f;
                    size_t v_index = s_tm.add_vertex(s_tm.positions[ind], new_uv);
                    visited[ind] = v_index;
                    temp = v_index;
                }
                else
                    temp = it->second;
                abc[jj] = temp;
            }
        }
        // Reassign triangle
        s_tm.set_triangle_by_index(ii, abc);
    }
}

// Fix stretched UVs at the poles due to vertex sharing by duplicating the pole
// vertices for each face, blend UVs at common edges
void fix_shared_pole_vertices()
{
    // Find indices of north and south poles
    glm::vec3 north = {0.f,  1.f, 0.f};
    glm::vec3 south = {0.f, -1.f, 0.f};
    std::array<glm::vec3,2> poles = { north, south };
    std::array<size_t,2> poles_idx =
    {
        s_tm.vertex_hash_map.find(north)->second,
        s_tm.vertex_hash_map.find(south)->second
    };

    for(size_t pp=0; pp<2; ++pp)
    {
        // Visit all triangles that contain this pole
        // In my sphere mesh, pole triangle index is always 1 mod 3 (vertex B in triangle ABC)
        // as long as refinement is at least 2.
        std::vector<std::pair<size_t, TriangleMeshFabricator::Triangle>> reassigned;
        s_tm.traverse_triangle_class(poles_idx[pp], [&](TriangleMeshFabricator::TriangleRange range)
        {
            bool first_vertex = true;
            for(auto it = range.first; it != range.second; ++it)
            {
                size_t tri_idx = it->second;
                size_t a = s_tm.indices[tri_idx+0];
                size_t b = s_tm.indices[tri_idx+1]; // Pole
                size_t c = s_tm.indices[tri_idx+2];

                glm::vec2 new_uv = s_tm.uvs[b];
                new_uv.x = 0.5f * (s_tm.uvs[a].x + s_tm.uvs[c].x);

                // Do not duplicate first vertex, simply reassign (avoids a dirty NaN normal later on
                // due to supernumerary pole vertex)
                if(first_vertex)
                {
                    s_tm.uvs[b] = new_uv;
                    first_vertex = false;
                }
                // Duplicate pole
                else
                {
                    size_t v_index = s_tm.add_vertex(poles[pp], new_uv);
                    reassigned.push_back({tri_idx, {a, v_index, c}});
                }
            }
        });
        // Reassign triangles
        for(auto&& [tri_idx, triangle]: reassigned)
            s_tm.set_triangle_by_index(tri_idx, triangle);
    }
}

Extent make_icosphere(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                      [[maybe_unused]] Parameters* params)
{
    // Constants to get normalized vertex positions
    static constexpr float PHI   = (1.0f + utils::fsqrt(5.0f)) / 2.0f;
    static constexpr float ONE_N = 1.0f / (utils::fsqrt(2.0f + PHI)); // norm of any icosahedron vertex position
    static constexpr float PHI_N = PHI * ONE_N;

    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Start with an icosahedron
    s_tm.add_vertex({-ONE_N, PHI_N, 0.f});
    s_tm.add_vertex({ONE_N, PHI_N, 0.f});
    s_tm.add_vertex({-ONE_N, -PHI_N, 0.f});
    s_tm.add_vertex({ONE_N, -PHI_N, 0.f});
    s_tm.add_vertex({0.f, -ONE_N, PHI_N});
    s_tm.add_vertex({0.f, ONE_N, PHI_N});
    s_tm.add_vertex({0.f, -ONE_N, -PHI_N});
    s_tm.add_vertex({0.f, ONE_N, -PHI_N});
    s_tm.add_vertex({PHI_N, 0.f, -ONE_N});
    s_tm.add_vertex({PHI_N, 0.f, ONE_N});
    s_tm.add_vertex({-PHI_N, 0.f, -ONE_N});
    s_tm.add_vertex({-PHI_N, 0.f, ONE_N});

    s_tm.add_triangle(0, 11, 5);
    s_tm.add_triangle(0, 5, 1);
    s_tm.add_triangle(0, 1, 7);
    s_tm.add_triangle(0, 7, 10);
    s_tm.add_triangle(0, 10, 11);
    s_tm.add_triangle(1, 5, 9);
    s_tm.add_triangle(5, 11, 4);
    s_tm.add_triangle(11, 10, 2);
    s_tm.add_triangle(10, 7, 6);
    s_tm.add_triangle(7, 1, 8);
    s_tm.add_triangle(3, 9, 4);
    s_tm.add_triangle(3, 4, 2);
    s_tm.add_triangle(3, 2, 6);
    s_tm.add_triangle(3, 6, 8);
    s_tm.add_triangle(3, 8, 9);
    s_tm.add_triangle(4, 9, 5);
    s_tm.add_triangle(2, 4, 11);
    s_tm.add_triangle(6, 2, 10);
    s_tm.add_triangle(8, 6, 7);
    s_tm.add_triangle(9, 8, 1);

    // Subdivide mesh
    constexpr int k_refine = 3;
    for(int ii = 0; ii < k_refine; ++ii)
        s_tm.subdivide();

    // Compute UVs
    for(size_t ii = 0; ii < s_tm.vertex_count; ++ii)
    {
        // Compute positions in spherical coordinates
        const glm::vec3& pos = s_tm.positions[ii];
        float phi   = std::atan2(pos.z, pos.x);
        float theta = std::asin(pos.y); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        s_tm.uvs[ii] = {0.5f + phi / (2 * float(M_PI)), 0.5f -theta / float(M_PI)};
    }

    // Fix UV distorsions at seams and poles
    fix_warped_faces();
    if constexpr(k_refine>=2) fix_shared_pole_vertices(); // Will not work with a lesser refinement

    return s_tm.build_shape(layout, vdata, idata);
}

Extent make_origin(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                   [[maybe_unused]] Parameters* params)
{
    // Ignore parameters for now
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    s_m.add_vertex({0.f, 0.f, 0.f});
    s_m.add_vertex({1.f, 0.f, 0.f});
    s_m.add_vertex({0.f, 1.f, 0.f});
    s_m.add_vertex({0.f, 0.f, 1.f});

    s_m.add_line(0, 1);
    s_m.add_line(0, 2);
    s_m.add_line(0, 3);

    return s_m.build_shape(layout, vdata, idata, true);
}

} // namespace pg
} // namespace erwin