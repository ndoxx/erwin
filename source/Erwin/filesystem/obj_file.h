#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <istream>
#include "glm/glm.hpp"

namespace erwin
{
namespace obj
{

struct Triangle
{
public:
    glm::i32vec3 indices;
    std::array<glm::vec3, 3> vertices;
    std::array<glm::vec3, 3> uvs;
    std::array<glm::vec3, 3> normals;
    int material;
    int attributes;
};
	
struct MeshData
{
	std::vector<Triangle> triangles;
};

MeshData read(std::istream& stream);

} // namespace obj
} // namespace erwin