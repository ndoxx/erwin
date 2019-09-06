#include <map>

#include "wmesh.h"
#include "mesh.h"

namespace erwin
{

struct WMeshCompare
{
   bool operator() (const WMesh& lhs, const WMesh& rhs) const
   {
       return lhs.id_ < rhs.id_;
   }
};

//static std::map<WMesh, Mesh, WMeshCompare> MESHES;
static std::map<std::size_t, Mesh> MESHES;
std::size_t WMesh::current_id_ = 0;

WMesh::WMesh():
id_(++current_id_)
{

}

WMesh::~WMesh()
{

}

std::size_t WMesh::submesh_count() const
{
    return MESHES.at(id_).submesh_count();
}

std::size_t WMesh::total_vertex_count() const
{
    return MESHES.at(id_).total_vertex_count();
}

std::size_t WMesh::total_index_count() const
{
    return MESHES.at(id_).total_index_count();
}

std::size_t WMesh::vertex_count(uint32_t submesh) const
{
    return MESHES.at(id_).vertex_count(submesh);
}

std::size_t WMesh::index_count(uint32_t submesh) const
{
    return MESHES.at(id_).index_count(submesh);
}

void* WMesh::vertices(uint32_t submesh) const
{
    return MESHES.at(id_).vertices(submesh);
}

uint32_t* WMesh::indices(uint32_t submesh) const
{
    return MESHES.at(id_).indices(submesh);
}

DrawPrimitive WMesh::intended_primitive() const
{
    return MESHES.at(id_).intended_primitive();
}

const Extent& WMesh::total_extent() const
{
    return MESHES.at(id_).total_extent();
}

const Extent& WMesh::extent(uint32_t submesh) const
{
    return MESHES.at(id_).extent(submesh);
}


} // namespace erwin
