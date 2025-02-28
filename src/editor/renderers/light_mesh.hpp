#pragma once

#include "erhe/components/components.hpp"
#include "erhe/primitive/primitive_geometry.hpp"

#include <glm/glm.hpp>

#include <map>
#include <memory>

namespace erhe::primitive
{
    class Primitive_eometry;
}
namespace erhe::scene
{
    class Light;
}

namespace editor
{

class Program_interface;

class Light_mesh
    : public erhe::components::Component
{
public:
    static constexpr std::string_view c_label{"Light_mesh"};
    static constexpr uint32_t hash = compiletime_xxhash::xxh32(c_label.data(), c_label.size(), {});

    Light_mesh ();
    ~Light_mesh() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void declare_required_components() override;
    void initialize_component       () override;

    // Public API
    [[nodiscard]] auto get_light_transform(const erhe::scene::Light& light) -> glm::mat4;
    [[nodiscard]] auto point_in_light     (const glm::vec3 point_in_world, const erhe::scene::Light& light) -> bool;
    [[nodiscard]] auto get_light_mesh     (const erhe::scene::Light& light) -> erhe::primitive::Primitive_geometry*;

private:
    // Component dependency
    std::shared_ptr<Program_interface>  m_program_interface;

    erhe::primitive::Primitive_geometry m_quad_mesh;
    erhe::primitive::Primitive_geometry m_cone_mesh;
    int                                 m_light_cone_sides{0};
};

} // namespace editor
