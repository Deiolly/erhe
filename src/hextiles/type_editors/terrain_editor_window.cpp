#include "type_editors/terrain_editor_window.hpp"
#include "type_editors/type_editor.hpp"
#include "menu_window.hpp"
#include "tiles.hpp"

#include "erhe/application/imgui_windows.hpp"

#include <imgui.h>

namespace hextiles
{

Terrain_editor_window::Terrain_editor_window()
    : erhe::components::Component{c_label}
    , Imgui_window               {c_title, c_label}
{
}

Terrain_editor_window::~Terrain_editor_window() noexcept
{
}

void Terrain_editor_window::declare_required_components()
{
    require<erhe::application::Imgui_windows>();
}

void Terrain_editor_window::initialize_component()
{
    Imgui_window::initialize(*m_components);
    get<erhe::application::Imgui_windows>()->register_imgui_window(this);
}

void Terrain_editor_window::imgui()
{
    constexpr ImVec2 button_size{110.0f, 0.0f};

    if (ImGui::Button("Back to Menu", button_size))
    {
        get<Menu_window>()->show_menu();
    }
    ImGui::SameLine();

    if (ImGui::Button("Load", button_size))
    {
        get<Tiles>()->load_terrain_defs();
    }
    ImGui::SameLine();

    if (ImGui::Button("Save", button_size))
    {
        get<Tiles>()->save_terrain_defs();
    }

    get<Type_editor>()->terrain_editor_imgui();
}

} // namespace hextiles
