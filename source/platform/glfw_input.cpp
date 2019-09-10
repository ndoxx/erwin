#include "glfw_input.h"
#include "glfw_keymap.h"
#include "core/application.h"
#include "GLFW/glfw3.h"

namespace erwin
{

using namespace keymap;

Input* Input::INSTANCE_ = new GLFWInput();

bool GLFWInput::is_key_pressed_impl(WKEY keycode) const
{
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::get_instance().get_window().get_native());
	auto state = glfwGetKey(window, WKEY_to_GLFW_KEY(keycode));
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool GLFWInput::is_mouse_button_pressed_impl(WMOUSE button) const
{
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::get_instance().get_window().get_native());
	auto state = glfwGetMouseButton(window, WMOUSE_to_GLFW_MB(button));
	return state == GLFW_PRESS;
}

std::pair<float,float> GLFWInput::get_mouse_position_impl() const
{
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::get_instance().get_window().get_native());
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}


} // namespace erwin