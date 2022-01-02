#pragma once

#include "renderers/buffer_writer.hpp"
#include "renderers/programs.hpp"
#include "renderers/frame_resources.hpp"
#include "renderers/light_mesh.hpp"

#include "erhe/components/component.hpp"
#include "erhe/gl/wrapper_enums.hpp"
#include "erhe/graphics/configuration.hpp"

#include "erhe/scene/mesh.hpp"
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>


namespace erhe::primitive
{
    class Primitive;
    class Material;
}

namespace erhe::scene
{
    class ICamera;
    class Camera;
    class Light;
    class Light_layer;
    class Mesh_layer;
    class Node;
    class Viewport;
    class Visibility_filter;
}

namespace editor
{

class Programs;

class Base_renderer
{
public:
    enum class Primitive_color_source : unsigned int
    {
        id_offset = 0,
        mesh_wireframe_color,
        constant_color,
    };

    static constexpr std::array<std::string_view, 3> c_primitive_color_source_strings =
    {
        "ID Offset",
        "Mesh Wireframe color",
        "Constant Color"
    };

    static constexpr std::array<const char*, 3> c_primitive_color_source_strings_data{
        std::apply(
            [](auto&&... s)
            {
                return std::array{s.data()...};
            },
            c_primitive_color_source_strings
        )
    };

    using Mesh_layer_collection = std::vector<const erhe::scene::Mesh_layer*>;

    Primitive_color_source primitive_color_source  {Primitive_color_source::constant_color};
    glm::vec4              primitive_constant_color{1.0f, 1.0f, 1.0f, 1.0f};

    enum class Primitive_size_source : unsigned int
    {
        mesh_point_size,
        mesh_line_width,
        constant_size
    };
    Primitive_size_source primitive_size_source  {Primitive_size_source::constant_size};
    float                 primitive_constant_size{1.0f};

    class Draw_indirect_buffer_range
    {
    public:
        Buffer_range range;
        size_t       draw_indirect_count{0};
    };

    class Id_range
    {
    public:
        uint32_t                           offset         {0};
        uint32_t                           length         {0};
        std::shared_ptr<erhe::scene::Mesh> mesh           {};
        size_t                             primitive_index{0};
    };

    using Material_collection = std::vector<std::shared_ptr<erhe::primitive::Material>>;

    explicit Base_renderer(const std::string& name);
    virtual ~Base_renderer();

    static constexpr size_t s_frame_resources_count = 4;

    void create_frame_resources(
        const size_t material_count,
        const size_t light_count,
        const size_t camera_count,
        const size_t primitive_count,
        const size_t draw_count
    );

    [[nodiscard]] auto current_frame_resources() -> Frame_resources&;

    void next_frame();

    void base_connect(const erhe::components::Component* component);

    // Can discard return value
    auto update_primitive_buffer(
        const erhe::scene::Mesh_layer&        mesh_layer,
        const erhe::scene::Visibility_filter& visibility_filter,
        const bool                            use_id_ranges = false
    ) -> Buffer_range;

    // Can discard return value
    auto update_light_buffer(
        const erhe::scene::Light_layer& light_layer,
        const erhe::scene::Viewport     light_texture_viewport
    ) -> Buffer_range;

    // Can discard return value
    auto update_material_buffer(const Material_collection& materials) -> Buffer_range;

    // Can discard return value
    auto update_camera_buffer(
        erhe::scene::ICamera&       camera,
        const erhe::scene::Viewport viewport
    ) -> Buffer_range;

    // Can discard return value
    auto update_draw_indirect_buffer(
        const erhe::scene::Mesh_layer&        mesh_layer,
        const erhe::primitive::Primitive_mode primitive_mode,
        const erhe::scene::Visibility_filter& visibility_filter
    ) -> Draw_indirect_buffer_range;

    void bind_material_buffer     ();
    void bind_light_buffer        ();
    void bind_camera_buffer       ();
    void bind_primitive_buffer    ();
    void bind_draw_indirect_buffer();
    void reset_id_ranges          ();
    auto id_offset                () const -> uint32_t;
    auto id_ranges                () const -> const std::vector<Id_range>&;
    void debug_properties_window  ();
    auto max_index_count_enable   () const -> bool;
    auto max_index_count          () const -> int;

private:
    Program_interface*           m_program_interface{nullptr};

    std::string                  m_name;
    std::vector<Frame_resources> m_frame_resources;
    size_t                       m_current_frame_resource_slot{0};

    Buffer_writer                m_material_writer;
    Buffer_writer                m_light_writer;
    Buffer_writer                m_camera_writer;
    Buffer_writer                m_primitive_writer;
    Buffer_writer                m_draw_indirect_writer;

    uint32_t                     m_id_offset{0};
    std::vector<Id_range>        m_id_ranges;

    bool                         m_max_index_count_enable{false};
    int                          m_max_index_count{256};
};

} // namespace editor
