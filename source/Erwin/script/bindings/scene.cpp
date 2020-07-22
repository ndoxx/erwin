#include "script/bindings/scene.h"
#include "script/bindings/scene_proxy.h"
#include <chaiscript/chaiscript.hpp>
#include <string>

namespace erwin
{
namespace script
{

// * Components bindings
void bind_transform(std::shared_ptr<chaiscript::Module> module)
{
    ADD_CLASS(Transform3D);
    ADD_FUN(Transform3D, position);
    ADD_FUN(Transform3D, rotation);
    ADD_FUN(Transform3D, uniform_scale);

    ADD_FUN(Transform3D, init);
}

// * Scene bindings
std::shared_ptr<chaiscript::Module> make_scene_bindings()
{
    using namespace chaiscript;
    auto module = std::make_shared<chaiscript::Module>();

    bind_transform(module);

    ADD_CLASS(SceneProxy);
    ADD_FUN(SceneProxy, get_transform);
    ADD_FUN(SceneProxy, force_update);

    return module;
}

} // namespace script
} // namespace erwin