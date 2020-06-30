#include "entity/reflection.h"
#include "entity/component/light.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentDirectionalLight>(const ComponentDirectionalLight& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
	    file.add_node(cmp_node, "inclination", std::to_string(cmp.inclination).c_str());
	    file.add_node(cmp_node, "arg_periapsis", std::to_string(cmp.arg_periapsis).c_str());
	    file.add_node(cmp_node, "color", to_string(cmp.color).c_str());
	    file.add_node(cmp_node, "ambient_color", to_string(cmp.ambient_color).c_str());
	    file.add_node(cmp_node, "ambient_strength", to_string(cmp.ambient_strength).c_str());
	    file.add_node(cmp_node, "brightness", to_string(cmp.brightness).c_str());
    }
}