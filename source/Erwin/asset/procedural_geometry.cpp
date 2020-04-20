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

using SmoothFunc = std::function<float(float)>;

enum SmoothFuncType : size_t
{
    MAX = 0,
    HEAVISIDE,
    LINEAR,
    COMPRESS_LINEAR,
    COMPRESS_QUADRATIC,

    COUNT
};

static std::array<SmoothFunc, SmoothFuncType::COUNT> s_smooth_funcs = {[](float x) {
                                                                           (void)x;
                                                                           return 1.0f;
                                                                       },
                                                                       [](float x) {
                                                                           x = 1.f - std::abs(x);
                                                                           return (x < 0.1f) ? 1.0f : 0.0f;
                                                                       },
                                                                       [](float x) {
                                                                           x = 1.f - std::abs(x);
                                                                           return 1.0f - x;
                                                                       },
                                                                       [](float x) {
                                                                           x = 1.f - std::abs(x);
                                                                           return (x < 0.5f) ? 1.0f : -2.0f * x + 2.0f;
                                                                       },
                                                                       [](float x) {
                                                                           x = 1.f - x;
                                                                           float a = 0.75f;
                                                                           return a * x * x - (a + 1.0f) * x + 1.0f;
                                                                       }};

static constexpr uint32_t k_max_vertices = 1024;
static constexpr uint32_t k_max_indices = 8 * 1024;

struct MeshFabricator
{
    glm::vec3 positions[k_max_vertices];
    glm::vec3 normals[k_max_vertices];
    glm::vec3 tangents[k_max_vertices];
    glm::vec2 uvs[k_max_vertices];
    uint32_t triangles[k_max_indices];
    uint32_t lines[k_max_indices];
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t line_count;

    inline void reset()
    {
        vertex_count = 0;
        triangle_count = 0;
        line_count = 0;
    }

    inline void add_vertex(const glm::vec3& position, const glm::vec2& uv = {0.f, 0.f})
    {
        // W_ASSERT(vertex_count+1<k_max_vertices, "Vertex data is full.");
        positions[vertex_count] = position;
        uvs[vertex_count] = uv;
        ++vertex_count;
    }

    inline void add_vertex(glm::vec3&& position, glm::vec2&& uv)
    {
        // W_ASSERT(vertex_count+1<k_max_vertices, "Vertex data is full.");
        positions[vertex_count] = std::move(position);
        uvs[vertex_count] = std::move(uv);
        ++vertex_count;
    }

    inline void add_triangle(uint32_t a, uint32_t b, uint32_t c)
    {
        triangles[3 * triangle_count + 0] = a;
        triangles[3 * triangle_count + 1] = b;
        triangles[3 * triangle_count + 2] = c;
        ++triangle_count;
    }

    inline void add_line(uint32_t a, uint32_t b)
    {
        lines[2 * line_count + 0] = a;
        lines[2 * line_count + 1] = b;
        ++line_count;
    }

    void build_normals()
    {
        // For each triangle in indices list
        for(uint32_t ii = 0; ii < triangle_count; ++ii)
        {
            uint32_t A = triangles[3 * ii + 0];
            uint32_t B = triangles[3 * ii + 1];
            uint32_t C = triangles[3 * ii + 2];

            // Get vertex position
            const glm::vec3& p1 = positions[A];
            const glm::vec3& p2 = positions[B];
            const glm::vec3& p3 = positions[C];

            // Compute local normal using cross product
            glm::vec3 U(p2 - p1);
            glm::vec3 V(p3 - p1);
            glm::vec3 normal = glm::normalize(glm::cross(U, V));

            // Assign normal to each vertex
            normals[A] = normal;
            normals[B] = normal;
            normals[C] = normal;
        }
    }

    void build_tangents()
    {
        // For each triangle in indices list
        for(uint32_t ii = 0; ii < triangle_count; ++ii)
        {
            uint32_t A = triangles[3 * ii + 0];
            uint32_t B = triangles[3 * ii + 1];
            uint32_t C = triangles[3 * ii + 2];

            // Get positions, UVs and their deltas
            const glm::vec3& p1 = positions[A];
            const glm::vec3& p2 = positions[B];
            const glm::vec3& p3 = positions[C];
            glm::vec3 e1(p2 - p1);
            glm::vec3 e2(p3 - p1);

            const glm::vec2& uv1 = uvs[A];
            const glm::vec2& uv2 = uvs[B];
            const glm::vec2& uv3 = uvs[C];
            glm::vec2 delta_uv1(uv2 - uv1);
            glm::vec2 delta_uv2(uv3 - uv1);

            // Compute tangents
            float det_inv = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
            glm::vec3 tangent = det_inv * (e1 * delta_uv2.y - e2 * delta_uv1.y);

            // Assign tangent to each vertex
            tangents[A] = tangent;
            tangents[B] = tangent;
            tangents[C] = tangent;
        }
    }

    Extent build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
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
                containers[elements_count] = &positions[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_normal"_h: {
                has_normal = true;
                containers[elements_count] = &normals[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_tangent"_h: {
                has_tangent = true;
                containers[elements_count] = &tangents[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_uv"_h: {
                has_uv = true;
                containers[elements_count] = &uvs[0][0];
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
        vdata.resize(vertex_count * vertex_size);
        // Interleave vertex data
        Extent dims;
        // For each vertex
        for(size_t ii = 0; ii < vertex_count; ++ii)
        {
            // Update dimensions in this loop for efficiency
            dims.update(positions[ii]);

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
            idata.resize(line_count * 2);
            std::copy(lines, lines + line_count * 2, idata.data());
        }
        else
        {
            idata.resize(triangle_count * 3);
            std::copy(triangles, triangles + triangle_count * 3, idata.data());
        }

        reset();

        return dims;
    }
};

struct pair_hash
{
    template <typename T1, typename T2> size_t operator()(const std::pair<T1, T2>& pair) const
    {
        return std::hash<T1>()(pair.first) ^ std::hash<T1>()(pair.second);
    }
};

struct TriangleMeshFabricator
{
    using TriangleMap = std::multimap<size_t, size_t>;
    using TriangleRange = std::pair<TriangleMap::iterator, TriangleMap::iterator>;
    using Edge = std::pair<size_t, size_t>;
    using Triangle = std::array<size_t, 3>;

    TriangleMap triangle_classes;
    std::unordered_map<Edge, size_t, pair_hash> lookup;
    glm::vec3 positions[k_max_vertices];
    glm::vec3 normals[k_max_vertices];
    glm::vec3 tangents[k_max_vertices];
    glm::vec2 uvs[k_max_vertices];
    size_t indices[k_max_indices];

    size_t vertex_count = 0;
    size_t triangle_count = 0;

    inline void reset()
    {
        vertex_count = 0;
        triangle_count = 0;
        triangle_classes.clear();
        lookup.clear();
    }

    inline void add_triangle(size_t T1, size_t T2, size_t T3)
    {
        indices[3 * triangle_count + 0] = T1;
        indices[3 * triangle_count + 1] = T2;
        indices[3 * triangle_count + 2] = T3;

        // Associate each vertex position to the index of triangle
        triangle_classes.insert({T1, triangle_count});
        triangle_classes.insert({T2, triangle_count});
        triangle_classes.insert({T3, triangle_count});
        ++triangle_count;
    }

    inline size_t add_vertex(const glm::vec3& position, const glm::vec2& uv = {0.f, 0.f})
    {
        positions[vertex_count] = position;
        uvs[vertex_count] = uv;
        ++vertex_count;
        return vertex_count - 1;
    }

    // visit a range of indices corresponding to triangles that contain a given vertex
    inline void traverse_triangle_class(size_t index, std::function<void(TriangleRange)> visitor)
    {
        visitor(triangle_classes.equal_range(index));
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
            midp = add_vertex(glm::normalize(mid_position(edge)), mid_uv(edge));
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
        size_t num_indices = 3 * triangle_count;
        size_t indices_copy[k_max_indices];
        memcpy(indices_copy, indices, k_max_indices);

        // for each triangle in mesh
        for(size_t ii = 0; ii + 2 < num_indices; ii += 3)
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
            add_triangle(m_ca, a, m_ab);
            add_triangle(m_ab, b, m_bc);
            add_triangle(m_bc, c, m_ca);
            // reassign old triangle to make center triangle
            set_triangle_by_index(ii, {m_ab, m_bc, m_ca});
        }
    }

    // Make unique vertices for each triangle, flush to mesh fabricator (inefficient)
    void convert(MeshFabricator& mf)
    {
        // For each triangle, copy and push its 3 vertices
        for(size_t ii = 0; ii + 2 < 3 * triangle_count; ii += 3)
        {
            for(size_t jj = 0; jj < 3; ++jj)
            {
                size_t index = indices[ii + jj];
                mf.add_vertex(positions[index], uvs[index]);
            }
            mf.add_triangle(uint32_t(ii + 0), uint32_t(ii + 1), uint32_t(ii + 2));
        }
    }

    void build_normals()
    {
        // For each vertex
        for(size_t ii = 0; ii < vertex_count; ++ii)
        {
            glm::vec3 normal0(0);
            // For each triangle that contain this vertex
            traverse_triangle_class(ii, [&](TriangleRange range) {
                for(auto it = range.first; it != range.second; ++it)
                {
                    size_t tri_index = it->second;
                    // Get positions
                    const glm::vec3& p1 = positions[indices[tri_index + 0]];
                    const glm::vec3& p2 = positions[indices[tri_index + 1]];
                    const glm::vec3& p3 = positions[indices[tri_index + 2]];
                    // Compute local normal using cross product
                    glm::vec3 e1(p2 - p1);
                    glm::vec3 e2(p3 - p1);
                    glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
                    normal0 += normal;
                }
            });

            // Assign mean normal to vertex
            normals[ii] = glm::normalize(normal0);
        }
    }

    void build_tangents()
    {
        // For each vertex
        for(size_t ii = 0; ii < vertex_count; ++ii)
        {
            glm::vec3 tangent0(0);
            // For each triangle that contain this vertex
            traverse_triangle_class(ii, [&](TriangleRange range) {
                for(auto it = range.first; it != range.second; ++it)
                {
                    size_t tri_index = it->second;

                    // Get positions
                    const glm::vec3& p1 = positions[indices[tri_index + 0]];
                    const glm::vec3& p2 = positions[indices[tri_index + 1]];
                    const glm::vec3& p3 = positions[indices[tri_index + 2]];

                    glm::vec3 e1(p2 - p1);
                    glm::vec3 e2(p3 - p1);

                    // Get UVs
                    const glm::vec2& uv1 = uvs[indices[tri_index + 0]];
                    const glm::vec2& uv2 = uvs[indices[tri_index + 1]];
                    const glm::vec2& uv3 = uvs[indices[tri_index + 2]];
                    // Compute deltas
                    glm::vec2 deltaUV1(uv2 - uv1);
                    glm::vec2 deltaUV2(uv3 - uv1);
                    // Compute local tangent
                    float det_inv = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                    glm::vec3 tangent(e1 * deltaUV2.y - e2 * deltaUV1.y);
                    tangent *= det_inv;
                    tangent0 += tangent;
                }
            });

            tangents[ii] = glm::normalize(tangent0);
        }
    }

    Extent build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata)
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
                containers[elements_count] = &positions[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_normal"_h: {
                has_normal = true;
                containers[elements_count] = &normals[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_tangent"_h: {
                has_tangent = true;
                containers[elements_count] = &tangents[0][0];
                components_count[elements_count] = 3;
                break;
            }
            case "a_uv"_h: {
                has_uv = true;
                containers[elements_count] = &uvs[0][0];
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
        vdata.resize(vertex_count * vertex_size);
        // Interleave vertex data
        Extent dims;
        // For each vertex
        for(size_t ii = 0; ii < vertex_count; ++ii)
        {
            // Update dimensions in this loop for efficiency
            dims.update(positions[ii]);

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

        idata.resize(triangle_count * 3);
        std::copy(indices, indices + triangle_count * 3, idata.data());

        reset();

        return dims;
    }
};

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

    s_tm.convert(s_m);

    // return s_tm.build_shape(layout, vdata, idata);
    return s_m.build_shape(layout, vdata, idata);
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