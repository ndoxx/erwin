#pragma once
#include "render/handles.h"
#include "core/wtypes.h"

namespace erwin
{

class CommonGeometry
{
public:
	static void init();
	static void shutdown();

	static VertexArrayHandle get_vertex_array(hash_t name);
};


} // namespace erwin