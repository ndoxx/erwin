#pragma once

#include "asset/handles.h"
#include "render/texture_atlas.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"

namespace erwin
{


class AssetManager
{
public:

	static void init(memory::HeapArea& area);
	static void shutdown();

	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static void release_texture_atlas(TextureAtlasHandle handle);
	static const TextureAtlas& get(TextureAtlasHandle handle);
};


} // namespace erwin