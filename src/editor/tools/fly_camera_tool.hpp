#pragma once

#include "tools/tool.hpp"
#include "scene/frame_controller.hpp"

#include "erhe/application/commands/command.hpp"
#include "erhe/application/view.hpp"
#include "erhe/application/windows/imgui_window.hpp"
#include "erhe/components/components.hpp"
#include "erhe/toolkit/view.hpp" // keycode

#if defined(_WIN32) && 0
#   include "erhe/toolkit/space_mouse.hpp"
#endif

namespace erhe::scene
{
    class Camera;
}

namespace editor
{

class Fly_camera_tool;
class Pointer_context;
class Scene_root;
class Tools;
class Trs_tool;


#if defined(_WIN32) && 0
class Fly_camera_space_mouse_listener
    : public erhe::toolkit::Space_mouse_listener
{
public:
    explicit Fly_camera_space_mouse_listener(Fly_camera_tool& fly_camera_tool);
    ~Fly_camera_space_mouse_listener() noexcept;

    auto is_active     () -> bool                                 override;
    void set_active    (const bool value)                         override;
    void on_translation(const int tx, const int ty, const int tz) override;
    void on_rotation   (const int rx, const int ry, const int rz) override;
    void on_button     (const int id)                             override;

private:
    bool             m_is_active{false};
    Fly_camera_tool& m_fly_camera_tool;
};
#endif

class Fly_camera_turn_command
    : public erhe::application::Command
{
public:
    explicit Fly_camera_turn_command(Fly_camera_tool& fly_camera_tool)
        : Command          {"Fly_camera.turn_camera"}
        , m_fly_camera_tool{fly_camera_tool}
    {
    }

    auto try_call (erhe::application::Command_context& context) -> bool override;
    void try_ready(erhe::application::Command_context& context) override;

private:
    Fly_camera_tool& m_fly_camera_tool;
};

class Fly_camera_move_command
    : public erhe::application::Command
{
public:
    Fly_camera_move_command(
        Fly_camera_tool&                         fly_camera_tool,
        const Control                            control,
        const erhe::application::Controller_item item,
        const bool                               active
    )
        : Command          {"Fly_camera.move"}
        , m_fly_camera_tool{fly_camera_tool  }
        , m_control        {control          }
        , m_item           {item             }
        , m_active         {active           }
    {
    }

    auto try_call(erhe::application::Command_context& context) -> bool override;

private:
    Fly_camera_tool&                   m_fly_camera_tool;
    Control                            m_control;
    erhe::application::Controller_item m_item;
    bool                               m_active;
};

class Fly_camera_tool
    : public erhe::components::Component
    , public erhe::components::IUpdate_fixed_step
    , public erhe::components::IUpdate_once_per_frame
    , public Tool
    , public erhe::application::Imgui_window
{
public:
    static constexpr int              c_priority{5};
    static constexpr std::string_view c_label   {"Fly_camera_tool"};
    static constexpr std::string_view c_title   {"Fly Camera"};
    static constexpr uint32_t hash = compiletime_xxhash::xxh32(c_label.data(), c_label.size(), {});

    Fly_camera_tool ();
    ~Fly_camera_tool() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void declare_required_components() override;
    void initialize_component       () override;
    void post_initialize            () override;

    // Implements Tool
    [[nodiscard]] auto tool_priority() const -> int   override { return c_priority; }
    [[nodiscard]] auto description  () -> const char* override;

    // Implements Window
    void imgui() override;

    // Implements IUpdate_fixed_step
    void update_fixed_step(const erhe::components::Time_context& time_context) override;

    // Implements IUpdate_once_per_frame
    void update_once_per_frame(const erhe::components::Time_context& time_context) override;

    // Public API

    [[nodiscard]] auto get_camera() const -> erhe::scene::Camera*;
    void set_camera (erhe::scene::Camera* camera);
    void translation(const int tx, const int ty, const int tz);
    void rotation   (const int rx, const int ry, const int rz);

    // Commands
    auto try_ready() -> bool;
    auto try_move(
        const Control                            control,
        const erhe::application::Controller_item item,
        const bool                               active
    ) -> bool;
    void turn_relative(const double dx, const double dy);

private:
    void update_camera   ();
    auto can_use_keyboard() const -> bool;

    Fly_camera_turn_command           m_turn_command;
    Fly_camera_move_command           m_move_up_active_command;
    Fly_camera_move_command           m_move_up_inactive_command;
    Fly_camera_move_command           m_move_down_active_command;
    Fly_camera_move_command           m_move_down_inactive_command;
    Fly_camera_move_command           m_move_left_active_command;
    Fly_camera_move_command           m_move_left_inactive_command;
    Fly_camera_move_command           m_move_right_active_command;
    Fly_camera_move_command           m_move_right_inactive_command;
    Fly_camera_move_command           m_move_forward_active_command;
    Fly_camera_move_command           m_move_forward_inactive_command;
    Fly_camera_move_command           m_move_backward_active_command;
    Fly_camera_move_command           m_move_backward_inactive_command;
    std::shared_ptr<Frame_controller> m_camera_controller;
    float                             m_rotate_scale_x{1.0f};
    float                             m_rotate_scale_y{1.0f};

    // Component dependencies
    std::shared_ptr<Tools>           m_editor_tools;
    std::shared_ptr<Pointer_context> m_pointer_context;
    std::shared_ptr<Scene_root>      m_scene_root;
    std::shared_ptr<Trs_tool>        m_trs_tool;

    std::mutex                       m_mutex;

#if defined(_WIN32) && 0
    Fly_camera_space_mouse_listener       m_space_mouse_listener;
    erhe::toolkit::Space_mouse_controller m_space_mouse_controller;
#endif
    float                             m_sensitivity        {1.0f};
    bool                              m_use_viewport_camera{true};
};

} // namespace editor
