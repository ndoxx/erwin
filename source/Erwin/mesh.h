#pragma once

#include <cstdint>
#include <vector>
#include <istream>

#include "wmesh.h"
#include "vertex_layout.h"

namespace erwin
{

struct Submesh
{
    std::size_t vertex_offset;
    std::size_t index_offset;
    std::size_t vertex_count;
    std::size_t index_count;
    Extent extent;
};

class Mesh
{
public:
    Mesh();
    Mesh(const BufferLayout& layout);
    ~Mesh();

    bool read_stream(std::istream& stream, const BufferLayout& layout, std::size_t vertex_count, std::size_t index_count);


    inline std::size_t submesh_count() const      { return submeshes_.size(); }
    inline std::size_t total_vertex_count() const { return vertex_data_.size()*sizeof(float)/layout_.get_stride(); }
    inline std::size_t total_index_count() const  { return indices_.size(); }
    inline std::size_t vertex_count(uint32_t submesh=0) const { return submeshes_.at(submesh).vertex_count; }
    inline std::size_t index_count(uint32_t submesh=0) const  { return submeshes_.at(submesh).index_count; }

    // Get pointer to vertex data for given submesh
    inline void* vertices(uint32_t submesh=0)     { return vertex_data_.data(); }
    // Get array of indices for given submesh
    inline uint32_t* indices(uint32_t submesh=0)  { return indices_.data(); }
    // Get enum type of intended draw primitive to use for given submesh
    inline DrawPrimitive intended_primitive() const       { return primitive_; }
    // Get x,y,z extent of submesh in model space
    inline const Extent& extent(uint32_t submesh=0) const { return submeshes_.at(submesh).extent; }
    // Get x,y,z extent of complete mesh in model space
    inline const Extent& total_extent() const     { return total_extent_; }

private:
    void compute_extent();

private:
    std::vector<float> vertex_data_;
    std::vector<uint32_t> indices_;
    std::vector<Submesh> submeshes_;
    Extent total_extent_;
    BufferLayout layout_;
    DrawPrimitive primitive_;
};

} // namespace erwin
