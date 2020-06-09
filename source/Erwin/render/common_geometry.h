#pragma once
#include <functional>
#include "render/handles.h"
#include "core/core.h"
#include "asset/mesh.h"

namespace erwin
{

class CommonGeometry
{
public:
	using MeshVisitor = std::function<bool(const Mesh&)>;

	static VertexArrayHandle get_vertex_array(hash_t name);
	static const Extent& get_extent(hash_t name);

	static void visit_meshes(MeshVisitor visit);

private:
	friend class Application;

	static void init();
	static void shutdown();
};


} // namespace erwin