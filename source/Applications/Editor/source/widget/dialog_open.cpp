#include "widget/dialog_open.h"
#include "imgui.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include <kibble/logger/logger.h>

namespace editor
{
namespace dialog
{

void show_open_directory(const std::string& key, const char* title, const fs::path& default_path)
{
    ImGui::SetNextWindowSize({700, 400});
    igfd::ImGuiFileDialog::Instance()->OpenModal(key, title, 0, default_path.string(), "");
}

void show_open(const std::string& key, const char* title, const char* filter, const fs::path& default_path, const std::string& default_filename)
{
    ImGui::SetNextWindowSize({700, 400});
    igfd::ImGuiFileDialog::Instance()->OpenModal(key, title, filter, default_path.string(), default_filename);
}

void on_open(const std::string& key, std::function<void(const fs::path& filepath)> visit)
{
    if(igfd::ImGuiFileDialog::Instance()->FileDialog(key))
    {
        // action if OK
        if(igfd::ImGuiFileDialog::Instance()->IsOk == true)
        	visit(igfd::ImGuiFileDialog::Instance()->GetFilepathName());
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog(key);
    }
}

} // namespace dialog
} // namespace editor