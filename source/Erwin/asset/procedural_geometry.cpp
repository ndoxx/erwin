#include "asset/procedural_geometry.h"
#include "core/core.h"
#include "core/intern_string.h"
#include "glm/glm.hpp"
#include "utils/constexpr_math.h"

#include "debug/logger.h"

#include <algorithm>
#include <map>
#include <numeric>

namespace erwin
{
namespace pg
{

static constexpr uint32_t k_max_vertices = 1024;
static constexpr uint32_t k_max_indices = 8 * 1024;

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

inline void add_vertex(const glm::vec3& position, const glm::vec2& uv = {0.f, 0.f})
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
    tl_storage.triangles[3 * tl_storage.triangle_count + 0] = a;
    tl_storage.triangles[3 * tl_storage.triangle_count + 1] = b;
    tl_storage.triangles[3 * tl_storage.triangle_count + 2] = c;
    ++tl_storage.triangle_count;
}

inline void add_line(uint32_t a, uint32_t b)
{
    tl_storage.lines[2 * tl_storage.line_count + 0] = a;
    tl_storage.lines[2 * tl_storage.line_count + 1] = b;
    ++tl_storage.line_count;
}

static void build_normals()
{
    // For each triangle in indices list
    for(uint32_t ii = 0; ii < tl_storage.triangle_count; ++ii)
    {
        uint32_t A = tl_storage.triangles[3 * ii + 0];
        uint32_t B = tl_storage.triangles[3 * ii + 1];
        uint32_t C = tl_storage.triangles[3 * ii + 2];

        // Get vertex position
        const glm::vec3& p1 = tl_storage.position[A];
        const glm::vec3& p2 = tl_storage.position[B];
        const glm::vec3& p3 = tl_storage.position[C];

        // Compute local normal using cross product
        glm::vec3 U(p2 - p1);
        glm::vec3 V(p3 - p1);
        glm::vec3 normal = glm::normalize(glm::cross(U, V));

        // Assign normal to each vertex
        tl_storage.normal[A] = normal;
        tl_storage.normal[B] = normal;
        tl_storage.normal[C] = normal;
    }
}

static void build_tangents()
{
    // For each triangle in indices list
    for(uint32_t ii = 0; ii < tl_storage.triangle_count; ++ii)
    {
        uint32_t A = tl_storage.triangles[3 * ii + 0];
        uint32_t B = tl_storage.triangles[3 * ii + 1];
        uint32_t C = tl_storage.triangles[3 * ii + 2];

        // Get positions, UVs and their deltas
        const glm::vec3& p1 = tl_storage.position[A];
        const glm::vec3& p2 = tl_storage.position[B];
        const glm::vec3& p3 = tl_storage.position[C];
        glm::vec3 e1(p2 - p1);
        glm::vec3 e2(p3 - p1);

        const glm::vec2& uv1 = tl_storage.uv[A];
        const glm::vec2& uv2 = tl_storage.uv[B];
        const glm::vec2& uv3 = tl_storage.uv[C];
        glm::vec2 delta_uv1(uv2 - uv1);
        glm::vec2 delta_uv2(uv3 - uv1);

        // Compute tangents
        float det_inv = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
        glm::vec3 tangent = det_inv * (e1 * delta_uv2.y - e2 * delta_uv1.y);

        // Assign tangent to each vertex
        tl_storage.tangent[A] = tangent;
        tl_storage.tangent[B] = tangent;
        tl_storage.tangent[C] = tangent;
    }
}

static Extent build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                          bool draw_lines = false)
{
    // Indirection tables for data interleaving
    constexpr uint32_t k_max_attrib = 4;
    float* containers[k_max_attrib];
    uint32_t components_count[k_max_attrib];
    std::fill(containers, containers + k_max_attrib, nullptr);
    std::fill(components_count, components_count + k_max_attrib, 0);

    // Check what we need to compute
    bool has_position = false;
    bool has_normal = false;
    bool has_tangent = false;
    bool has_uv = false;
    uint32_t elements_count = 0;
    for(const BufferLayoutElement& elt : layout)
    {
        switch(elt.name)
        {
        case "a_position"_h: {
            has_position = true;
            containers[elements_count] = &tl_storage.position[0][0];
            components_count[elements_count] = 3;
            break;
        }
        case "a_normal"_h: {
            has_normal = true;
            containers[elements_count] = &tl_storage.normal[0][0];
            components_count[elements_count] = 3;
            break;
        }
        case "a_tangent"_h: {
            has_tangent = true;
            containers[elements_count] = &tl_storage.tangent[0][0];
            components_count[elements_count] = 3;
            break;
        }
        case "a_uv"_h: {
            has_uv = true;
            containers[elements_count] = &tl_storage.uv[0][0];
            components_count[elements_count] = 2;
            break;
        }
        default:
            W_ASSERT_FMT(false, "Unknown attribute: %s", istr::resolve(elt.name).c_str());
        }
        ++elements_count;
    }

    // Get the total number of components per vertex
    uint32_t vertex_size = uint32_t(std::accumulate(components_count, components_count + k_max_attrib, 0));

    W_ASSERT(has_position, "Meshes must have a position attribute.");

    // Build attributes that need to be built
    if(has_normal)
        build_normals();
    if(has_tangent)
        build_tangents();

    // Export
    vdata.resize(tl_storage.vertex_count * vertex_size);
    // Interleave vertex data
    Extent dims;
    // For each vertex
    for(size_t ii = 0; ii < tl_storage.vertex_count; ++ii)
    {
        // Update dimensions in this loop for efficiency
        dims.update(tl_storage.position[ii]);

        uint32_t offset = 0;
        // For each layout element
        for(size_t jj = 0; jj < elements_count; ++jj)
        {
            uint32_t ccount = components_count[jj];
            float* data = containers[jj] + ii * ccount;
            // For each component of this layout element
            for(size_t kk = 0; kk < ccount; ++kk)
            {
                vdata[ii * vertex_size + offset + kk] = data[kk];
            }
            offset += ccount;
        }
    }

    if(draw_lines)
    {
        idata.resize(tl_storage.line_count * 2);
        std::copy(tl_storage.lines, tl_storage.lines + tl_storage.line_count * 2, idata.data());
    }
    else
    {
        idata.resize(tl_storage.triangle_count * 3);
        std::copy(tl_storage.triangles, tl_storage.triangles + tl_storage.triangle_count * 3, idata.data());
    }

    reset_storage();

    return dims;
}

Extent make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                 Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Setup position and UV vertex data, all th rest can be computed from this
    add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 1.0f});
    add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f});
    add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({0.5f, 0.5f, 0.5f}, {0.0f, 1.0f});
    add_vertex({0.5f, -0.5f, 0.5f}, {0.0f, 0.0f});
    add_vertex({0.5f, 0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});
    add_vertex({-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f});
    add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f});
    add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f});

    // Setup index data
    add_triangle(0, 1, 2);
    add_triangle(0, 2, 3);
    add_triangle(4, 5, 8);
    add_triangle(4, 8, 9);
    add_triangle(6, 7, 10);
    add_triangle(6, 10, 11);
    add_triangle(12, 13, 14);
    add_triangle(12, 14, 15);
    add_triangle(16, 17, 18);
    add_triangle(16, 18, 19);
    add_triangle(20, 21, 22);
    add_triangle(20, 22, 23);

    // Build interleaved vertex data according to input specifications
    return build_shape(layout, vdata, idata);
}

Extent make_cube_lines(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                       Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    // Setup position and UV vertex data, all th rest can be computed from this
    add_vertex({0.5f, -0.5f, 0.5f}, {1.0f, 1.0f});
    add_vertex({0.5f, 0.5f, 0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f});
    add_vertex({0.5f, -0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({0.5f, 0.5f, -0.5f}, {1.0f, 1.0f});
    add_vertex({-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f});
    add_vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f});

    // Setup line indices
    add_line(0, 3);
    add_line(0, 1);
    add_line(0, 4);
    add_line(6, 5);
    add_line(6, 7);
    add_line(6, 2);
    add_line(3, 7);
    add_line(3, 2);
    add_line(4, 7);
    add_line(4, 5);
    add_line(1, 2);
    add_line(1, 5);

    // Build interleaved vertex data according to input specifications
    return build_shape(layout, vdata, idata, true);
}

Extent make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                  Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    add_vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f});
    add_vertex({1.0f, -1.0f, 0.0f}, {1.0f, 0.0f});
    add_vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f});
    add_vertex({-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f});

    add_triangle(0, 1, 2);
    add_triangle(2, 3, 0);

    return build_shape(layout, vdata, idata);
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

    add_vertex({-ONE_N, PHI_N, 0.f});
    add_vertex({ONE_N, PHI_N, 0.f});
    add_vertex({-ONE_N, -PHI_N, 0.f});
    add_vertex({ONE_N, -PHI_N, 0.f});
    add_vertex({0.f, -ONE_N, PHI_N});
    add_vertex({0.f, ONE_N, PHI_N});
    add_vertex({0.f, -ONE_N, -PHI_N});
    add_vertex({0.f, ONE_N, -PHI_N});
    add_vertex({PHI_N, 0.f, -ONE_N});
    add_vertex({PHI_N, 0.f, ONE_N});
    add_vertex({-PHI_N, 0.f, -ONE_N});
    add_vertex({-PHI_N, 0.f, ONE_N});

    // Compute UVs
    for(size_t ii = 0; ii < tl_storage.vertex_count; ++ii)
    {
        // Compute positions in spherical coordinates
        const glm::vec3& pos = tl_storage.position[ii];
        float phi = std::atan2(pos.y, pos.x);
        float theta = std::acos(pos.z); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        tl_storage.uv[ii] = {0.5f + phi / (2 * float(M_PI)), theta / float(M_PI)};
    }

    add_triangle(0, 11, 5);
    add_triangle(0, 5, 1);
    add_triangle(0, 1, 7);
    add_triangle(0, 7, 10);
    add_triangle(0, 10, 11);
    add_triangle(1, 5, 9);
    add_triangle(5, 11, 4);
    add_triangle(11, 10, 2);
    add_triangle(10, 7, 6);
    add_triangle(7, 1, 8);
    add_triangle(3, 9, 4);
    add_triangle(3, 4, 2);
    add_triangle(3, 2, 6);
    add_triangle(3, 6, 8);
    add_triangle(3, 8, 9);
    add_triangle(4, 9, 5);
    add_triangle(2, 4, 11);
    add_triangle(6, 2, 10);
    add_triangle(8, 6, 7);
    add_triangle(9, 8, 1);

    return build_shape(layout, vdata, idata);
}

// TMP: Ad-hoc implementation

struct pair_hash
{
    template <typename T1, typename T2> size_t operator()(const std::pair<T1, T2>& pair) const
    {
        return std::hash<T1>()(pair.first) ^ std::hash<T1>()(pair.second);
    }
};

struct TriangleMesh
{
    using TriangleMap = std::multimap<size_t, size_t>;
    using TriangleRange = std::pair<TriangleMap::iterator, TriangleMap::iterator>;
    using Edge = std::pair<size_t, size_t>;
    using Triangle = std::array<size_t, 3>;

    TriangleMap triangle_classes;
    std::unordered_map<Edge, size_t, pair_hash> lookup;
    std::vector<size_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;

    inline void push_triangle(size_t T1, size_t T2, size_t T3)
    {
        size_t cur_index = indices.size();
        indices.push_back(T1);
        indices.push_back(T2);
        indices.push_back(T3);
        // Associate each vertex position to the index of triangle
        triangle_classes.insert({T1, cur_index});
        triangle_classes.insert({T2, cur_index});
        triangle_classes.insert({T3, cur_index});
    }

    inline size_t push_vertex(const glm::vec3& position, const glm::vec2& uv)
    {
        size_t ret = positions.size();
        positions.push_back(position);
        uvs.push_back(uv);
        return ret;
    }

    inline glm::vec3 mid_position(Edge edge) { return 0.5f * (positions[edge.first] + positions[edge.second]); }

    inline glm::vec2 mid_uv(Edge edge) { return 0.5f * (uvs[edge.first] + uvs[edge.second]); }

    inline Edge ordered_edge(const Edge& edge)
    {
        if(edge.first < edge.second)
            return {edge.first, edge.second};
        else
            return {edge.second, edge.first};
    }

    size_t get_mid_point(Edge edge)
    {
        auto it = lookup.find(edge);

        size_t midp;
        if(it != lookup.end())
            midp = it->second;
        else
        {
            // Normalize position to force it on the unit sphere
            midp = push_vertex(glm::normalize(mid_position(edge)), mid_uv(edge));
            lookup.insert(std::make_pair(edge, midp));
        }

        return midp;
    }

    void set_triangle_by_index(size_t tri_index, const Triangle& T)
    {
        // W_ASSERT(tri_index+2<indices.size(), "Index out of bounds during triangle assignment operation.");
        // W_ASSERT(tri_index%3 == 0, "Index is not a triangle index (index%3 != 0)");
        for(size_t ii = 0; ii < 3; ++ii)
        {
            // Remove old triangle class association
            TriangleRange range = triangle_classes.equal_range(indices[tri_index + ii]);
            for(auto it = range.first; it != range.second; ++it)
            {
                if(it->second == tri_index)
                {
                    triangle_classes.erase(it);
                    break;
                }
            }

            // Update triangle with new indices
            indices[tri_index + ii] = T[ii];
            // Associate each vertex position to the index of triangle
            triangle_classes.insert({T[ii], tri_index});
        }
    }

    void subdivide()
    {
        // Copy indices list
        std::vector<size_t> indices_copy = indices;
        // for each triangle in mesh
        for(size_t ii = 0; ii + 2 < indices_copy.size(); ii += 3)
        {
            // Triangle points indices
            size_t a = indices_copy[ii + 0];
            size_t b = indices_copy[ii + 1];
            size_t c = indices_copy[ii + 2];

            // Try to find mid points in lookup first
            // If can't find, create new vertex
            size_t m_ab = get_mid_point(ordered_edge({a, b}));
            size_t m_bc = get_mid_point(ordered_edge({b, c}));
            size_t m_ca = get_mid_point(ordered_edge({c, a}));

            // add new triangles
            push_triangle(m_ca, a, m_ab);
            push_triangle(m_ab, b, m_bc);
            push_triangle(m_bc, c, m_ca);
            // reassign old triangle to make center triangle
            set_triangle_by_index(ii, {m_ab, m_bc, m_ca});
        }
    }

    void flush()
    {
    	// For each triangle, copy and push its 3 vertices
    	for(size_t ii=0; ii+2<indices.size(); ii+=3)
    	{
    		for(size_t jj=0; jj<3; ++jj)
    		{
    			size_t index = indices[ii+jj];
    			add_vertex(positions[index], uvs[index]);
    		}
    		add_triangle(uint32_t(ii+0),uint32_t(ii+1),uint32_t(ii+2));
    	}
    }
};

Extent make_icosphere(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                      Parameters* params)
{
    // Ignore parameters for now, only z-plane available
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    TriangleMesh tm;

    // Constants to get normalized vertex positions
    static constexpr float PHI = (1.0f + utils::fsqrt(5.0f)) / 2.0f;
    static constexpr float ONE_N = 1.0f / (utils::fsqrt(2.0f + PHI)); // norm of any icosahedron vertex position
    static constexpr float PHI_N = PHI * ONE_N;

    // Start with an icosahedron
    tm.positions = {
        {-ONE_N, PHI_N, 0.f}, {ONE_N, PHI_N, 0.f}, {-ONE_N, -PHI_N, 0.f}, {ONE_N, -PHI_N, 0.f},
        {0.f, -ONE_N, PHI_N}, {0.f, ONE_N, PHI_N}, {0.f, -ONE_N, -PHI_N}, {0.f, ONE_N, -PHI_N},
        {PHI_N, 0.f, -ONE_N}, {PHI_N, 0.f, ONE_N}, {-PHI_N, 0.f, -ONE_N}, {-PHI_N, 0.f, ONE_N},
    };

    // Compute UVs
    std::vector<glm::vec2> uvs;
    for(size_t ii = 0; ii < tm.positions.size(); ++ii)
    {
        // Compute positions in spherical coordinates
        const glm::vec3& pos = tm.positions[ii];
        float phi = std::atan2(pos.y, pos.x);
        float theta = std::acos(pos.z); // Position is normalized -> r = 1
        // Remap latitude and longitude angles to [0,1] and use them as UVs
        tm.uvs.push_back({0.5f + phi / (2 * float(M_PI)), theta / float(M_PI)});
    }

    tm.push_triangle(0, 11, 5);
    tm.push_triangle(0, 5, 1);
    tm.push_triangle(0, 1, 7);
    tm.push_triangle(0, 7, 10);
    tm.push_triangle(0, 10, 11);
    tm.push_triangle(1, 5, 9);
    tm.push_triangle(5, 11, 4);
    tm.push_triangle(11, 10, 2);
    tm.push_triangle(10, 7, 6);
    tm.push_triangle(7, 1, 8);
    tm.push_triangle(3, 9, 4);
    tm.push_triangle(3, 4, 2);
    tm.push_triangle(3, 2, 6);
    tm.push_triangle(3, 6, 8);
    tm.push_triangle(3, 8, 9);
    tm.push_triangle(4, 9, 5);
    tm.push_triangle(2, 4, 11);
    tm.push_triangle(6, 2, 10);
    tm.push_triangle(8, 6, 7);
    tm.push_triangle(9, 8, 1);

    // Subdivide mesh
    constexpr int k_refine = 2;
    for(int ii = 0; ii < k_refine; ++ii)
        tm.subdivide();

    tm.flush();

    return build_shape(layout, vdata, idata);
}

Extent make_origin(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                   Parameters* params)
{
    // Ignore parameters for now
    W_ASSERT(params == nullptr, "Parameters unsupported for now.");

    add_vertex({0.f, 0.f, 0.f});
    add_vertex({1.f, 0.f, 0.f});
    add_vertex({0.f, 1.f, 0.f});
    add_vertex({0.f, 0.f, 1.f});

    add_line(0, 1);
    add_line(0, 2);
    add_line(0, 3);

    return build_shape(layout, vdata, idata, true);
}

} // namespace pg
} // namespace erwin