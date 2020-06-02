#pragma once

#include <filesystem>
#include <queue>

#include "entity/reflection.h"
#include "entity/component_PBR_material.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"
#include "render/handles.h"

#include "level/scene_manager.h"

namespace fs = std::filesystem;

namespace editor
{

class MaterialEditorScene: public erwin::AbstractScene
{
protected:
    // Load all needed data in graphics memory
    virtual bool on_load() override;
    // Unload all graphics resources
    virtual void on_unload() override;

public:
    virtual ~MaterialEditorScene() override = default;

    // Cleanup all dead components and entities
    virtual void cleanup() override {}

    void reset_material();

	erwin::ComponentPBRMaterial current_material_;
	erwin::ComponentTransform3D transform_;
	erwin::ComponentDirectionalLight directional_light_;
	erwin::ComponentCamera3D camera_;
	erwin::ComponentTransform3D camera_transform_;
	erwin::VertexArrayHandle current_mesh_;

	struct Environment
	{
	    erwin::CubemapHandle environment_map;
	    erwin::CubemapHandle diffuse_irradiance_map;
	    erwin::CubemapHandle prefiltered_env_map;
	} environment;
};

} // namespace editor