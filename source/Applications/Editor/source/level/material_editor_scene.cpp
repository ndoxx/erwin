#include "level/material_editor_scene.h"
#include "asset/asset_manager.h"
#include "render/common_geometry.h"

using namespace erwin;

namespace editor
{

bool MaterialEditorScene::on_load()
{
	reset_material();

	transform_ = {{0.f,0.f,0.f}, {0.f,0.f,0.f}, 1.f};

	directional_light_.set_position(47.626f, 49.027f);
	directional_light_.color         = {0.95f,0.85f,0.5f};
	directional_light_.ambient_color = {0.95f,0.85f,0.5f};
	directional_light_.ambient_strength = 0.1f;
	directional_light_.brightness = 3.7f;

	current_mesh_ = CommonGeometry::get_mesh("icosphere_pbr"_h).VAO;

	return true;
}

void MaterialEditorScene::on_unload()
{

}

void MaterialEditorScene::reset_material()
{
	ShaderHandle shader         = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	UniformBufferHandle ubo     = AssetManager::create_material_data_buffer<ComponentPBRMaterial>();
	Material mat = {"current"_h, {}, shader, ubo, sizeof(ComponentPBRMaterial::MaterialData)};

    current_material_.set_material(mat);
	current_material_.clear_flags();
	current_material_.material_data.uniform_metallic = 0.f;
	current_material_.material_data.uniform_roughness = 0.01f;
	current_material_.material_data.uniform_albedo = {1.0f,1.0f,1.0f,1.f};
}


} // namespace editor