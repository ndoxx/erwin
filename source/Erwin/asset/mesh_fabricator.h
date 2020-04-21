#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "asset/bounding.h"
#include "glm/glm.hpp"

namespace erwin
{

static constexpr size_t k_max_vertices = 1024;
static constexpr size_t k_max_indices = 8 * 1024;

enum SmoothFuncType : size_t
{
    MAX = 0,
    HEAVISIDE,
    LINEAR,
    COMPRESS_LINEAR,
    COMPRESS_QUADRATIC,

    COUNT
};

class BufferLayout;

class MeshFabricator
{
public:
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

    void build_normals();

    void build_tangents();

    Extent build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                       bool draw_lines = false);
};

struct pair_hash
{
    template <typename T1, typename T2> size_t operator()(const std::pair<T1, T2>& pair) const
    {
        return std::hash<T1>()(pair.first) ^ std::hash<T1>()(pair.second);
    }
};

class TriangleMeshFabricator
{
public:
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
        triangle_classes.insert({T1, 3*triangle_count});
        triangle_classes.insert({T2, 3*triangle_count});
        triangle_classes.insert({T3, 3*triangle_count});
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

    size_t get_mid_point(Edge edge);

    void set_triangle_by_index(size_t tri_index, const Triangle& T);

    void subdivide();

    // Make unique vertices for each triangle, flush to mesh fabricator (inefficient)
    void convert(MeshFabricator& mf);

    void build_normals();

    void build_tangents();

    Extent build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata);
};

} // namespace erwin