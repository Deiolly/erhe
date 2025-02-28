#pragma once

#include "erhe/components/components.hpp"

#include <gsl/gsl>

namespace erhe::application{
    class Imgui_windows;
}

namespace editor
{

class Render_context;
class Tool;

class Tools
    : public erhe::components::Component
{
public:
    static constexpr std::string_view c_label{"Editor_tools"};
    static constexpr std::string_view c_title{"Editor Tools"};
    static constexpr uint32_t         hash{
        compiletime_xxhash::xxh32(
            c_label.data(),
            c_label.size(),
            {}
        )
    };

    Tools ();
    ~Tools() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void post_initialize() override;

    // Public API
    void render_tools            (const Render_context& context);
    void register_tool           (Tool* tool);
    void register_background_tool(Tool* tool);

private:
    // Component dependencies
    std::shared_ptr<erhe::application::Imgui_windows> m_imgui_windows;

    std::mutex                        m_mutex;
    std::vector<gsl::not_null<Tool*>> m_tools;
    std::vector<gsl::not_null<Tool*>> m_background_tools;
};

} // namespace editor
