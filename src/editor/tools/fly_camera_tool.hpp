#pragma once

#include "command.hpp"
#include "editor_view.hpp"
#include "scene/frame_controller.hpp"
#include "tools/tool.hpp"
#include "windows/imgui_window.hpp"

#include "erhe/components/component.hpp"
#include "erhe/toolkit/view.hpp" // keycode

#ifdef _WIN32
#   include "erhe/toolkit/space_mouse.hpp"
#endif

namespace erhe::scene
{

class Camera;
class ICamera;

}

namespace editor
{

class Editor_tools;
class Fly_camera_tool;
class Pointer_context;
class Scene_builder;
class Scene_root;
class Trs_tool;


#ifdef _WIN32
class Fly_camera_space_mouse_listener
    : public erhe::toolkit::Space_mouse_listener
{
public:
    explicit Fly_camera_space_mouse_listener(Fly_camera_tool& fly_camera_tool);
    ~Fly_camera_space_mouse_listener();

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
    : public Command
{
public:
    explicit Fly_camera_turn_command(Fly_camera_tool& fly_camera_tool)
        : Command          {"Fly_camera.turn_camera"}
        , m_fly_camera_tool{fly_camera_tool}
    {
    }

    auto try_call (Command_context& context) -> bool override;
    void try_ready(Command_context& context) override;

private:
    Fly_camera_tool& m_fly_camera_tool;
};

class Fly_camera_move_command
    : public Command
{
public:
    Fly_camera_move_command(
        Fly_camera_tool&      fly_camera_tool,
        const Control         control,
        const Controller_item item, 
        const bool            active
    )
        : Command          {"Fly_camera.move"}
        , m_fly_camera_tool{fly_camera_tool  }
        , m_control        {control          }
        , m_item           {item             }
        , m_active         {active           }
    {
    }

    auto try_call(Command_context& context) -> bool override;

private:
    Fly_camera_tool& m_fly_camera_tool;
    Control          m_control;
    Controller_item  m_item;
    bool             m_active;
};

class Fly_camera_tool
    : public erhe::components::Component
    , public erhe::components::IUpdate_fixed_step
    , public erhe::components::IUpdate_once_per_frame
    , public Tool
    , public Imgui_window
{
public:
    static constexpr int              c_priority   {5};
    static constexpr std::string_view c_name       {"Fly_camera_tool"};
    static constexpr std::string_view c_description{"Fly Camera"};
    static constexpr uint32_t hash = compiletime_xxhash::xxh32(c_name.data(), c_name.size(), {});

    Fly_camera_tool ();
    ~Fly_camera_tool() override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void connect             () override;
    void initialize_component() override;

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

    [[nodiscard]] auto get_camera() const -> erhe::scene::ICamera*;
    void set_camera (erhe::scene::ICamera* camera);
    void translation(const int tx, const int ty, const int tz);
    void rotation   (const int rx, const int ry, const int rz);

    // Commands
    auto try_ready() -> bool;
    void turn_relative(const double dx, const double dy);
    void move(
        const Control         control,
        const Controller_item item, 
        const bool            active
    );

private:
    auto can_use_keyboard() const -> bool;

    Fly_camera_turn_command               m_turn_command;
    Fly_camera_move_command               m_move_up_active_command;
    Fly_camera_move_command               m_move_up_inactive_command;
    Fly_camera_move_command               m_move_down_active_command;
    Fly_camera_move_command               m_move_down_inactive_command;
    Fly_camera_move_command               m_move_left_active_command;
    Fly_camera_move_command               m_move_left_inactive_command;
    Fly_camera_move_command               m_move_right_active_command;
    Fly_camera_move_command               m_move_right_inactive_command;
    Fly_camera_move_command               m_move_forward_active_command;
    Fly_camera_move_command               m_move_forward_inactive_command;
    Fly_camera_move_command               m_move_backward_active_command;
    Fly_camera_move_command               m_move_backward_inactive_command;
    Frame_controller                      m_camera_controller;

    Editor_tools*                         m_editor_tools    {nullptr};
    Pointer_context*                      m_pointer_context {nullptr};
    Scene_root*                           m_scene_root      {nullptr};
    Trs_tool*                             m_trs_tool        {nullptr};

    std::mutex                            m_mutex;

#ifdef _WIN32
    Fly_camera_space_mouse_listener       m_space_mouse_listener;
    erhe::toolkit::Space_mouse_controller m_space_mouse_controller;
#endif
    float                                 m_sensitivity{1.0f};
};

} // namespace editor
