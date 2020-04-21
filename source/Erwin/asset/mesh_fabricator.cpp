#include "asset/mesh_fabricator.h"
#include "core/intern_string.h"
#include "render/buffer_layout.h"

#include <algorithm>
#include <map>
#include <numeric>

#include "debug/logger.h"
#include "glm/gtx/string_cast.hpp"

namespace erwin
{

using SmoothFunc = std::function<float(float)>;

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

void MeshFabricator::build_normals()
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

void MeshFabricator::build_tangents()
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

Extent MeshFabricator::build_shape(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata,
                                   bool draw_lines)
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

size_t TriangleMeshFabricator::get_mid_point(Edge edge)
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

void TriangleMeshFabricator::set_triangle_by_index(size_t tri_index, const Triangle& T)
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

void TriangleMeshFabricator::subdivide()
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
void TriangleMeshFabricator::convert(MeshFabricator& mf)
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

void TriangleMeshFabricator::build_normals()
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

void TriangleMeshFabricator::build_tangents()
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

Extent TriangleMeshFabricator::build_shape(const BufferLayout& layout, std::vector<float>& vdata,
                                           std::vector<uint32_t>& idata)
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

} // namespace erwin