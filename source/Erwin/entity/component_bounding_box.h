#pragma once

#include "entity/component.h"
#include "asset/bounding.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

class ComponentOBB: public Component
{
public:
	COMPONENT_DECLARATION(ComponentOBB);

    std::array<glm::vec3, 8> vertices_w; // Vertices in world space
    glm::vec3 offset;
    glm::vec3 half;
    glm::mat4 model_matrix;
    Extent extent_m; // Extent in model space

    ComponentOBB() = default;
    ComponentOBB(const Extent& extent);

    inline void update(const glm::mat4& parent_model_matrix)
    {
    	model_matrix = glm::translate(parent_model_matrix, offset);

    	bound::to_model_space_vertices(extent_m, vertices_w);
    	for(int ii=0; ii<8; ++ii)
    		vertices_w[ii] = model_matrix * glm::vec4(vertices_w[ii], 1.f);
    }

    inline void init(const Extent& extent)
    {
    	extent_m = extent;
		std::tie(offset, half) = bound::to_vectors(extent_m);
    }

	virtual bool init(void* description) override final;
	virtual void inspector_GUI() override final;
};

} // namespace erwin
