#pragma once

#include "entity/reflection.h"
#include "asset/bounding.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

struct ComponentOBB
{
    std::array<glm::vec3, 8> vertices_w; // Vertices in world space
    glm::vec3 offset       = {0.f,0.f,0.f};
    glm::vec3 half         = {0.f,0.f,0.f};
    glm::mat4 model_matrix = glm::mat4(1.f);
    Extent extent_m; // Extent in model space
    float scale            = 1.f;
    bool display           = false;

    ComponentOBB(const Extent& extent);

    inline void init(const Extent& extent)
    {
        extent_m = extent;
        std::tie(offset, half) = bound::to_vectors(extent_m);
    }

    inline void update(const glm::mat4& parent_model_matrix, float _scale)
    {
    	model_matrix = glm::translate(parent_model_matrix, offset);
        scale = _scale;

    	bound::to_model_space_vertices(extent_m, vertices_w);
    	for(int ii=0; ii<8; ++ii)
    		vertices_w[ii] = model_matrix * glm::vec4(vertices_w[ii], 1.f);
    }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentOBB>(void* data);

} // namespace erwin
