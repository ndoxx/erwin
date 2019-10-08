#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace erwin
{

typedef std::array<float, 6> Extent;

enum class DrawPrimitive
{
    Lines = 2,
    Triangles = 3,
    Quads = 4
};

#ifdef W_DEBUG
class Mesh;
#endif

// Handle for meshes
class WMesh
{
public:
    WMesh();
    ~WMesh();

    std::size_t submesh_count() const;
    std::size_t total_vertex_count() const;
    std::size_t total_index_count() const;
    std::size_t vertex_count(uint32_t submesh=0) const;
    std::size_t index_count(uint32_t submesh=0) const;

    // Get pointer to vertex data for given submesh
    void* vertices(uint32_t submesh=0) const;
    // Get array of indices for given submesh
    uint32_t* indices(uint32_t submesh=0) const;
    // Get enum type of intended draw primitive to use
    DrawPrimitive intended_primitive() const;
    // Get x,y,z extent of complete mesh in model space
    const Extent& total_extent() const;
    // Get x,y,z extent of submesh in model space
    const Extent& extent(uint32_t submesh=0) const;

    // DBG
#ifdef W_DEBUG
    // Dereference this handle to get the underlying mesh object
    const Mesh& operator*();
#endif

private:
    friend struct WMeshCompare;
    static std::size_t current_id_;
    std::size_t id_;
};

} // namespace erwin
