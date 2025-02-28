#include "game/player.hpp"
#include "map.hpp"
#include "rendering.hpp"
#include "tile_renderer.hpp"
#include "tiles.hpp"
#include "unit_type.hpp"
#include "game/game.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace hextiles
{

[[nodiscard]] auto clamp(float x, float lowerlimit, float upperlimit) -> float
{
    if (x < lowerlimit)
    {
        x = lowerlimit;
    }
    if (x > upperlimit)
    {
        x = upperlimit;
    }
    return x;
}

[[nodiscard]] auto smoothstep(float edge0, float edge1, float x) -> float
{
    if (x == 0.0f)
    {
        return 0.0f;
    }
    if (x == 1.0f)
    {
        return 1.0f;
    }

    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    // Evaluate polynomial
    return x * x * (3.0f - 2.0f * x);
}

[[nodiscard]] auto smootherstep(float edge0, float edge1, float x) -> float
{
    if (x == 0.0f)
    {
        return 0.0f;
    }
    if (x == 1.0f)
    {
        return 1.0f;
    }

    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    // Evaluate polynomial
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

void Player::city_imgui(Game_context& context)
{
    const int    player     = context.game.get_current_player().id;
    const size_t city_index = m_current_unit;

    Unit& city = cities.at(city_index);
    const unit_tile_t unit_tile = context.tile_renderer.get_single_unit_tile(player, city.type);
    const Unit_type&  product   = context.tiles.get_unit_type(city.production);

    ImGui::Text("City %zu", city_index);
    ImGui::SameLine();
    context.rendering.unit_image(unit_tile, 2);
    ImGui::SameLine();
    ImGui::InputText("##city_name", &city.name);
    ImGui::Text("%d, %d", city.location.x, city.location.y);
    context.rendering.make_unit_type_combo("Product", city.production, id);
    ImGui::Text("PT: %d/%d", city.production_progress, product.production_time);
}

void Player::unit_imgui(Game_context& context)
{
    const int    player     = context.game.get_current_player().id;
    const size_t unit_index = m_current_unit - cities.size();

    Unit& unit = units.at(unit_index);
    const unit_tile_t unit_tile = context.tile_renderer.get_single_unit_tile(player, unit.type);
    const Unit_type&  unit_type = context.tiles.get_unit_type(unit.type);

    ImGui::Text("Unit %zu", unit_index);
    ImGui::SameLine();
    context.rendering.unit_image(unit_tile, 2);
    ImGui::SameLine();
    ImGui::Text("%s", unit_type.name.c_str());
    ImGui::Text("%d, %d", unit.location.x, unit.location.y);
    ImGui::Text("Hit Points: %d / %d", unit.hit_points, unit_type.hit_points);
    ImGui::Text("Move Points: %d / %d", unit.move_points, unit_type.move_points[0]);
    ImGui::Text("Fuel: %d / %d", unit.fuel, unit_type.fuel);
}

void Player::animate_current_unit(Game_context& context)
{
    const double      sin_time         = std::sin(context.time * 10.0f);
    const bool        show_active_unit = sin_time > 0.0f;
    const size_t      unit_index       = m_current_unit - cities.size();
    const Unit&       unit             = units.at(unit_index);
    const unit_tile_t unit_tile =
        show_active_unit
            ? context.tile_renderer.get_single_unit_tile(id, unit.type)
            : context.game.get_unit_tile(unit.location, &unit);
    map->set_unit_tile(unit.location, unit_tile);
}

void Player::update(Game_context& context)
{
    if (m_current_unit < cities.size())
    {
        return; // TODO
    }

    if (m_move.has_value())
    {
        const Move&           move         = m_move.value();
        const size_t          unit_index   = m_current_unit - cities.size();
        Unit& unit = units.at(unit_index);
        const Unit_type&      unit_type    = context.tiles.get_unit_type(unit.type);
        const Tile_coordinate old_location = unit.location;
        const Tile_coordinate new_location = move.target;

        unit.location = new_location;
        context.game.update_map_unit_tile(old_location);
        context.game.update_map_unit_tile(new_location);
        context.game.reveal(*map.get(), old_location, unit_type.vision_range[0]);
        context.game.reveal(*map.get(), new_location, unit_type.vision_range[0]);
        m_move.reset();
    }
}

void Player::move_unit(Game_context& context, direction_t direction)
{
    // Cities can not be moved
    if (m_current_unit < cities.size())
    {
        return;
    }

    const size_t unit_index = m_current_unit - cities.size();
    Unit& unit = units.at(unit_index);
    //const Unit_type&      unit_type    = context.tiles.get_unit_type(unit.type);
    const Tile_coordinate old_location = unit.location;
    const Tile_coordinate new_location = map->wrap(old_location.neighbor(direction));

    const unit_tile_t unit_tile = context.game.get_unit_tile(unit.location, &unit);
    map->set_unit_tile(unit.location, unit_tile);

    m_move = Move{
        .target     = new_location,
        .start_time = context.time
    };

    //unit.location = new_location;
    //context.game.update_map_unit_tile(old_location);
    //context.game.update_map_unit_tile(new_location);
    //context.game.reveal(*map.get(), old_location, unit_type.vision_range[0]);
    //context.game.reveal(*map.get(), new_location, unit_type.vision_range[0]);
}

void Player::select_unit(Game_context& context, int direction)
{
    if (m_current_unit >= cities.size())
    {
        const size_t unit_index = m_current_unit - cities.size();
        Unit& unit = units.at(unit_index);
        context.game.reveal(*map.get(), unit.location, 0);
    }

    if (direction > 0)
    {
        ++m_current_unit;
        if (m_current_unit >= (cities.size() + units.size()))
        {
            m_current_unit = 0;
        }
    }
    else
    {
        if (m_current_unit > 0)
        {
            --m_current_unit;
        }
        else
        {
            m_current_unit = cities.size() + units.size() - 1;
        }
    }
}

void Player::imgui(Game_context& context)
{
    constexpr ImVec2 button_size{110.0f, 0.0f};

    if (ImGui::Button("Next Unit", button_size))
    {
        select_unit(context, 1);
    }

    if (ImGui::Button("Previous Unit", button_size))
    {
        select_unit(context, -1);
    }

    if (m_current_unit < cities.size())
    {
        city_imgui(context);
    }
    else
    {
        unit_imgui(context);
    }
}

void Player::fog_of_war(Game_context& context)
{
    // Step 1: Half-hide explored map cells
    const coordinate_t width      = static_cast<coordinate_t>(map->width());
    const coordinate_t height     = static_cast<coordinate_t>(map->height());
    const unit_tile_t  fog_of_war = context.tile_renderer.get_special_unit_tile(Special_unit_tiles::fog_of_war);
    const unit_tile_t  half_fog   = context.tile_renderer.get_special_unit_tile(Special_unit_tiles::half_fog_of_war);
    for (coordinate_t y = 0; y < height; ++y)
    {
        for (coordinate_t x = 0; x < width; ++x)
        {
            const Tile_coordinate position{x, y};
            const unit_tile_t     old_unit_tile = map->get_unit_tile(position);
            if (old_unit_tile != fog_of_war)
            {
                map->set_unit_tile(
                    Tile_coordinate{x, y},
                    half_fog
                );
            }
        }
    }

    // Step 2: Reveal map cells that can be seen
    Map& map_reference = *map.get();
    for (Unit& city : cities)
    {
        const Unit_type& unit_type = context.tiles.get_unit_type(city.type);
        context.game.reveal(map_reference, city.location, unit_type.vision_range[0]);
    }
    for (Unit& unit : units)
    {
        const Unit_type& unit_type = context.tiles.get_unit_type(unit.type);
        context.game.reveal(map_reference, unit.location, unit_type.vision_range[0]);
    }
}

void Player::update_units(Game_context& context)
{
    for (Unit& unit : units)
    {
        const Unit_type& unit_type = context.tiles.get_unit_type(unit.type);
        unit.move_points = unit_type.move_points[0];
        // TODO repair and refuel if in city or carrier
    }
}

void Player::update_cities(Game_context& context)
{
    // Progress production
    for (Unit& city : cities)
    {
        if (city.production == unit_t{0})
        {
            continue;
        }
        //Unit_type& product = context.tiles.get_unit_type(city.production);
        ++city.production_progress;
        if (city.production_progress >= 1) //product.production_time)
        {
            Unit unit = context.game.make_unit(city.production, city.location);
            units.push_back(unit);
            city.production_progress = 0;
        }
    }
}


} // namespace hextiles
