#include "operations/insert_operation.hpp"
#include "scene/node_physics.hpp"
#include "scene/helpers.hpp"
#include "tools/selection_tool.hpp"

#include "erhe/scene/scene.hpp"

#include <sstream>

namespace editor
{

Node_transform_operation::Node_transform_operation(const Parameters& parameters)
    : m_parameters{parameters}
{
}

Node_transform_operation::~Node_transform_operation() noexcept
{
}

auto Node_transform_operation::describe() const -> std::string
{
    std::stringstream ss;
    ss << "Node_transform " << m_parameters.node->name();
    return ss.str();
}

void Node_transform_operation::execute(const Operation_context&)
{
    m_parameters.node->set_parent_from_node(m_parameters.parent_from_node_after);
}

void Node_transform_operation::undo(const Operation_context&)
{
    m_parameters.node->set_parent_from_node(m_parameters.parent_from_node_before);
}

auto Mesh_insert_remove_operation::describe() const -> std::string
{
    std::stringstream ss;
    switch (m_parameters.mode)
    {
        //using enum Mode;
        case Mode::insert: ss << "Mesh_insert "; break;
        case Mode::remove: ss << "Mesh_remove "; break;
        default: break;
    }
    ss << m_parameters.mesh->name();
    return ss.str();
}

Mesh_insert_remove_operation::Mesh_insert_remove_operation(const Parameters& parameters)
    : m_parameters{parameters}
{
}

Mesh_insert_remove_operation::~Mesh_insert_remove_operation() noexcept
{
}

void Mesh_insert_remove_operation::execute(const Operation_context& context)
{
    execute(context, m_parameters.mode);
}

void Mesh_insert_remove_operation::undo(const Operation_context& context)
{
    execute(context, inverse(m_parameters.mode));
}

void Mesh_insert_remove_operation::execute(
    const Operation_context& context,
    const Mode               mode
) const
{
    ERHE_VERIFY(m_parameters.mesh);
    ERHE_VERIFY(m_parameters.mesh);

    m_parameters.scene.sanity_check();

    if (mode == Mode::insert)
    {
        m_parameters.scene.add_to_mesh_layer(m_parameters.layer, m_parameters.mesh);
        if (m_parameters.parent)
        {
            m_parameters.parent->attach(m_parameters.mesh);
        }
        if (m_parameters.node_physics)
        {
            add_to_physics_world(m_parameters.physics_world, m_parameters.node_physics);
        }
        if (m_parameters.node_raytrace)
        {
            add_to_raytrace_scene(m_parameters.raytrace_scene, m_parameters.node_raytrace);
        }
    }
    else
    {
        //m_context.selection_tool->remove_from_selection(m_context.mesh);
        m_parameters.scene.remove(m_parameters.mesh);
        if (m_parameters.node_physics)
        {
            remove_from_physics_world(m_parameters.physics_world, *m_parameters.node_physics.get());
        }
        if (m_parameters.node_raytrace)
        {
            remove_from_raytrace_scene(m_parameters.raytrace_scene, m_parameters.node_raytrace);
        }
        if (m_parameters.parent)
        {
            m_parameters.parent->detach(m_parameters.mesh.get());
        }
    }

    auto selection_tool = context.components->get<Selection_tool>();
    if (selection_tool)
    {
        selection_tool->update_selection_from_node(m_parameters.mesh, mode == Mode::insert);
    }
    m_parameters.scene.sanity_check();
}

//
//

auto Light_insert_remove_operation::describe() const -> std::string
{
    std::stringstream ss;
    switch (m_parameters.mode)
    {
        //using enum Mode;
        case Mode::insert: ss << "Light_insert "; break;
        case Mode::remove: ss << "Light_remove "; break;
        default: break;
    }
    ss << m_parameters.light->name();
    return ss.str();
}

Light_insert_remove_operation::Light_insert_remove_operation(const Parameters& parameters)
    : m_parameters{parameters}
{
}

Light_insert_remove_operation::~Light_insert_remove_operation() noexcept
{
}

void Light_insert_remove_operation::execute(const Operation_context& context)
{
    execute(context, m_parameters.mode);
}

void Light_insert_remove_operation::undo(const Operation_context& context)
{
    execute(context, inverse(m_parameters.mode));
}

void Light_insert_remove_operation::execute(
    const Operation_context& context,
    const Mode               mode
)
{
    ERHE_VERIFY(m_parameters.light);
    if (mode == Mode::insert)
    {
        m_parameters.scene.add_to_light_layer(m_parameters.layer, m_parameters.light);
        if (m_parameters.parent)
        {
            m_parameters.parent->attach(m_parameters.light);
        }
    }
    else
    {
        m_parameters.scene.remove(m_parameters.light);
        if (m_parameters.parent)
        {
            m_parameters.parent->detach(m_parameters.light.get());
        }
    }

    auto selection_tool = context.components->get<Selection_tool>();
    if (selection_tool)
    {
        selection_tool->update_selection_from_node(m_parameters.light, mode == Mode::insert);
    }

    m_parameters.scene.sanity_check();
}

//
//

auto Camera_insert_remove_operation::describe() const -> std::string
{
    std::stringstream ss;
    switch (m_parameters.mode)
    {
        //using enum Mode;
        case Mode::insert: ss << "Camera_insert "; break;
        case Mode::remove: ss << "Camera_remove "; break;
        default: break;
    }
    ss << m_parameters.camera->name();
    return ss.str();
}

Camera_insert_remove_operation::Camera_insert_remove_operation(const Parameters& parameters)
    : m_parameters{parameters}
{
}

Camera_insert_remove_operation::~Camera_insert_remove_operation() noexcept
{
}

void Camera_insert_remove_operation::execute(const Operation_context& context)
{
    execute(context, m_parameters.mode);
}

void Camera_insert_remove_operation::undo(const Operation_context& context)
{
    execute(context, inverse(m_parameters.mode));
}

void Camera_insert_remove_operation::execute(
    const Operation_context& context,
    const Mode               mode
) const
{
    ERHE_VERIFY(m_parameters.camera);
    if (mode == Mode::insert)
    {
        m_parameters.scene.add(m_parameters.camera);
        if (m_parameters.parent)
        {
            m_parameters.parent->attach(m_parameters.camera);
        }
    }
    else
    {
        m_parameters.scene.remove(m_parameters.camera);
        if (m_parameters.parent)
        {
            m_parameters.parent->detach(m_parameters.camera.get());
        }
    }

    auto selection_tool = context.components->get<Selection_tool>();
    if (selection_tool)
    {
        selection_tool->update_selection_from_node(m_parameters.camera, mode == Mode::insert);
    }
    m_parameters.scene.sanity_check();
}


}
