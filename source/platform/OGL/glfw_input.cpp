#include "glfw_input.h"
#include "glfw_keymap.h"
#include "glfw_window.h"
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
	return { float(xpos), float(ypos) };
}

void GLFWInput::set_mouse_position_impl(float x, float y) const
{
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::get_instance().get_window().get_native());
	glfwSetCursorPos(window, double(x), double(y));
}

void GLFWInput::center_mouse_position_impl() const
{
	const Window& app_win = Application::get_instance().get_window();
	GLFWwindow* window = static_cast<GLFWwindow*>(app_win.get_native());
	glfwSetCursorPos(window, 0.5*double(app_win.get_width()), 0.5*double(app_win.get_height()));
}

void GLFWInput::show_cursor_impl(bool value) const
{
	GLFWwindow* window = static_cast<GLFWwindow*>(Application::get_instance().get_window().get_native());
	glfwSetInputMode(window, GLFW_CURSOR, value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}


} // namespace erwin