#include <limits>

#include "mesh.h"

namespace erwin
{

Mesh::Mesh(const BufferLayout& layout):
layout_(layout)
{

}

bool Mesh::read_stream(std::istream& stream, const BufferLayout& layout, std::size_t vertex_count, std::size_t index_count)
{
    layout_ = layout;

    std::size_t vsize  = vertex_count*layout_.get_stride();
    std::size_t fcount = vsize / sizeof(float);
    vertex_data_.resize(fcount);
    indices_.resize(index_count);

    // Read vertex data
    stream.read(reinterpret_cast<char*>(&vertex_data_[0]), vsize);
    // Read index data
    stream.read(reinterpret_cast<char*>(&indices_[0]), index_count*sizeof(uint32_t));

    if(layout_.begin()->name == "a_position"_h)
        compute_extent();

    return stream.good();
}

Mesh::~Mesh()
{

}

void Mesh::compute_extent()
{

}

} // namespace erwin
