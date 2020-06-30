#include "entity/component/camera.h"
#include "entity/reflection.h"

namespace erwin
{

template <>
std::string to_string<ComponentCamera3D::Frustum3D>(const ComponentCamera3D::Frustum3D& f)
{
    return "(" + std::to_string(f.left) + "," + std::to_string(f.right) + "," + std::to_string(f.bottom) + "," +
           std::to_string(f.top) + "," + std::to_string(f.near) + "," + std::to_string(f.far) + ")";
}

template <>
void serialize_xml<ComponentCamera3D>(const ComponentCamera3D& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "frustum", to_string(cmp.frustum).c_str());
}
} // namespace erwin