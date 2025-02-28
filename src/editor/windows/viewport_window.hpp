#pragma once

#include "windows/viewport_config.hpp"
#include "renderers/programs.hpp"

#include "erhe/application/commands/command.hpp"
#include "erhe/application/windows/imgui_window.hpp"
#include "erhe/components/components.hpp"
#include "erhe/scene/camera.hpp"
#include "erhe/scene/viewport.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace erhe::graphics
{
    class Framebuffer;
    class Texture;
    class Renderbuffer;
    class OpenGL_state_tracker;
}

namespace erhe::application
{
    class Configuration;
    class Imgui_windows;
    class View;
}
namespace editor
{

class Editor_rendering;
class Pointer_context;
class Post_processing;
class Render_context;
class Scene_root;
#if defined(ERHE_XR_LIBRARY_OPENXR)
class Headset_renderer;
#endif

class Viewport_window
    : public erhe::application::Imgui_window
    , public erhe::application::Mouse_input_sink
{
public:
    static constexpr std::string_view c_label{"Viewport_window"};
    static constexpr uint32_t hash {
        compiletime_xxhash::xxh32(
            c_label.data(),
            c_label.size(),
            {}
        )
    };

    Viewport_window(
        const std::string_view              name,
        const erhe::components::Components& components,
        erhe::scene::Camera*                camera
    );

    // Implements Imgui_window
    void imgui               () override;
    auto get_window_type_hash() const -> uint32_t override { return hash; }
    auto consumes_mouse_input() const -> bool override;
    void on_begin            () override;
    void on_end              () override;

    // Public API
    void try_hover       (const int px, const int py);
    void update          ();
    void bind_framebuffer();
    void resolve         ();
    void set_viewport    (const int x, const int y, const int width, const int height);
    void render(
        Editor_rendering&                     editor_rendering,
        erhe::graphics::OpenGL_state_tracker& pipeline_state_tracker
    );

    [[nodiscard]] auto to_scene_content    (const glm::vec2 position_in_root) const -> glm::vec2;
    [[nodiscard]] auto project_to_viewport (const glm::dvec3 position_in_world) const -> glm::dvec3;
    [[nodiscard]] auto unproject_to_world  (const glm::dvec3 position_in_window) const -> nonstd::optional<glm::dvec3>;
    [[nodiscard]] auto pointer_context     () const -> const std::shared_ptr<Pointer_context>&;
    [[nodiscard]] auto is_framebuffer_ready() const -> bool;
    [[nodiscard]] auto content_region_size () const -> glm::ivec2;
    [[nodiscard]] auto is_hovered          () const -> bool;
    [[nodiscard]] auto viewport            () const -> const erhe::scene::Viewport&;
    [[nodiscard]] auto camera              () const -> erhe::scene::Camera*;

private:
    [[nodiscard]] auto should_render             () const -> bool;
    [[nodiscard]] auto should_post_process       () const -> bool;
    [[nodiscard]] auto get_override_shader_stages() const -> erhe::graphics::Shader_stages*;

    void update_framebuffer();

    void clear(erhe::graphics::OpenGL_state_tracker& pipeline_state_tracker) const;

    static int s_serial;

    std::string                                   m_name;

    // Component dependencies
    std::shared_ptr<erhe::application::Configuration> m_configuration;
    std::shared_ptr<Pointer_context>              m_pointer_context;
    std::shared_ptr<Post_processing>              m_post_processing;
    std::shared_ptr<Programs>                     m_programs;
    std::shared_ptr<Scene_root>                   m_scene_root;
    std::shared_ptr<Viewport_config>              m_viewport_config;

    erhe::scene::Viewport                         m_viewport              {0, 0, 0, 0, true};
    erhe::scene::Camera*                          m_camera                {nullptr};
    Shader_stages_variant                         m_shader_stages_variant {Shader_stages_variant::standard};
    bool                                          m_enable_post_processing{true};
    erhe::scene::Camera_projection_transforms     m_camera_projection_transforms;
    bool                                          m_is_hovered            {false};
    bool                                          m_can_present           {false};
    glm::ivec2                                    m_content_region_min    {0.0f, 0.0f};
    glm::ivec2                                    m_content_region_max    {0.0f, 0.0f};
    glm::ivec2                                    m_content_region_size   {0.0f, 0.0f};
    std::unique_ptr<erhe::graphics::Texture>      m_color_texture_multisample;
    std::shared_ptr<erhe::graphics::Texture>      m_color_texture_resolved;
    std::shared_ptr<erhe::graphics::Texture>      m_color_texture_resolved_for_present;
    std::unique_ptr<erhe::graphics::Renderbuffer> m_depth_stencil_multisample_renderbuffer;
    std::unique_ptr<erhe::graphics::Renderbuffer> m_depth_stencil_resolved_renderbuffer;
    std::unique_ptr<erhe::graphics::Framebuffer>  m_framebuffer_multisample;
    std::unique_ptr<erhe::graphics::Framebuffer>  m_framebuffer_resolved;
};

inline auto is_viewport_window(erhe::application::Imgui_window* window) -> bool
{
    return
        (window != nullptr) &&
        (window->get_window_type_hash() == Viewport_window::hash);
}

inline auto as_viewport_window(erhe::application::Imgui_window* window) -> Viewport_window*
{
    return
        (window != nullptr) &&
        (window->get_window_type_hash() == Viewport_window::hash)
            ? reinterpret_cast<Viewport_window*>(window)
            : nullptr;
}

class Viewport_windows;

class Open_new_viewport_window_command
    : public erhe::application::Command
{
public:
    explicit Open_new_viewport_window_command(Viewport_windows& viewport_windows)
        : Command           {"Viewport_windows.open_new_viewport_window"}
        , m_viewport_windows{viewport_windows}
    {
    }

    auto try_call(erhe::application::Command_context& context) -> bool override;

private:
    Viewport_windows& m_viewport_windows;
};

class Viewport_windows
    : public erhe::components::Component
{
public:
    static constexpr std::string_view c_label{"Viewport_windows"};
    static constexpr std::string_view c_title{"View"};
    static constexpr uint32_t hash{
        compiletime_xxhash::xxh32(
            c_label.data(),
            c_label.size(),
            {}
        )
    };

    Viewport_windows ();
    ~Viewport_windows() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void declare_required_components() override;
    void initialize_component       () override;
    void post_initialize            () override;

    // Public API
    auto create_window(
        const std::string_view name,
        erhe::scene::Camera*   camera
    ) -> Viewport_window*;

    auto open_new_viewport_window() -> bool;

    void update_viewport_windows();
    void render();

private:
    // Component dependencies
    std::shared_ptr<erhe::application::Configuration>     m_configuration;
    std::shared_ptr<Editor_rendering>                     m_editor_rendering;
    std::shared_ptr<erhe::application::Imgui_windows>     m_imgui_windows;
    std::shared_ptr<erhe::application::View>              m_editor_view;
    std::shared_ptr<erhe::graphics::OpenGL_state_tracker> m_pipeline_state_tracker;
    std::shared_ptr<Pointer_context>                      m_pointer_context;
    std::shared_ptr<Scene_root>                           m_scene_root;
    std::shared_ptr<Viewport_config>                      m_viewport_config;
#if defined(ERHE_XR_LIBRARY_OPENXR)
    std::shared_ptr<Headset_renderer>                     m_headset_renderer;
#endif

    // Commands
    Open_new_viewport_window_command              m_open_new_viewport_window_command;

    std::vector<std::shared_ptr<Viewport_window>> m_windows;
    Viewport_window*                              m_hover_window{nullptr};
};

} // namespace editor
