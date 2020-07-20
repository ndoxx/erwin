#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
#include <chaiscript/chaiscript.hpp>
#include <string>

#include "script/bindings/glm.h"

namespace erwin
{
namespace script
{
std::shared_ptr<chaiscript::Module> make_glm_bindings()
{
	using namespace chaiscript;
	using namespace glm;
    auto module = std::make_shared<chaiscript::Module>();

	// Scalar
	module->add(fun(static_cast<float (*)(float, float)>(max)), "max");
	module->add(fun(static_cast<float (*)(float, float)>(min)), "min");
	module->add(fun(static_cast<float (*)(float, float, float)>(clamp)), "clamp");
	module->add(fun(static_cast<float (*)(float, float, float)>(mix)), "mix");
	module->add(fun(static_cast<float (*)(float, float, float)>(mix)), "lerp");
	module->add(fun(static_cast<float (*)(float)>(radians)), "radians");
	module->add(fun(static_cast<float (*)(float)>(degrees)), "degrees");
	module->add(fun(static_cast<float (*)(float)>(sin)), "sin");
	module->add(fun(static_cast<float (*)(float)>(cos)), "cos");
	module->add(fun(static_cast<float (*)(float)>(tan)), "tan");
	module->add(fun(static_cast<float (*)(float)>(asin)), "asin");
	module->add(fun(static_cast<float (*)(float)>(acos)), "acos");
	module->add(fun(static_cast<float (*)(float)>(atan)), "atan");
	module->add(fun(static_cast<float (*)(float)>(abs)), "abs");
	module->add(fun(static_cast<float (*)(float)>(ceil)), "ceil");
	module->add(fun(static_cast<float (*)(float)>(floor)), "floor");
	module->add(fun(static_cast<float (*)(float)>(fract)), "fract");
	module->add(fun(&glm::pi<float>), "pi");
	module->add(fun(&glm::half_pi<float>), "half_pi");
	module->add(fun(&glm::two_pi<float>), "two_pi");
	module->add(fun(&glm::quarter_pi<float>), "quarter_pi");
	module->add(fun(&glm::three_over_two_pi<float>), "three_over_two_pi");

	// 2D vector
	module->add(user_type<vec2>(), "vec2");
	module->add(constructor<vec2(float)>(), "vec2");
	module->add(constructor<vec2(float, float)>(), "vec2");
	module->add(constructor<vec2(const vec2&)>(), "vec2");
	module->add(constructor<vec2()>(), "vec2");
	module->add(fun(&vec2::x), "x");
	module->add(fun(&vec2::y), "y");
	module->add(fun([](vec2& lhs, const vec2& rhs){ lhs = rhs; return lhs; }), "=");
	module->add(fun([](const vec2& rhs){ return -rhs; }), "-");
	module->add(fun([](const vec2& lhs, const vec2& rhs){ return lhs - rhs; }), "-");
	module->add(fun([](const vec2& lhs, const vec2& rhs){ return lhs + rhs; }), "+");
	module->add(fun([](const vec2& lhs, const vec2& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const vec2& lhs, const float rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const float lhs, const vec2& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](vec2& lhs, const vec2& rhs){ lhs += rhs; return lhs; }), "+=");
	module->add(fun([](vec2& lhs, const vec2& rhs){ lhs -= rhs; return lhs; }), "-=");
	module->add(fun([](vec2& lhs, const float rhs){ lhs *= rhs; return lhs; }), "*=");
	module->add(fun(static_cast<vec2 (*)(const vec2&)>(normalize)), "normalize");
	module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&)>(reflect)), "reflect");
	module->add(fun(static_cast<float (*)(const vec2&)>(length)), "length");
	module->add(fun(static_cast<float (*)(const vec2&)>(length)), "norm");
	module->add(fun(static_cast<float (*)(const vec2&)>(length2)), "length2");
	module->add(fun(static_cast<float (*)(const vec2&)>(length2)), "norm2");
	module->add(fun(static_cast<float (*)(const vec2&, const vec2&)>(dot)), "dot");
	module->add(fun(static_cast<float (*)(const vec2&, const vec2&)>(distance)), "distance");
	module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&, float)>(refract)), "refract");
	module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&, float)>(mix)), "mix");
	module->add(fun([](const vec2& a, const vec2& b) { return all(equal(a, b)); }), "==");
	module->add(fun([](const vec2& v) -> std::string { return glm::to_string(v); }), "to_string");

	// 3D vector
	module->add(user_type<vec3>(), "vec3");
	module->add(constructor<vec3(float)>(), "vec3");
	module->add(constructor<vec3(float, float, float)>(), "vec3");
	module->add(constructor<vec3(const vec3&)>(), "vec3");
	module->add(constructor<vec3(const vec2&, float)>(), "vec3");
	module->add(constructor<vec3()>(), "vec3");
	module->add(fun(&vec3::x), "x");
	module->add(fun(&vec3::y), "y");
	module->add(fun(&vec3::z), "z");
	module->add(fun([](vec3& lhs, const vec3& rhs){ lhs = rhs; return lhs; }), "=");
	module->add(fun([](const vec3& rhs){ return -rhs; }), "-");
	module->add(fun([](const vec3& lhs, const vec3& rhs){ return lhs - rhs; }), "-");
	module->add(fun([](const vec3& lhs, const vec3& rhs){ return lhs + rhs; }), "+");
	module->add(fun([](const vec3& lhs, const vec3& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const vec3& lhs, const float rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const float lhs, const vec3& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](vec3& lhs, const vec3& rhs){ lhs += rhs; return lhs; }), "+=");
	module->add(fun([](vec3& lhs, const vec3& rhs){ lhs -= rhs; return lhs; }), "-=");
	module->add(fun([](vec3& lhs, const float rhs){ lhs *= rhs; return lhs; }), "*=");
	module->add(fun(static_cast<vec3 (*)(const vec3&)>(normalize)), "normalize");
	module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(cross)), "cross");
	module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(reflect)), "reflect");
	module->add(fun(static_cast<float (*)(const vec3&)>(length)), "length");
	module->add(fun(static_cast<float (*)(const vec3&)>(length)), "norm");
	module->add(fun(static_cast<float (*)(const vec3&)>(length2)), "length2");
	module->add(fun(static_cast<float (*)(const vec3&)>(length2)), "norm2");
	module->add(fun(static_cast<float (*)(const vec3&, const vec3&)>(dot)), "dot");
	module->add(fun(static_cast<float (*)(const vec3&, const vec3&)>(distance)), "distance");
	module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&, float)>(refract)), "refract");
	module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&, float)>(mix)), "mix");
	module->add(fun([](const vec3& a, const vec3& b) { return all(equal(a, b)); }), "==");
	module->add(fun([](const vec3& v) -> std::string { return glm::to_string(v); }), "to_string");

	// 4D vector
	module->add(user_type<vec4>(), "vec4");
	module->add(constructor<vec4(float)>(), "vec4");
	module->add(constructor<vec4(float, float, float, float)>(), "vec4");
	module->add(constructor<vec4(const vec4&)>(), "vec4");
	module->add(constructor<vec4(const vec3&, float)>(), "vec4");
	module->add(constructor<vec4(const glm::vec2&, float, float)>(), "vec4");
	module->add(constructor<vec4()>(), "vec4");
	module->add(fun(&vec4::x), "x");
	module->add(fun(&vec4::y), "y");
	module->add(fun(&vec4::z), "z");
	module->add(fun(&vec4::w), "w");
	module->add(fun([](vec4& lhs, const vec4& rhs){ lhs = rhs; return lhs; }), "=");
	module->add(fun([](const vec4& rhs){ return -rhs; }), "-");
	module->add(fun([](const vec4& lhs, const vec4& rhs){ return lhs - rhs; }), "-");
	module->add(fun([](const vec4& lhs, const vec4& rhs){ return lhs + rhs; }), "+");
	module->add(fun([](const vec4& lhs, const vec4& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const vec4& lhs, const float rhs){ return lhs * rhs; }), "*");
	module->add(fun([](const float lhs, const vec4& rhs){ return lhs * rhs; }), "*");
	module->add(fun([](vec4& lhs, const vec4& rhs){ lhs += rhs; return lhs; }), "+=");
	module->add(fun([](vec4& lhs, const vec4& rhs){ lhs -= rhs; return lhs; }), "-=");
	module->add(fun([](vec4& lhs, const float rhs){ lhs *= rhs; return lhs; }), "*=");
	module->add(fun(static_cast<vec4 (*)(const vec4&)>(normalize)), "normalize");
	module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&)>(reflect)), "reflect");
	module->add(fun(static_cast<float (*)(const vec4&)>(length)), "length");
	module->add(fun(static_cast<float (*)(const vec4&)>(length)), "norm");
	module->add(fun(static_cast<float (*)(const vec4&)>(length2)), "length2");
	module->add(fun(static_cast<float (*)(const vec4&)>(length2)), "norm2");
	module->add(fun(static_cast<float (*)(const vec4&, const vec4&)>(dot)), "dot");
	module->add(fun(static_cast<float (*)(const vec4&, const vec4&)>(distance)), "distance");
	module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&, float)>(refract)), "refract");
	module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&, float)>(mix)), "mix");
	module->add(fun([](const vec4& a, const vec4& b) { return all(equal(a, b)); }), "==");
	module->add(fun([](const vec4& v) -> std::string { return glm::to_string(v); }), "to_string");

	// Quaternion
	module->add(user_type<quat>(), "quat");
	module->add(constructor<quat()>(), "quat");
	module->add(constructor<quat(const vec3&)>(), "quat");
	module->add(fun(&quat::w), "w");
	module->add(fun(&quat::x), "x");
	module->add(fun(&quat::y), "y");
	module->add(fun(&quat::z), "z");
	module->add(fun([](const quat& lhs, const quat& rhs) -> quat { return lhs * rhs; }), "*");
	module->add(fun([](const quat& lhs, const quat& rhs) -> quat { return lhs + rhs; }), "+");
	module->add(fun([](const quat& lhs, const quat& rhs) -> quat { return lhs - rhs; }), "-");
	module->add(fun([](const quat& lhs, const vec3& rhs) -> vec3 { return lhs * rhs; }), "*");
	module->add(fun(static_cast<quat (*)(const float&, const vec3&)>(angleAxis)), "angleAxis");
	module->add(fun(static_cast<quat (*)(const quat&)>(conjugate)), "conjugate");
	module->add(fun(static_cast<quat (*)(const quat&)>(inverse)), "inverse");
	module->add(fun(static_cast<quat (*)(const quat&)>(normalize)), "normalize");
	module->add(fun(static_cast<quat (*)(const quat&, const float&, const vec3&)>(rotate)), "rotate");
	module->add(fun(static_cast<quat (*)(const quat&, const quat&, float)>(mix)), "mix");
	module->add(fun(static_cast<quat (*)(const quat&, const quat&, float)>(mix)), "slerp");
	module->add(fun([](const quat& a, const quat& b) { return all(equal(a, b)); }), "==");
	module->add(fun([](const quat& q) -> std::string { return glm::to_string(q); }), "to_string");

	return module;
}

} // namespace script
} // namespace erwin
