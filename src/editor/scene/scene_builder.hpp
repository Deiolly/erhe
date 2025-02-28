#pragma once

#include "scene/collision_generator.hpp"
#include "scene/frame_controller.hpp"
#include "windows/brushes.hpp"

#include "erhe/components/components.hpp"

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

class btCollisionShape;

namespace erhe::geometry
{
    class Geometry;
}

namespace erhe::graphics
{
    class Buffer;
    class Buffer_transfer_queue;
    class Vertex_format;
}

namespace erhe::primitive
{
    class Primitive;
    class Primitive_eometry;
}

namespace erhe::scene
{
    class Camera;
    class Light;
    class Mesh;
    class Mesh_layer;
    class Scene;
}

namespace editor
{

class Brush;
class Debug_draw;
class Mesh_memory;
class Node_physics;
class Rendertarget_viewport;
class Scene_root;

class Scene_builder
    : public erhe::components::Component
    , public erhe::components::IUpdate_fixed_step
    , public erhe::components::IUpdate_once_per_frame
{
public:
    static constexpr std::string_view c_label{"Scene_builder"};
    static constexpr uint32_t hash = compiletime_xxhash::xxh32(c_label.data(), c_label.size(), {});

    Scene_builder ();
    ~Scene_builder() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void declare_required_components() override;
    void initialize_component       () override;

    // Implements IUpdate_once_per_frame
    void update_once_per_frame(const erhe::components::Time_context& time_context) override;

    // Implements IUpdate_fixed_step
    void update_fixed_step(const erhe::components::Time_context& time_context) override;

    // Public API
    void setup_scene();

    // Can discard return value
    auto make_camera(
        const std::string_view name,
        const glm::vec3        position,
        const glm::vec3        look_at = glm::vec3{0.0f, 0.0f, 0.0f}
    ) -> std::shared_ptr<erhe::scene::Camera>;

    auto make_directional_light(
        const std::string_view name,
        const glm::vec3        position,
        const glm::vec3        color,
        const float            intensity
    ) -> std::shared_ptr<erhe::scene::Light>;

    auto make_spot_light(
        const std::string_view name,
        const glm::vec3        position,
        const glm::vec3        direction,
        const glm::vec3        color,
        const float            intensity,
        const glm::vec2        spot_cone_angle
    ) -> std::shared_ptr<erhe::scene::Light>;

    template <typename ...Args>
    auto make_brush(
        const bool instantiate_to_scene,
        Args&& ...args
    ) -> std::shared_ptr<Brush>
    {
        // cppcheck-suppress redundantAssignment
        const auto brush = m_brushes->make_brush(std::forward<Args>(args)...);
        if (instantiate_to_scene)
        {
            const std::lock_guard<std::mutex> lock{m_scene_brushes_mutex};

            m_scene_brushes.push_back(brush);
        }
        return brush;
    }

private:
    [[nodiscard]] auto build_info           () -> erhe::primitive::Build_info&;
    [[nodiscard]] auto buffer_transfer_queue() -> erhe::graphics::Buffer_transfer_queue&;

    void setup_cameras      ();
    void animate_lights     (const double time_d);
    void add_room           ();
    void make_brushes       ();
    void make_mesh_nodes    ();
    void make_cube_benchmark();
    void setup_lights       ();

    // Component dependencies
    std::shared_ptr<Brushes>     m_brushes;
    std::shared_ptr<Mesh_memory> m_mesh_memory;
    std::shared_ptr<Scene_root>  m_scene_root;

    // Self owned parts
    std::mutex                             m_brush_mutex;
    std::unique_ptr<Brush>                 m_floor_brush;
    std::unique_ptr<Brush>                 m_table_brush;
    std::mutex                             m_scene_brushes_mutex;
    std::vector<std::shared_ptr<Brush>>    m_scene_brushes;
    std::shared_ptr<Rendertarget_viewport> m_rendertarget_viewport;

    std::vector<std::shared_ptr<erhe::physics::ICollision_shape>> m_collision_shapes;
};

} // namespace editor
