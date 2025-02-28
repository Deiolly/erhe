﻿#include "erhe/application/imgui_viewport.hpp"
#include "erhe/toolkit/window.hpp"

#include <algorithm>

namespace erhe::application {

namespace {

auto from_erhe(const erhe::toolkit::Keycode keycode) -> ImGuiKey
{
    switch (keycode)
    {
        case erhe::toolkit::Key_tab          : return ImGuiKey_Tab           ;
        case erhe::toolkit::Key_left         : return ImGuiKey_LeftArrow     ;
        case erhe::toolkit::Key_right        : return ImGuiKey_RightArrow    ;
        case erhe::toolkit::Key_up           : return ImGuiKey_UpArrow       ;
        case erhe::toolkit::Key_down         : return ImGuiKey_DownArrow     ;
        case erhe::toolkit::Key_page_up      : return ImGuiKey_PageUp        ;
        case erhe::toolkit::Key_page_down    : return ImGuiKey_PageDown      ;
        case erhe::toolkit::Key_home         : return ImGuiKey_Home          ;
        case erhe::toolkit::Key_end          : return ImGuiKey_End           ;
        case erhe::toolkit::Key_insert       : return ImGuiKey_Insert        ;
        case erhe::toolkit::Key_delete       : return ImGuiKey_Delete        ;
        case erhe::toolkit::Key_backspace    : return ImGuiKey_Backspace     ;
        case erhe::toolkit::Key_space        : return ImGuiKey_Space         ;
        case erhe::toolkit::Key_enter        : return ImGuiKey_Enter         ;
        case erhe::toolkit::Key_escape       : return ImGuiKey_Escape        ;
        case erhe::toolkit::Key_left_control : return ImGuiKey_LeftCtrl      ;
        case erhe::toolkit::Key_left_shift   : return ImGuiKey_LeftShift     ;
        case erhe::toolkit::Key_left_alt     : return ImGuiKey_LeftAlt       ;
        case erhe::toolkit::Key_left_super   : return ImGuiKey_LeftSuper     ;
        case erhe::toolkit::Key_right_control: return ImGuiKey_RightCtrl     ;
        case erhe::toolkit::Key_right_shift  : return ImGuiKey_RightShift    ;
        case erhe::toolkit::Key_right_alt    : return ImGuiKey_RightAlt      ;
        case erhe::toolkit::Key_right_super  : return ImGuiKey_RightSuper    ;
        case erhe::toolkit::Key_menu         : return ImGuiKey_Menu          ;
        case erhe::toolkit::Key_0            : return ImGuiKey_0             ;
        case erhe::toolkit::Key_1            : return ImGuiKey_1             ;
        case erhe::toolkit::Key_2            : return ImGuiKey_2             ;
        case erhe::toolkit::Key_3            : return ImGuiKey_3             ;
        case erhe::toolkit::Key_4            : return ImGuiKey_4             ;
        case erhe::toolkit::Key_5            : return ImGuiKey_5             ;
        case erhe::toolkit::Key_6            : return ImGuiKey_6             ;
        case erhe::toolkit::Key_7            : return ImGuiKey_7             ;
        case erhe::toolkit::Key_8            : return ImGuiKey_8             ;
        case erhe::toolkit::Key_9            : return ImGuiKey_9             ;
        case erhe::toolkit::Key_a            : return ImGuiKey_A             ;
        case erhe::toolkit::Key_b            : return ImGuiKey_B             ;
        case erhe::toolkit::Key_c            : return ImGuiKey_C             ;
        case erhe::toolkit::Key_d            : return ImGuiKey_D             ;
        case erhe::toolkit::Key_e            : return ImGuiKey_E             ;
        case erhe::toolkit::Key_f            : return ImGuiKey_F             ;
        case erhe::toolkit::Key_g            : return ImGuiKey_G             ;
        case erhe::toolkit::Key_h            : return ImGuiKey_H             ;
        case erhe::toolkit::Key_i            : return ImGuiKey_I             ;
        case erhe::toolkit::Key_j            : return ImGuiKey_J             ;
        case erhe::toolkit::Key_k            : return ImGuiKey_K             ;
        case erhe::toolkit::Key_l            : return ImGuiKey_L             ;
        case erhe::toolkit::Key_m            : return ImGuiKey_M             ;
        case erhe::toolkit::Key_n            : return ImGuiKey_N             ;
        case erhe::toolkit::Key_o            : return ImGuiKey_O             ;
        case erhe::toolkit::Key_p            : return ImGuiKey_P             ;
        case erhe::toolkit::Key_q            : return ImGuiKey_Q             ;
        case erhe::toolkit::Key_r            : return ImGuiKey_R             ;
        case erhe::toolkit::Key_s            : return ImGuiKey_S             ;
        case erhe::toolkit::Key_t            : return ImGuiKey_T             ;
        case erhe::toolkit::Key_u            : return ImGuiKey_U             ;
        case erhe::toolkit::Key_v            : return ImGuiKey_V             ;
        case erhe::toolkit::Key_w            : return ImGuiKey_W             ;
        case erhe::toolkit::Key_x            : return ImGuiKey_X             ;
        case erhe::toolkit::Key_y            : return ImGuiKey_Y             ;
        case erhe::toolkit::Key_z            : return ImGuiKey_Z             ;
        case erhe::toolkit::Key_f1           : return ImGuiKey_F1            ;
        case erhe::toolkit::Key_f2           : return ImGuiKey_F2            ;
        case erhe::toolkit::Key_f3           : return ImGuiKey_F3            ;
        case erhe::toolkit::Key_f4           : return ImGuiKey_F4            ;
        case erhe::toolkit::Key_f5           : return ImGuiKey_F5            ;
        case erhe::toolkit::Key_f6           : return ImGuiKey_F6            ;
        case erhe::toolkit::Key_f7           : return ImGuiKey_F7            ;
        case erhe::toolkit::Key_f8           : return ImGuiKey_F8            ;
        case erhe::toolkit::Key_f9           : return ImGuiKey_F9            ;
        case erhe::toolkit::Key_f10          : return ImGuiKey_F10           ;
        case erhe::toolkit::Key_f11          : return ImGuiKey_F11           ;
        case erhe::toolkit::Key_f12          : return ImGuiKey_F12           ;
        case erhe::toolkit::Key_apostrophe   : return ImGuiKey_Apostrophe    ;
        case erhe::toolkit::Key_comma        : return ImGuiKey_Comma         ;
        case erhe::toolkit::Key_minus        : return ImGuiKey_Minus         ; // -
        case erhe::toolkit::Key_period       : return ImGuiKey_Period        ; // .
        case erhe::toolkit::Key_slash        : return ImGuiKey_Slash         ; // /
        case erhe::toolkit::Key_semicolon    : return ImGuiKey_Semicolon     ; // ;
        case erhe::toolkit::Key_equal        : return ImGuiKey_Equal         ; // =
        case erhe::toolkit::Key_left_bracket : return ImGuiKey_LeftBracket   ; // [
        case erhe::toolkit::Key_backslash    : return ImGuiKey_Backslash     ; // \ (this text inhibit multiline comment caused by backslash)
        case erhe::toolkit::Key_right_bracket: return ImGuiKey_RightBracket  ; // ]
        case erhe::toolkit::Key_grave_accent : return ImGuiKey_GraveAccent   ; // `
        case erhe::toolkit::Key_caps_lock    : return ImGuiKey_CapsLock      ;
        case erhe::toolkit::Key_scroll_lock  : return ImGuiKey_ScrollLock    ;
        case erhe::toolkit::Key_num_lock     : return ImGuiKey_NumLock       ;
        case erhe::toolkit::Key_print_screen : return ImGuiKey_PrintScreen   ;
        case erhe::toolkit::Key_pause        : return ImGuiKey_Pause         ;
        case erhe::toolkit::Key_kp_0         : return ImGuiKey_Keypad0       ;
        case erhe::toolkit::Key_kp_1         : return ImGuiKey_Keypad1       ;
        case erhe::toolkit::Key_kp_2         : return ImGuiKey_Keypad2       ;
        case erhe::toolkit::Key_kp_3         : return ImGuiKey_Keypad3       ;
        case erhe::toolkit::Key_kp_4         : return ImGuiKey_Keypad4       ;
        case erhe::toolkit::Key_kp_5         : return ImGuiKey_Keypad5       ;
        case erhe::toolkit::Key_kp_6         : return ImGuiKey_Keypad6       ;
        case erhe::toolkit::Key_kp_7         : return ImGuiKey_Keypad7       ;
        case erhe::toolkit::Key_kp_8         : return ImGuiKey_Keypad8       ;
        case erhe::toolkit::Key_kp_9         : return ImGuiKey_Keypad9       ;
        case erhe::toolkit::Key_kp_decimal   : return ImGuiKey_KeypadDecimal ;
        case erhe::toolkit::Key_kp_divide    : return ImGuiKey_KeypadDivide  ;
        case erhe::toolkit::Key_kp_multiply  : return ImGuiKey_KeypadMultiply;
        case erhe::toolkit::Key_kp_subtract  : return ImGuiKey_KeypadSubtract;
        case erhe::toolkit::Key_kp_add       : return ImGuiKey_KeypadAdd     ;
        case erhe::toolkit::Key_kp_enter     : return ImGuiKey_KeypadEnter   ;
        case erhe::toolkit::Key_kp_equal     : return ImGuiKey_KeypadEqual   ;
        default                              : return ImGuiKey_None          ;
    }
}

void update_key_modifiers(ImGuiIO& io, uint32_t modifier_mask)
{
    io.AddKeyEvent(ImGuiKey_ModCtrl,  (modifier_mask & erhe::toolkit::Key_modifier_bit_ctrl ) != 0);
    io.AddKeyEvent(ImGuiKey_ModShift, (modifier_mask & erhe::toolkit::Key_modifier_bit_shift) != 0);
    io.AddKeyEvent(ImGuiKey_ModAlt,   (modifier_mask & erhe::toolkit::Key_modifier_bit_menu ) != 0);
    io.AddKeyEvent(ImGuiKey_ModSuper, (modifier_mask & erhe::toolkit::Key_modifier_bit_super) != 0);
}

}

Imgui_viewport::Imgui_viewport()
{
    std::fill(
        std::begin(m_mouse_just_pressed),
        std::end(m_mouse_just_pressed),
        false
    );
    IMGUI_CHECKVERSION();
}

Imgui_viewport::~Imgui_viewport()
{
    ImGui::DestroyContext(m_imgui_context);
}

#if defined(ERHE_GUI_LIBRARY_IMGUI)
auto Imgui_viewport::want_capture_mouse() const -> bool
{
    return ImGui::GetIO(m_imgui_context).WantCaptureMouse;
}

void Imgui_viewport::on_focus(int focused)
{
    ImGuiIO& io = ImGui::GetIO(m_imgui_context);
    io.AddFocusEvent(focused != 0);
}

[[nodiscard]] auto Imgui_viewport::has_cursor() const -> bool
{
    return m_has_cursor;
}

void Imgui_viewport::on_cursor_enter(int entered)
{
    m_has_cursor = entered != 0;
}

void Imgui_viewport::on_mouse_move(const double x, const double y)
{
    ImGuiIO& io = ImGui::GetIO(m_imgui_context);
    io.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));
}

void Imgui_viewport::on_mouse_click(
    const uint32_t button,
    const int      count
)
{
    if (
        (button < ImGuiMouseButton_COUNT) &&
        (count > 0)
    )
    {
        m_mouse_just_pressed[button] = true;
    }
}

void Imgui_viewport::on_mouse_wheel(
    const double x,
    const double y
)
{
    ImGuiIO& io = ImGui::GetIO(m_imgui_context);
    io.MouseWheelH += static_cast<float>(x);
    io.MouseWheel  += static_cast<float>(y);

    ///// for (const auto& rendertarget : m_rendertarget_imgui_windows)
    ///// {
    /////     rendertarget->on_mouse_wheel(x, y);
    ///// }
}

void Imgui_viewport::make_imgui_context_current()
{
    ImGui::SetCurrentContext(m_imgui_context);
}

void Imgui_viewport::make_imgui_context_uncurrent()
{
    ImGui::SetCurrentContext(nullptr);
}

void Imgui_viewport::on_key(
    const signed int keycode,
    const uint32_t   modifier_mask,
    const bool       pressed
)
{
    ///// for (const auto& rendertarget : m_rendertarget_imgui_windows)
    ///// {
    /////     rendertarget->on_key(keycode, modifier_mask, pressed);
    ///// }

    using erhe::toolkit::Keycode;

    ImGuiIO& io = ImGui::GetIO(m_imgui_context);
    update_key_modifiers(io, modifier_mask);
    io.AddKeyEvent(from_erhe(keycode), pressed);
}

void Imgui_viewport::on_char(
    const unsigned int codepoint
)
{
    ImGuiIO& io = ImGui::GetIO(m_imgui_context);
    io.AddInputCharacter(codepoint);
}

#else
auto Imgui_viewport::want_capture_mouse() const -> bool
{
    return false;
}

void Imgui_viewport::on_focus(int focused)
{
    static_cast<void>(focused);
}

void Imgui_viewport::on_cursor_enter(int entered)
{
    static_cast<void>(entered);
}

void Imgui_viewport::on_mouse_click(
    const uint32_t button,
    const int      count
)
{
    static_cast<void>(button);
    static_cast<void>(count);
}

void Imgui_viewport::on_mouse_wheel(
    const double x,
    const double y
)
{
    static_cast<void>(x);
    static_cast<void>(y);
}

void Imgui_viewport::make_imgui_context_current()
{
}

void Imgui_viewport::make_imgui_context_uncurrent()
{
}

void Imgui_viewport::on_key(
    const signed int keycode,
    const uint32_t   modifier_mask,
    const bool       pressed
)
{
    static_cast<void>(keycode);
    static_cast<void>(modifier_mask);
    static_cast<void>(pressed);
}

void Imgui_viewport::on_char(
    const unsigned int codepoint
)
{
    static_cast<void>(codepoint);
}

#endif

} // namespace erhe::application
