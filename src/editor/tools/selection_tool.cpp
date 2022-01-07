#include "tools/selection_tool.hpp"
#include "editor_time.hpp"
#include "editor_tools.hpp"
#include "editor_view.hpp"
#include "log.hpp"
#include "rendering.hpp"

#include "operations/compound_operation.hpp"
#include "operations/insert_operation.hpp"
#include "operations/operation_stack.hpp"
#include "renderers/line_renderer.hpp"
#include "scene/node_physics.hpp"
#include "scene/scene_root.hpp"
#include "tools/pointer_context.hpp"
#include "tools/trs_tool.hpp"
#include "windows/viewport_config.hpp"

#include "erhe/primitive/primitive_geometry.hpp"
#include "erhe/scene/mesh.hpp"
#include "erhe/scene/camera.hpp"
#include "erhe/scene/light.hpp"
#include "erhe/toolkit/math_util.hpp"

#include <imgui.h>

namespace editor
{

using namespace erhe::toolkit;

void Selection_tool_select_command::try_ready(Command_context& context)
{
    if (m_selection_tool.mouse_select_try_ready())
    {
        set_ready(context);
    }
}

auto Selection_tool_select_command::try_call(Command_context& context) -> bool
{
    if (state() != State::Ready)
    {
        return false;
    }

    bool consumed = m_selection_tool.on_mouse_select();
    set_inactive(context);
    return consumed;
}

auto Selection_tool_delete_command::try_call(Command_context& context) -> bool
{
    static_cast<void>(context);

    return m_selection_tool.delete_selection();
}

auto Selection_tool::delete_selection() -> bool
{
    if (m_selection.empty())
    {
        return false;
    }

    const auto scene_root = get<Scene_root>();
    Compound_operation::Context compound_context;
    for (auto node : m_selection)
    {
        const auto mesh = as_mesh(node);
        if (!mesh)
        {
            continue;
        }

        auto* parent = mesh->parent();
        compound_context.operations.push_back(
            std::make_shared<Mesh_insert_remove_operation>(
                Mesh_insert_remove_operation::Context{
                    .scene          = scene_root->scene(),
                    .layer          = scene_root->content_layer(),
                    .physics_world  = scene_root->physics_world(),
                    .mesh           = mesh,
                    .node_physics   = get_physics_node(mesh.get()),
                    .parent         = (parent != nullptr)
                        ? parent->shared_from_this()
                        : std::shared_ptr<erhe::scene::Node>{},
                    .mode           = Scene_item_operation::Mode::remove,
                    .selection_tool = this
                }
            )
        );
    }
    if (compound_context.operations.empty())
    {
        return false;
    }

    auto op = std::make_shared<Compound_operation>(std::move(compound_context));
    get<Operation_stack>()->push(op);
    return true;
}

Selection_tool::Selection_tool()
    : erhe::components::Component{c_name}
    , m_select_command           {*this}
{
}

Selection_tool::~Selection_tool() = default;

void Selection_tool::connect()
{
    m_line_renderer   = get<Line_renderer>();
    m_pointer_context = get<Pointer_context>();
    m_viewport_config = get<Viewport_config>();
}

void Selection_tool::initialize_component()
{
    get<Editor_tools>()->register_tool(this);

    const auto view = get<Editor_view>();
    view->register_command           (&m_select_command);
    view->bind_command_to_mouse_click(&m_select_command, Mouse_button_left);
}

auto Selection_tool::description() -> const char*
{
    return c_description.data();
}

Selection_tool::Subcription Selection_tool::subscribe_selection_change_notification(On_selection_changed callback)
{
    const int handle = m_next_selection_change_subscription++;
    m_selection_change_subscriptions.push_back({callback, handle});
    return Subcription(this, handle);
}

auto Selection_tool::selection() const -> const Selection&
{
    return m_selection;
}

void Selection_tool::set_selection(const Selection& selection)
{
    m_selection = selection;
    call_selection_change_subscriptions();
}

void Selection_tool::unsubscribe_selection_change_notification(int handle)
{
    m_selection_change_subscriptions.erase(
        std::remove_if(
            m_selection_change_subscriptions.begin(),
            m_selection_change_subscriptions.end(),
            [=](Subscription_entry& entry) -> bool
            {
                return entry.handle == handle;
            }
        ),
        m_selection_change_subscriptions.end()
    );
}

auto Selection_tool::mouse_select_try_ready() -> bool
{
    if (m_pointer_context->window() == nullptr)
    {
        return false;
    }

    m_hover_mesh    = m_pointer_context->hover_mesh();
    m_hover_content = m_pointer_context->hovering_over_content();
    m_hover_tool    = m_pointer_context->hovering_over_tool();

    return m_hover_content;
}

auto Selection_tool::on_mouse_select() -> bool
{
    if (m_pointer_context->shift_key_down())
    {
        if (m_hover_content)
        {
            toggle_selection(m_hover_mesh, false);
            return true;
        }
        return false;
    }

    if (!m_hover_content)
    {
        clear_selection();
    }
    else
    {
        toggle_selection(m_hover_mesh, true);
    }
    return true;
}

auto Selection_tool::clear_selection() -> bool
{
    if (m_selection.empty())
    {
        return false;
    }

    for (auto item : m_selection)
    {
        ERHE_VERIFY(item);
        if (!item)
        {
            continue;
        }
        item->visibility_mask() &= ~erhe::scene::Node::c_visibility_selected;
    }

    log_selection.trace("Clearing selection ({} items were selected)\n", m_selection.size());
    m_selection.clear();
    call_selection_change_subscriptions();
    return true;
}

void Selection_tool::toggle_selection(
    const std::shared_ptr<erhe::scene::Node>& item,
    const bool                                clear_others
)
{
    if (clear_others)
    {
        const bool was_selected = is_in_selection(item);

        clear_selection();
        if (!was_selected && item)
        {
            add_to_selection(item);
        }
    }
    else if (item)
    {
        if (is_in_selection(item))
        {
            remove_from_selection(item);
        }
        else
        {
            add_to_selection(item);
        }
    }

    call_selection_change_subscriptions();
}

auto Selection_tool::is_in_selection(
    const std::shared_ptr<erhe::scene::Node>& item
) const -> bool
{
    if (!item)
    {
        return false;
    }

    return std::find(
        m_selection.begin(),
        m_selection.end(),
        item
    ) != m_selection.end();
}

auto Selection_tool::add_to_selection(
    const std::shared_ptr<erhe::scene::Node>& item
) -> bool
{
    if (!item)
    {
        log_selection.warn("Trying to add empty item to selection\n");
        return false;
    }

    item->visibility_mask() |= erhe::scene::Node::c_visibility_selected;

    if (!is_in_selection(item))
    {
        log_selection.trace("Adding {} to selection\n", item->name());
        m_selection.push_back(item);
        call_selection_change_subscriptions();
        return true;
    }

    log_selection.warn("Adding {} to selection failed - was already in selection\n", item->name());
    return false;
}

auto Selection_tool::remove_from_selection(
    const std::shared_ptr<erhe::scene::Node>& item
) -> bool
{
    if (!item)
    {
        log_selection.warn("Trying to remove empty item from selection\n");
        return false;
    }

    item->visibility_mask() &= ~erhe::scene::Node::c_visibility_selected;

    const auto i = std::remove(
        m_selection.begin(),
        m_selection.end(),
        item
    );
    if (i != m_selection.end())
    {
        log_selection.trace("Removing item {} from selection\n", item->name());
        m_selection.erase(i, m_selection.end());
        call_selection_change_subscriptions();
        return true;
    }

    log_selection.info("Removing item {} from selection failed - was not in selection\n", item->name());
    return false;
}

void Selection_tool::call_selection_change_subscriptions() const
{
    for (const auto& entry : m_selection_change_subscriptions)
    {
        entry.callback(m_selection);
    }
}

namespace 
{

auto sign(const float x) -> float
{
    return x < 0.0f ? -1.0f : 1.0f;
}

}

void Selection_tool::tool_render(const Render_context& context)
{
    using namespace glm;

    if (m_line_renderer == nullptr)
    {
        return;
    }

    constexpr uint32_t red        = 0xff0000ffu;
    constexpr uint32_t green      = 0xff00ff00u;
    constexpr uint32_t blue       = 0xffff0000u;
    constexpr uint32_t yellow     = 0xff00ffffu;
    constexpr uint32_t white      = 0xffffffffu;
    constexpr uint32_t half_white = 0x88888888u; // premultiplied
    constexpr float    thickness  = 10.0f;
    auto& line_renderer = m_line_renderer->hidden;
    for (auto node : m_selection)
    {
        const glm::mat4 m     {node->world_from_node()};
        const glm::vec3 O     {0.0f};
        const glm::vec3 axis_x{1.0f, 0.0f, 0.0f};
        const glm::vec3 axis_y{0.0f, 1.0f, 0.0f};
        const glm::vec3 axis_z{0.0f, 0.0f, 1.0f};
        line_renderer.add_lines( m, red,   {{ O, axis_x }}, thickness );
        line_renderer.add_lines( m, green, {{ O, axis_y }}, thickness );
        line_renderer.add_lines( m, blue,  {{ O, axis_z }}, thickness );

        if (is_mesh(node))
        {
            const auto& mesh = as_mesh(node);
            for (auto primitive : mesh->data.primitives)
            {
                if (!primitive.source_geometry)
                {
                    continue;
                }
                const auto& primitive_geometry = primitive.gl_primitive_geometry;
                const vec3 box_min = primitive_geometry.bounding_box_min;
                const vec3 box_max = primitive_geometry.bounding_box_max;
                line_renderer.add_lines(
                    node->world_from_node(),
                    yellow,
                    {
                        { vec3{box_min.x, box_min.y, box_min.z}, vec3{box_max.x, box_min.y, box_min.z} },
		                { vec3{box_max.x, box_min.y, box_min.z}, vec3{box_max.x, box_max.y, box_min.z} },
		                { vec3{box_max.x, box_max.y, box_min.z}, vec3{box_min.x, box_max.y, box_min.z} },
		                { vec3{box_min.x, box_max.y, box_min.z}, vec3{box_min.x, box_min.y, box_min.z} },
		                { vec3{box_min.x, box_min.y, box_min.z}, vec3{box_min.x, box_min.y, box_max.z} },
		                { vec3{box_max.x, box_min.y, box_min.z}, vec3{box_max.x, box_min.y, box_max.z} },
		                { vec3{box_max.x, box_max.y, box_min.z}, vec3{box_max.x, box_max.y, box_max.z} },
		                { vec3{box_min.x, box_max.y, box_min.z}, vec3{box_min.x, box_max.y, box_max.z} },
		                { vec3{box_min.x, box_min.y, box_max.z}, vec3{box_max.x, box_min.y, box_max.z} },
		                { vec3{box_max.x, box_min.y, box_max.z}, vec3{box_max.x, box_max.y, box_max.z} },
		                { vec3{box_max.x, box_max.y, box_max.z}, vec3{box_min.x, box_max.y, box_max.z} },
                        { vec3{box_min.x, box_max.y, box_max.z}, vec3{box_min.x, box_min.y, box_max.z} }
                    },
                    thickness
                );
            }
        }

        if (
            is_light(node) &&
            (m_viewport_config->debug_visualizations.light == Visualization_mode::selected)
        )
        {
            const auto& light = as_light(node);
            const uint32_t light_color = ImGui::ColorConvertFloat4ToU32(
                ImVec4{
                    light->color.r,
                    light->color.g,
                    light->color.b,
                    1.0f
                }
            );
            const uint32_t half_light_color = ImGui::ColorConvertFloat4ToU32(
                ImVec4{
                    0.5f * light->color.r,
                    0.5f * light->color.g,
                    0.5f * light->color.b,
                    0.5f
                }
            );
            if (light->type == erhe::scene::Light_type::directional)
            {
                line_renderer.add_lines(m, light_color, {{ O, -light->range * axis_z }}, thickness );
            }
            if (light->type == erhe::scene::Light_type::point)
            {
                constexpr float scale = 0.5f;
                const auto nnn = scale * normalize(-axis_x - axis_y - axis_z);
                const auto nnp = scale * normalize(-axis_x - axis_y + axis_z);
                const auto npn = scale * normalize(-axis_x + axis_y - axis_z);
                const auto npp = scale * normalize(-axis_x + axis_y + axis_z);
                const auto pnn = scale * normalize( axis_x - axis_y - axis_z);
                const auto pnp = scale * normalize( axis_x - axis_y + axis_z);
                const auto ppn = scale * normalize( axis_x + axis_y - axis_z);
                const auto ppp = scale * normalize( axis_x + axis_y + axis_z);
                line_renderer.add_lines(
                    m,
                    light_color,
                    {
                        { -scale * axis_x, scale * axis_x},
                        { -scale * axis_y, scale * axis_y},
                        { -scale * axis_z, scale * axis_z},
                        { nnn, ppp },
                        { nnp, ppn },
                        { npn, pnp },
                        { npp, pnn }
                    },
                    thickness
                );
            }
            if (light->type == erhe::scene::Light_type::spot)
            {
                constexpr int   edge_count       = 200;
                constexpr float light_cone_sides = edge_count * 6;
                const float outer_alpha   = light->outer_spot_angle;
                const float inner_alpha   = light->inner_spot_angle;
                const float length        = light->range;
                const float outer_apothem = length * std::tan(outer_alpha * 0.5f);
                const float inner_apothem = length * std::tan(inner_alpha * 0.5f);
                const float outer_radius  = outer_apothem / std::cos(pi<float>() / static_cast<float>(light_cone_sides));
                const float inner_radius  = inner_apothem / std::cos(pi<float>() / static_cast<float>(light_cone_sides));

                const vec3 view_position = node->transform_point_from_world_to_local(context.camera->position_in_world());
                //auto* editor_time = get<Editor_time>();
                //const float time = static_cast<float>(editor_time->time());
                //const float half_position = (editor_time != nullptr)
                //    ? time - floor(time)
                //    : 0.5f;

                for (int i = 0; i < light_cone_sides; ++i)
                {
                    const float t0 = two_pi<float>() * static_cast<float>(i    ) / static_cast<float>(light_cone_sides);
                    const float t1 = two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(light_cone_sides);
                    line_renderer.add_lines(
                        m,
                        light_color,
                        {
                            {
                                -length * axis_z 
                                + outer_radius * std::cos(t0) * axis_x 
                                + outer_radius * std::sin(t0) * axis_y,
                                -length * axis_z 
                                + outer_radius * std::cos(t1) * axis_x 
                                + outer_radius * std::sin(t1) * axis_y
                            }
                        },
                        thickness
                    );
                    line_renderer.add_lines(
                        m,
                        half_light_color,
                        {
                            {
                                -length * axis_z 
                                + inner_radius * std::cos(t0) * axis_x 
                                + inner_radius * std::sin(t0) * axis_y,
                                -length * axis_z 
                                + inner_radius * std::cos(t1) * axis_x 
                                + inner_radius * std::sin(t1) * axis_y
                            }
                            //{
                            //    -length * axis_z * half_position
                            //    + outer_radius * std::cos(t0) * axis_x * half_position 
                            //    + outer_radius * std::sin(t0) * axis_y * half_position,
                            //    -length * axis_z * half_position
                            //    + outer_radius * std::cos(t1) * axis_x * half_position
                            //    + outer_radius * std::sin(t1) * axis_y * half_position
                            //}
                        },
                        thickness
                    );
                }
                line_renderer.add_lines(
                    m,
                    half_light_color,
                    {
                        //{
                        //    O,
                        //    -length * axis_z - outer_radius * axis_x,
                        //},
                        //{
                        //    O,
                        //    -length * axis_z + outer_radius * axis_x
                        //},
                        //{
                        //    O,
                        //    -length * axis_z - outer_radius * axis_y,
                        //},
                        //{
                        //    O,
                        //    -length * axis_z + outer_radius * axis_y
                        //},
                        {
                            O,
                            -length * axis_z
                        },
                        {
                            -length * axis_z - inner_radius * axis_x,
                            -length * axis_z + inner_radius * axis_x
                        },
                        {
                            -length * axis_z - inner_radius * axis_y,
                            -length * axis_z + inner_radius * axis_y
                        }
                    },
                    thickness
                );

                class Cone_edge
                {
                public:
                    Cone_edge(
                        const glm::vec3& p,
                        const glm::vec3& n,
                        const glm::vec3& t,
                        const glm::vec3& b,
                        const float      phi,
                        const float      n_dot_v
                    )
                    : p      {p}
                    , n      {n}
                    , t      {t}
                    , b      {b}
                    , phi    {phi}
                    , n_dot_v{n_dot_v}
                    {
                    }

                    glm::vec3 p;
                    glm::vec3 n;
                    glm::vec3 t;
                    glm::vec3 b;
                    float     phi;
                    float     n_dot_v;
                };

                std::vector<Cone_edge> cone_edges;
                for (int i = 0; i < edge_count; ++i)
                {
                    const float phi     = two_pi<float>() * static_cast<float>(i) / static_cast<float>(edge_count);
                    const float sin_phi = std::sin(phi);
                    const float cos_phi = std::cos(phi);

                    const vec3 p{
                        + outer_radius * sin_phi,
                        + outer_radius * cos_phi,
                        -length
                    };

                    const vec3 B = normalize(O - p); // generatrix
                    const vec3 T{
                        static_cast<float>(std::sin(phi + half_pi<float>())),
                        static_cast<float>(std::cos(phi + half_pi<float>())),
                        0.0f
                    };
                    const vec3  N0      = glm::cross(B, T);
                    const vec3  N       = glm::normalize(N0);
                    const vec3  v       = normalize(p - view_position);
                    const float n_dot_v = dot(N, v);

                    cone_edges.emplace_back(
                        p,
                        N,
                        T,
                        B,
                        phi,
                        n_dot_v
                    );
                }

                std::vector<Cone_edge> sign_flip_edges;
                for (size_t i = 0; i < cone_edges.size(); ++i)
                {
                    const size_t next_i    = (i + 1) % cone_edges.size();
                    const auto&  edge      = cone_edges[i];
                    const auto&  next_edge = cone_edges[next_i];
                    if (sign(edge.n_dot_v) != sign(next_edge.n_dot_v))
                    {
                        if (std::abs(edge.n_dot_v) < std::abs(next_edge.n_dot_v))
                        {
                            sign_flip_edges.push_back(edge);
                        }
                        else
                        {
                            sign_flip_edges.push_back(next_edge);
                        }
                    }
                }

                for (auto& edge : sign_flip_edges)
                {
                    line_renderer.add_lines(m, light_color, { { O, edge.p } }, thickness);
                    //line_renderer.add_lines(m, red,   { { edge.p, edge.p + 0.1f * edge.n } }, thickness);
                    //line_renderer.add_lines(m, green, { { edge.p, edge.p + 0.1f * edge.t } }, thickness);
                    //line_renderer.add_lines(m, blue,  { { edge.p, edge.p + 0.1f * edge.b } }, thickness);
                }
            }
        }

        if (
            is_icamera(node) &&
            (
                (
                    !is_light(node) &&
                    (m_viewport_config->debug_visualizations.camera == Visualization_mode::selected)
                ) ||
                (
                    is_light(node) &&
                    (m_viewport_config->debug_visualizations.light_camera == Visualization_mode::selected)
                )
            )

        )
        {
            const auto& icamera         = as_icamera(node).get();
            const mat4  clip_from_node  = icamera->projection()->get_projection_matrix(1.0f, context.viewport.reverse_depth);
            const mat4  node_from_clip  = inverse(clip_from_node);
            const mat4  world_from_clip = icamera->world_from_node() * node_from_clip;
            constexpr std::array<glm::vec3, 8> p = {
                glm::vec3{-1.0f, -1.0f, 0.0f},
                glm::vec3{ 1.0f, -1.0f, 0.0f},
                glm::vec3{ 1.0f,  1.0f, 0.0f},
                glm::vec3{-1.0f,  1.0f, 0.0f},
                glm::vec3{-1.0f, -1.0f, 1.0f},
                glm::vec3{ 1.0f, -1.0f, 1.0f},
                glm::vec3{ 1.0f,  1.0f, 1.0f},
                glm::vec3{-1.0f,  1.0f, 1.0f}
            };

            line_renderer.add_lines(
                world_from_clip,
                white,
                {
                    // near plane
                    { p[0], p[1] },
                    { p[1], p[2] },
                    { p[2], p[3] },
                    { p[3], p[0] },

                    // far plane
                    { p[4], p[5] },
                    { p[5], p[6] },
                    { p[6], p[7] },
                    { p[7], p[4] },

                    // near to far
                    { p[0], p[4] },
                    { p[1], p[5] },
                    { p[2], p[6] },
                    { p[3], p[7] }
                },
                thickness
            );
            line_renderer.add_lines(
                world_from_clip,
                half_white,
                {
                    // near to far middle
                    { 0.5f * p[0] + 0.5f * p[1], 0.5f * p[4] + 0.5f * p[5] },
                    { 0.5f * p[1] + 0.5f * p[2], 0.5f * p[5] + 0.5f * p[6] },
                    { 0.5f * p[2] + 0.5f * p[3], 0.5f * p[6] + 0.5f * p[7] },
                    { 0.5f * p[3] + 0.5f * p[0], 0.5f * p[7] + 0.5f * p[4] },

                    // near+far/2 plane
                    { 0.5f * p[0] + 0.5f * p[4], 0.5f * p[1] + 0.5f * p[5] },
                    { 0.5f * p[1] + 0.5f * p[5], 0.5f * p[2] + 0.5f * p[6] },
                    { 0.5f * p[2] + 0.5f * p[6], 0.5f * p[3] + 0.5f * p[7] },
                    { 0.5f * p[3] + 0.5f * p[7], 0.5f * p[0] + 0.5f * p[4] },
                },
                thickness
            );
        }
    }
}

}
