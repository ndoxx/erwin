#pragma once

#include <filesystem>
#include <queue>

#include "widget/widget.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace editor
{

class MaterialAuthoringWidget: public Widget
{
public:
	MaterialAuthoringWidget();
	virtual ~MaterialAuthoringWidget();
	virtual void on_update(const erwin::GameClock& clock) override;

protected:
	virtual void on_imgui_render() override;

private:
	void load_texture_map(TextureMapType tm_type, const fs::path& filepath);
	void load_directory(const fs::path& dirpath);
	void pack_textures();
	void clear_texture_map(TextureMapType tm_type);
	void clear();
	void export_TOM(const fs::path& tom_path);

private:
	struct MaterialComposition;
	struct TOMExportTask;

	erwin::TextureHandle checkerboard_tex_;
	erwin::ShaderHandle PBR_packing_shader_;
	erwin::UniformBufferHandle packing_ubo_;
	std::unique_ptr<MaterialComposition> current_composition_;

	std::vector<TOMExportTask> tom_export_tasks_;
};

} // namespace editor