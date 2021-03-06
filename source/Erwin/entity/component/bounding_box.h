#pragma once

#include <tuple>

#include "asset/bounding.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

struct ComponentOBB
{
    std::array<glm::vec3, 8> vertices_w; // Vertices in world space
    glm::vec3 offset = {0.f, 0.f, 0.f};
    glm::vec3 half = {0.f, 0.f, 0.f};
    glm::vec3 scale = {1.f, 1.f, 1.f};
    glm::mat4 model_matrix = glm::mat4(1.f);
    Extent extent_m; // Extent in model space
    float uniform_scale = 1.f;
    bool display = false;

    ComponentOBB();
    explicit ComponentOBB(const Extent& extent);

    inline void init(const Extent& extent)
    {
        extent_m = extent;
        scale = {extent_m.xmax() - extent_m.xmin(), extent_m.ymax() - extent_m.ymin(),
                 extent_m.zmax() - extent_m.zmin()};
        std::tie(offset, half) = bound::to_vectors(extent_m);
    }

    inline void update(const glm::mat4& parent_model_matrix, float _uniform_scale)
    {
        model_matrix = glm::translate(parent_model_matrix, offset);
        uniform_scale = _uniform_scale;

        bound::to_model_space_vertices(extent_m, vertices_w);
        for(size_t ii = 0; ii < 8; ++ii)
            vertices_w[ii] = model_matrix * glm::vec4(vertices_w[ii], 1.f);
    }
};

} // namespace erwin
