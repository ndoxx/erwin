#include "entity/entity.h"

namespace erwin
{

Entity::Entity(EntityID id):
id_(id)
{

}

Entity::~Entity()
{
	// TMP: For now, entities own their components
	// Destroy all components
	for(auto&& [cid, pcmp]: components_)
		delete pcmp;
}

} // namespace erwin