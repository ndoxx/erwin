#pragma once
#include <functional>
#include "render/handles.h"
#include "core/core.h"
#include "asset/bounding.h"

namespace erwin
{

// TODO: This is temporary (obviously). Replace by proper mesh class.
struct MeshStub
{
	VertexArrayHandle VAO;
	VertexBufferLayoutHandle layout;
	Extent extent;
	char name[32];
};

class CommonGeometry
{
public:
	using MeshVisitor = std::function<bool(const MeshStub&)>;

	static VertexArrayHandle get_vertex_array(hash_t name);
	static const Extent& get_extent(hash_t name);

	static void visit_meshes(MeshVisitor visit);

private:
	friend class Application;

	static void init();
	static void shutdown();
};


} // namespace erwin