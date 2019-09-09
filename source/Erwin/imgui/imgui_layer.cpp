#include "imgui_layer.h"
#include "../core/application.h"
#include "../debug/logger.h"

#include "imgui.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"

// TMP
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace erwin
{


ImGuiLayer::ImGuiLayer():
Layer("ImGuiLayer")
{
	
}

ImGuiLayer::~ImGuiLayer()
{
	
}

void ImGuiLayer::on_attach()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Configure ImGui IO
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	else
	{
    	style.WindowRounding = 5.0f;
	}

    ImVec4 neutral(0.7f, 0.3f, 0.05f, 1.0f);
    ImVec4 active(1.0f, 0.5f, 0.05f, 1.00f);
    ImVec4 hovered(0.6f, 0.6f, 0.6f, 1.00f);
    ImVec4 inactive(0.7f, 0.3f, 0.05f, 0.75f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.7f, 0.3f, 0.05f, 0.75f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    style.Colors[ImGuiCol_TitleBg] = neutral;
    style.Colors[ImGuiCol_TitleBgCollapsed] = inactive;
    style.Colors[ImGuiCol_TitleBgActive] = active;

    style.Colors[ImGuiCol_Header] = neutral;
    style.Colors[ImGuiCol_HeaderHovered] = hovered;
    style.Colors[ImGuiCol_HeaderActive] = active;

    style.Colors[ImGuiCol_ResizeGrip] = neutral;
    style.Colors[ImGuiCol_ResizeGripHovered] = hovered;
    style.Colors[ImGuiCol_ResizeGripActive] = active;

    style.Colors[ImGuiCol_Button] = neutral;
    style.Colors[ImGuiCol_ButtonHovered] = hovered;
    style.Colors[ImGuiCol_ButtonActive] = active;

    style.Colors[ImGuiCol_SliderGrab] = hovered;
    style.Colors[ImGuiCol_SliderGrabActive] = active;

    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.1f, 0.8f, 0.2f, 1.0f);

	// Setup Platform/Renderer bindings
	Application& app = Application::get_instance();
	GLFWwindow* window = static_cast<GLFWwindow*>(app.get_window().get_native());
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");
}

void ImGuiLayer::on_detach()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiLayer::begin()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiLayer::end()
{
	ImGuiIO& io = ImGui::GetIO();
	Application& app = Application::get_instance();
	io.DisplaySize = ImVec2((float)app.get_window().get_width(), 
						    (float)app.get_window().get_height());

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void ImGuiLayer::on_imgui_render()
{
	static bool show = true;
	ImGui::ShowDemoWindow(&show);
}

} // namespace erwin