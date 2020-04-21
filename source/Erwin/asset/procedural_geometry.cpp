#include "asset/procedural_geometry.h"
#include "asset/mesh_fabricator.h"
#include "core/core.h"
#include "glm/glm.hpp"
#include "utils/constexpr_math.h"

namespace erwin
{
namespace pg
{

static MeshFabricator s_m;
static TriangleMeshFabricator s_tm;

Extent make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
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
                  Parameters* params)
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
                        Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Constants to get normalized vertex positions
    static constexpr float PHI = (1.0f + utils::fsqrt(5.0f)) / 2.0f;
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
        float phi = std::atan2(pos.y, pos.x);
        float theta = std::acos(pos.z); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        s_m.uvs[ii] = {0.5f + phi / (2 * float(M_PI)), theta / float(M_PI)};
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

// TMP: Ad-hoc implementation
Extent make_icosphere(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                      Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Constants to get normalized vertex positions
    static constexpr float PHI = (1.0f + utils::fsqrt(5.0f)) / 2.0f;
    static constexpr float ONE_N = 1.0f / (utils::fsqrt(2.0f + PHI)); // norm of any icosahedron vertex position
    static constexpr float PHI_N = PHI * ONE_N;

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

    // Compute UVs
    std::vector<glm::vec2> uvs;
    for(size_t ii = 0; ii < s_tm.vertex_count; ++ii)
    {
        // Compute positions in spherical coordinates
        const glm::vec3& pos = s_tm.positions[ii];
        float phi = std::atan2(pos.y, pos.x);
        float theta = std::acos(pos.z); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        s_tm.uvs[ii] = {0.5f + phi / (2 * float(M_PI)), theta / float(M_PI)};
    }

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
    constexpr int k_refine = 2;
    for(int ii = 0; ii < k_refine; ++ii)
        s_tm.subdivide();

    return s_tm.build_shape(layout, vdata, idata);
}

Extent make_origin(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                   Parameters* params)
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