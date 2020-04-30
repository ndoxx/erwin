#pragma once

#include <memory>
#include <filesystem>

#include "widget/widget.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/texture_common.h"

namespace fs = std::filesystem;

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
	void load_texture_map(TextureMapType tm_type, const fs::path& filepath);
	void load_directory(const fs::path& dirpath);
	void pack_textures();
	void clear_texture_map(TextureMapType tm_type);
	void clear();

private:
	struct MaterialComposition;

	erwin::TextureHandle checkerboard_tex_;
	erwin::ShaderHandle PBR_packing_shader_;
	erwin::UniformBufferHandle packing_ubo_;
	std::unique_ptr<MaterialComposition> current_composition_;

	MaterialViewWidget& material_view_;
};

} // namespace editor