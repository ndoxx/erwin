#include "entity/component/serial/camera.h"
#include "level/scene.h"

namespace erwin
{

template <> std::string to_string<ComponentCamera3D::Frustum3D>(const ComponentCamera3D::Frustum3D& f)
{
    return "(" + std::to_string(f.left) + "," + std::to_string(f.right) + "," + std::to_string(f.bottom) + "," +
           std::to_string(f.top) + "," + std::to_string(f.near) + "," + std::to_string(f.far) + ")";
}

template <> bool str_val<ComponentCamera3D::Frustum3D>(const char* value, ComponentCamera3D::Frustum3D& result)
{
    return sscanf(value, "(%f,%f,%f,%f,%f,%f)", &result.left, &result.right, &result.bottom, &result.top, &result.near,
                  &result.far) > 0;
}

template <>
void serialize_xml<ComponentCamera3D>(const ComponentCamera3D& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "frustum", to_string(cmp.frustum).c_str());
}

template <>
void deserialize_xml<ComponentCamera3D>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
    ComponentCamera3D::Frustum3D frustum;
    xml::parse_node(cmp_node, "frustum", frustum);

    auto& cmp_camera = scene.add_component<ComponentCamera3D>(e);
    cmp_camera.set_projection(frustum);
}
} // namespace erwin