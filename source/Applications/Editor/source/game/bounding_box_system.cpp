#include "game/bounding_box_system.h"

namespace erwin
{

void BoundingBoxSystem::update(const GameClock& clock)
{
	for(auto&& cmp_tuple: components_)
	{
		ComponentTransform3D* transform = eastl::get<ComponentTransform3D*>(cmp_tuple);
		ComponentOBB* OBB = eastl::get<ComponentOBB*>(cmp_tuple);
		OBB->update(transform->get_model_matrix());
	}
}

} // namespace erwin