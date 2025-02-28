#include "windows/layers_window.hpp"
#include "editor_log.hpp"
#include "graphics/icon_set.hpp"
#include "tools/selection_tool.hpp"
#include "scene/node_physics.hpp"
#include "scene/scene_root.hpp"

#include "erhe/application/imgui_windows.hpp"

#include "erhe/graphics/texture.hpp"
#include "erhe/scene/scene.hpp"
#include "erhe/scene/light.hpp"
#include "erhe/scene/mesh.hpp"
#include "erhe/scene/node.hpp"
#include "erhe/toolkit/profile.hpp"

#include <gsl/gsl>

#if defined(ERHE_GUI_LIBRARY_IMGUI)
#   include <imgui.h>
#endif

namespace editor
{

using Light_type = erhe::scene::Light_type;

Layers_window::Layers_window()
    : erhe::components::Component    {c_label}
    , erhe::application::Imgui_window{c_title, c_label}
{
}

Layers_window::~Layers_window() noexcept
{
}

void Layers_window::declare_required_components()
{
    require<erhe::application::Imgui_windows>();
}

void Layers_window::initialize_component()
{
    get<erhe::application::Imgui_windows>()->register_imgui_window(this);
}

void Layers_window::post_initialize()
{
    m_scene_root     = get<Scene_root>();
    m_selection_tool = get<Selection_tool>();
    m_icon_set       = get<Icon_set>();
}

void Layers_window::imgui()
{
#if defined(ERHE_GUI_LIBRARY_IMGUI)
    ERHE_PROFILE_FUNCTION

    const ImGuiTreeNodeFlags parent_flags{
        ImGuiTreeNodeFlags_OpenOnArrow       |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanFullWidth
    };
    const ImGuiTreeNodeFlags leaf_flags{
        ImGuiTreeNodeFlags_SpanFullWidth    |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_Leaf
    };

    const auto& scene = m_scene_root->scene();
    for (const auto& layer : scene.mesh_layers)
    {
        if (ImGui::TreeNodeEx(layer->name.c_str(), parent_flags))
        {
            const auto& meshes = layer->meshes;
            for (const auto& mesh : meshes)
            {
                m_icon_set->icon(*mesh.get());
                ImGui::TreeNodeEx(
                    mesh->name().c_str(),
                    leaf_flags |
                    (
                        mesh->is_selected()
                            ? ImGuiTreeNodeFlags_Selected
                            : ImGuiTreeNodeFlags_None
                    )
                );
                if (ImGui::IsItemClicked())
                {
                    m_node_clicked = mesh->shared_from_this();
                }
            }
            ImGui::TreePop();
        }
    }

    //const ImGuiIO& io = ImGui::GetIO();
    if (m_node_clicked && m_selection_tool)
    {
        if (false) // TODO shift or maybe ctrl?
        {
            if (m_node_clicked->is_selected())
            {
                m_selection_tool->remove_from_selection(m_node_clicked);
            }
            else
            {
                m_selection_tool->add_to_selection(m_node_clicked);
            }
        }
        else
        {
            bool was_selected = m_node_clicked->is_selected();
            m_selection_tool->clear_selection();
            if (!was_selected)
            {
                m_selection_tool->add_to_selection(m_node_clicked);
            }
        }
    }
    m_node_clicked.reset();
#endif
}

} // namespace editor
