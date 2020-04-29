#pragma once

#include <memory>

#include "widget/widget.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/texture_common.h"

namespace editor
{

class MaterialViewWidget;
class MaterialAuthoringWidget: public Widget
{
public:
	MaterialAuthoringWidget(MaterialViewWidget& material_view);
	virtual ~MaterialAuthoringWidget();

protected:
	virtual void on_imgui_render() override;

private:
	erwin::TextureGroup pack_textures();

private:
	struct MaterialComposition;

	erwin::TextureHandle checkerboard_tex_;
	erwin::ShaderHandle PBR_packing_shader_;
	erwin::UniformBufferHandle packing_ubo_;
	std::unique_ptr<MaterialComposition> current_composition_;

	MaterialViewWidget& material_view_;
};

} // namespace editor