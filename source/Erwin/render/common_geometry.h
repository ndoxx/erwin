#pragma once
#include "render/handles.h"
#include "core/core.h"
#include "asset/mesh.h"

namespace erwin
{

class CommonGeometry
{
public:
	static VertexArrayHandle get_vertex_array(hash_t name);
	static const Dimensions& get_dimensions(hash_t name);

private:
	friend class Application;

	static void init();
	static void shutdown();
};


} // namespace erwin