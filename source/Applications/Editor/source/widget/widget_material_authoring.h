#pragma once

#include "widget/widget.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/texture_common.h"

namespace editor
{

class MaterialAuthoringWidget: public Widget
{
public:
	MaterialAuthoringWidget();
	virtual ~MaterialAuthoringWidget();

protected:
	virtual void on_imgui_render() override;

private:
	erwin::TextureHandle checkerboard_tex_;

	struct MaterialComposition
	{
		TMEnum texture_maps = TMF_NONE;
		erwin::TextureGroup textures;

        inline bool has_map(TextureMapType map_type)   { return bool(texture_maps & (1 << map_type)); }
        inline void set_map(TextureMapType map_type)   { texture_maps |= (1 << map_type); }
        inline void clear_map(TextureMapType map_type) { texture_maps &= ~(1 << map_type); textures[size_t(map_type)] = {}; }
    };

	MaterialComposition current_composition_;
};

} // namespace editor