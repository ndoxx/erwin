#include "imgui/dialog.h"

namespace ImGui
{

void OpenModal(const char* title) { ImGui::OpenPopup(title); }

DialogState CheckOkCancel(const char* title, const char* text)
{
    bool open = true;
    DialogState ret = DialogState::NONE;

    auto flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize({175, 75});
    if(ImGui::BeginPopupModal(title, &open, flags))
    {
        ImGui::TextWrapped("%s", text);
    	ImVec2 btn_span_size(ImGui::GetContentRegionAvailWidth()*0.5f, 0.f);
        if(ImGui::Button("Ok", btn_span_size))
        {
            ImGui::CloseCurrentPopup();
            ret = DialogState::OK;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", btn_span_size))
        {
            ImGui::CloseCurrentPopup();
            ret = DialogState::CANCEL;
        }
        ImGui::EndPopup();
    }

    return ret;
}

DialogState CheckYesNo(const char* title, const char* text)
{
    bool open = true;
    DialogState ret = DialogState::NONE;

    auto flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize({175, 75});
    if(ImGui::BeginPopupModal(title, &open, flags))
    {
        ImGui::TextWrapped("%s", text);
    	ImVec2 btn_span_size(ImGui::GetContentRegionAvailWidth()*0.5f, 0.f);
        if(ImGui::Button("Yes", btn_span_size))
        {
            ImGui::CloseCurrentPopup();
            ret = DialogState::YES;
        }
        ImGui::SameLine();
        if(ImGui::Button("No", btn_span_size))
        {
            ImGui::CloseCurrentPopup();
            ret = DialogState::NO;
        }
        ImGui::EndPopup();
    }

    return ret;
}

} // namespace ImGui