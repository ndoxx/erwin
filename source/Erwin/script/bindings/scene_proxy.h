#pragma once

#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "level/scene.h"

namespace erwin
{
namespace script
{

/*
	Proxy object to interact with the scene within scripts. This allows
	to have a separate interface for scene bindings.
*/
class SceneProxy
{
public:
	SceneProxy(Scene& scene): scene_(scene) {}

	inline Transform3D& get_transform(int ent_id)
    {
    	return scene_.get_component<ComponentTransform3D>(EntityID(ent_id)).local;
    }

    inline void force_update(int ent_id)
    {
    	scene_.try_add_component<DirtyTransformTag>(EntityID(ent_id));
    }

private:
	Scene& scene_;
};

} // namespace script
} // namespace erwin