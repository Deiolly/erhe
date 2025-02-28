#include "erhe/graphics/texture.hpp"
#include "erhe/gl/enum_string_functions.hpp"
#include "erhe/gl/wrapper_functions.hpp"
#include "erhe/graphics/buffer.hpp"
#include "erhe/graphics/configuration.hpp"
#include "erhe/graphics/graphics_log.hpp"
#include "erhe/graphics/sampler.hpp"
#include "erhe/toolkit/verify.hpp"

#include <algorithm>

namespace erhe::graphics
{

class InternalFormatFormatType
{
public:
    gl::Internal_format internal_format;
    gl::Pixel_format    format;
    gl::Pixel_type      type;
};

auto component_count(const gl::Pixel_format pixel_format) -> size_t
{
    switch (pixel_format)
    {
        //using enum gl::Pixel_format;
        case gl::Pixel_format::red:
        case gl::Pixel_format::red_integer:
        {
            return 1;
        }

        case gl::Pixel_format::rg:
        case gl::Pixel_format::rg_integer:
        {
            return 2;
        }

        case gl::Pixel_format::rgb:
        case gl::Pixel_format::rgb_integer:
        {
            return 3;
        }

        case gl::Pixel_format::rgba:
        case gl::Pixel_format::rgba_integer:
        {
            return 4;
        }

        default:
        {
            ERHE_FATAL("Bad pixel format");
        }
    }
}

auto byte_count(const gl::Pixel_type pixel_type) -> size_t
{
    switch (pixel_type)
    {
        //using enum gl::Pixel_type;
        case gl::Pixel_type::unsigned_byte:
        case gl::Pixel_type::byte:
        {
            return 1;
        }

        case gl::Pixel_type::unsigned_short:
        case gl::Pixel_type::short_:
        {
            return 2;
        }

        case gl::Pixel_type::unsigned_int:
        case gl::Pixel_type::int_:
        case gl::Pixel_type::float_:
        {
            return 4;
        }

        default:
        {
            ERHE_FATAL("Bad pixel type");
        }
    }
};

constexpr std::array INTERNAL_FORMAT_INFO =
{
    InternalFormatFormatType{ gl::Internal_format::r8               , gl::Pixel_format::red            , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::r8_snorm         , gl::Pixel_format::red            , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::r16f             , gl::Pixel_format::red            , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::r32f             , gl::Pixel_format::red            , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::rg8              , gl::Pixel_format::rg             , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rg8_snorm        , gl::Pixel_format::rg             , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::rg16f            , gl::Pixel_format::rg             , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::rg32f            , gl::Pixel_format::rg             , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::rgb8             , gl::Pixel_format::rgb            , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rgb8_snorm       , gl::Pixel_format::rgb            , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::srgb8            , gl::Pixel_format::rgb            , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rgb16f           , gl::Pixel_format::rgb            , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::r11f_g11f_b10f   , gl::Pixel_format::rgb            , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::rgba8            , gl::Pixel_format::rgba           , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::srgb8_alpha8     , gl::Pixel_format::rgba           , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rgba8_snorm      , gl::Pixel_format::rgba           , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::rgba32f          , gl::Pixel_format::rgba           , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::rgba16f          , gl::Pixel_format::rgba           , gl::Pixel_type::float_         },
    InternalFormatFormatType{ gl::Internal_format::r8ui             , gl::Pixel_format::red_integer    , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::r16ui            , gl::Pixel_format::red_integer    , gl::Pixel_type::unsigned_short },
    InternalFormatFormatType{ gl::Internal_format::r32ui            , gl::Pixel_format::red_integer    , gl::Pixel_type::unsigned_int   },
    InternalFormatFormatType{ gl::Internal_format::r8i              , gl::Pixel_format::red_integer    , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::r16i             , gl::Pixel_format::red_integer    , gl::Pixel_type::short_         },
    InternalFormatFormatType{ gl::Internal_format::r32i             , gl::Pixel_format::red_integer    , gl::Pixel_type::int_           },
    InternalFormatFormatType{ gl::Internal_format::rg8ui            , gl::Pixel_format::rg_integer     , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rg16ui           , gl::Pixel_format::rg_integer     , gl::Pixel_type::unsigned_short },
    InternalFormatFormatType{ gl::Internal_format::rg32ui           , gl::Pixel_format::rg_integer     , gl::Pixel_type::unsigned_int   },
    InternalFormatFormatType{ gl::Internal_format::rg8i             , gl::Pixel_format::rg_integer     , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::rg16i            , gl::Pixel_format::rg_integer     , gl::Pixel_type::short_         },
    InternalFormatFormatType{ gl::Internal_format::rg32i            , gl::Pixel_format::rg_integer     , gl::Pixel_type::int_           },
    InternalFormatFormatType{ gl::Internal_format::rgb8ui           , gl::Pixel_format::rgb_integer    , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rgb16ui          , gl::Pixel_format::rgb_integer    , gl::Pixel_type::unsigned_short },
    InternalFormatFormatType{ gl::Internal_format::rgb32ui          , gl::Pixel_format::rgb_integer    , gl::Pixel_type::unsigned_int   },
    InternalFormatFormatType{ gl::Internal_format::rgb8i            , gl::Pixel_format::rgb_integer    , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::rgb16i           , gl::Pixel_format::rgb_integer    , gl::Pixel_type::short_         },
    InternalFormatFormatType{ gl::Internal_format::rgb32i           , gl::Pixel_format::rgb_integer    , gl::Pixel_type::int_           },
    InternalFormatFormatType{ gl::Internal_format::rgba8ui          , gl::Pixel_format::rgba_integer   , gl::Pixel_type::unsigned_byte  },
    InternalFormatFormatType{ gl::Internal_format::rgba16ui         , gl::Pixel_format::rgba_integer   , gl::Pixel_type::unsigned_short },
    InternalFormatFormatType{ gl::Internal_format::rgba32ui         , gl::Pixel_format::rgba_integer   , gl::Pixel_type::unsigned_int   },
    InternalFormatFormatType{ gl::Internal_format::rgba8i           , gl::Pixel_format::rgba_integer   , gl::Pixel_type::byte           },
    InternalFormatFormatType{ gl::Internal_format::rgba16i          , gl::Pixel_format::rgba_integer   , gl::Pixel_type::short_         },
    InternalFormatFormatType{ gl::Internal_format::rgba32i          , gl::Pixel_format::rgba_integer   , gl::Pixel_type::int_           },
    InternalFormatFormatType{ gl::Internal_format::depth_component16, gl::Pixel_format::depth_component, gl::Pixel_type::unsigned_short },
    InternalFormatFormatType{ gl::Internal_format::depth_component16, gl::Pixel_format::depth_component, gl::Pixel_type::unsigned_int   }
};

auto get_upload_pixel_byte_count(
    const gl::Internal_format internalformat
)-> size_t
{
    for (const auto& entry : INTERNAL_FORMAT_INFO)
    {
        if (entry.internal_format == internalformat)
        {
            // For now, there are no packed entries
            return component_count(entry.format) * byte_count(entry.type);
        }
    }
    ERHE_FATAL("Bad internal format");
}

auto get_format_and_type(
    const gl::Internal_format internalformat,
    gl::Pixel_format&         format,
    gl::Pixel_type&           type
) -> bool
{
    for (const auto& entry : INTERNAL_FORMAT_INFO)
    {
        if (entry.internal_format == internalformat)
        {
            format = entry.format;
            type   = entry.type;
            return true;
        }
    }
    return false;
}

auto Texture::storage_dimensions(const gl::Texture_target target) -> int
{
    switch (target)
    {
        //using enum gl::Texture_target;
        case gl::Texture_target::texture_buffer:
        {
            return 0;
        }

        case gl::Texture_target::texture_1d:
        {
            return 1;
        }

        case gl::Texture_target::texture_1d_array:
        case gl::Texture_target::texture_2d:
        case gl::Texture_target::texture_2d_multisample:
        case gl::Texture_target::texture_rectangle:
        case gl::Texture_target::texture_cube_map:
        {
            return 2;
        }

        case gl::Texture_target::texture_2d_array:
        case gl::Texture_target::texture_2d_multisample_array:
        case gl::Texture_target::texture_3d:
        case gl::Texture_target::texture_cube_map_array:
        {
            return 3;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

auto Texture::mipmap_dimensions(const gl::Texture_target target) -> int
{
    switch (target)
    {
        //using enum gl::Texture_target;
        case gl::Texture_target::texture_buffer:
        {
            return 0;
        }

        case gl::Texture_target::texture_1d:
        case gl::Texture_target::texture_1d_array:
        {
            return 1;
        }

        case gl::Texture_target::texture_rectangle:
        case gl::Texture_target::texture_cube_map:
        case gl::Texture_target::texture_2d:
        case gl::Texture_target::texture_2d_multisample:
        case gl::Texture_target::texture_2d_array:
        case gl::Texture_target::texture_2d_multisample_array:
        {
            return 2;
        }

        case gl::Texture_target::texture_3d:
        case gl::Texture_target::texture_cube_map_array:
        {
            return 3;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

Texture::~Texture() noexcept
{
    log_texture->trace("Deleting texture {} {}", gl_name(), m_debug_label);
}

Texture::Texture(Texture&& other) noexcept
    : m_handle         {std::move(other.m_handle)}
    , m_debug_label    {std::move(other.m_debug_label)}
    , m_target         {other.m_target}
    , m_internal_format{other.m_internal_format}
    , m_sample_count   {other.m_sample_count}
    , m_level_count    {other.m_level_count}
    , m_width          {other.m_width}
    , m_height         {other.m_height}
    , m_depth          {other.m_depth}
    , m_buffer         {other.m_buffer}
{
}

auto Texture::operator=(Texture&& other) noexcept -> Texture&
{
    m_handle          = std::move(other.m_handle);
    m_debug_label     = other.m_debug_label;
    m_target          = other.m_target;
    m_internal_format = other.m_internal_format;
    m_sample_count    = other.m_sample_count;
    m_level_count     = other.m_level_count;
    m_width           = other.m_width;
    m_height          = other.m_height;
    m_depth           = other.m_depth;
    m_buffer          = other.m_buffer;
    return *this;
}

auto Texture::gl_name() const -> GLuint
{
    return m_handle.gl_name();
}

auto Texture::get_handle() const -> uint64_t
{
    return gl::get_texture_handle_arb(m_handle.gl_name());
}

auto get_handle(
    const Texture& texture,
    const Sampler& sampler
) -> uint64_t
{
    if (erhe::graphics::Instance::info.use_bindless_texture)
    {
        return gl::get_texture_sampler_handle_arb(
            texture.gl_name(),
            sampler.gl_name()
        );
    }
    else
    {
        const uint64_t texture_name  = static_cast<uint64_t>(texture.gl_name());
        const uint64_t sampler_name  = static_cast<uint64_t>(sampler.gl_name());
        const uint64_t handle        = texture_name | (sampler_name << 32);
        return handle;
    }
}

auto get_texture_from_handle(uint64_t handle) -> GLuint
{
    return static_cast<GLuint>(handle & 0xffffffffu);
}

auto get_sampler_from_handle(uint64_t handle) -> GLuint
{
    return static_cast<GLuint>((handle & 0xffffffff00000000u) >> 32);
}

auto Texture::size_level_count(int size) -> int
{
    int level_count = size > 0 ? 1 : 0;

    while (size > 1)
    {
        size = size / 2;
        ++level_count;
    }
    return level_count;
}

auto Texture_create_info::calculate_level_count(
    const int width,
    const int height,
    const int depth
) -> int
{
    const auto x_level_count = Texture::size_level_count(width);
    const auto y_level_count = Texture::size_level_count(height);
    const auto z_level_count = Texture::size_level_count(depth);
    return std::max(std::max(x_level_count, y_level_count), z_level_count);
}

auto Texture_create_info::calculate_level_count() const -> int
{
    const auto dimensions = Texture::mipmap_dimensions(target);

    if (dimensions >= 1)
    {
        if (width == 0)
        {
            ERHE_FATAL("zero texture width");
        }
    }

    if (dimensions >= 2)
    {
        if (height == 0)
        {
            ERHE_FATAL("zero texture height");
        }
    }

    if (dimensions == 3)
    {
        if (depth == 0)
        {
            ERHE_FATAL("zero texture depth");
        }
    }

    return use_mipmaps
        ? calculate_level_count(
            width,
            (dimensions >= 2) ? height : 0,
            (dimensions >= 3) ? depth : 0
        )
        : 1;
}

Texture::Texture(const Create_info& create_info)
    : m_handle                {create_info.target, create_info.wrap_texture_name}
    , m_target                {create_info.target}
    , m_internal_format       {create_info.internal_format}
    , m_fixed_sample_locations{create_info.fixed_sample_locations}
    , m_sample_count          {create_info.sample_count}
    , m_level_count           {
        (create_info.level_count != 0)
            ? create_info.level_count
            : create_info.calculate_level_count()
    }
    , m_width                 {create_info.width}
    , m_height                {create_info.height}
    , m_depth                 {create_info.depth}
    , m_buffer                {create_info.buffer}
{
    log_texture->trace("Created texture {} {}x{} {}", gl_name(), m_width, m_height, gl::c_str(m_internal_format));

    const auto dimensions = storage_dimensions(m_target);

    if (create_info.sparse && Instance::info.use_sparse_texture)
    {
        gl::texture_parameter_i(m_handle.gl_name(), gl::Texture_parameter_name::texture_sparse_arb, GL_TRUE);
        m_is_sparse = true;
    }

    if (create_info.wrap_texture_name != 0)
    {
        // Current limitation
        ERHE_VERIFY(create_info.target == gl::Texture_target::texture_2d);

        //// Store original texture 2d binding
        //GLint original_texture_2d_binding{0};
        //gl::get_integer_v(gl::Get_p_name::texture_binding_2d, &original_texture_2d_binding);
        //gl::bind_texture(gl::Texture_target::texture_2d, gl_name());
        ERHE_VERIFY(gl::is_texture(gl_name()));
        GLint max_framebuffer_width {0};
        GLint max_framebuffer_height{0};
        gl::get_integer_v(gl::Get_p_name::max_framebuffer_width, &max_framebuffer_width);
        gl::get_integer_v(gl::Get_p_name::max_framebuffer_width, &max_framebuffer_height);
        ERHE_VERIFY(create_info.width  <= max_framebuffer_width);
        ERHE_VERIFY(create_info.height <= max_framebuffer_height);
        GLint width   {0};
        GLint height  {0};
        GLint depth   {0};
        GLint samples {0};
        GLint fixed_sample_locations{0};
        GLint internal_format_i {0};
        gl::get_texture_level_parameter_iv(gl_name(), 0, gl::Get_texture_parameter::texture_width,  &width);
        gl::get_texture_level_parameter_iv(gl_name(), 0, gl::Get_texture_parameter::texture_height, &height);
        gl::get_texture_level_parameter_iv(gl_name(), 0, static_cast<gl::Get_texture_parameter>(GL_TEXTURE_DEPTH),                  &depth);
        gl::get_texture_level_parameter_iv(gl_name(), 0, static_cast<gl::Get_texture_parameter>(GL_TEXTURE_SAMPLES),                &samples);
        gl::get_texture_level_parameter_iv(gl_name(), 0, static_cast<gl::Get_texture_parameter>(GL_TEXTURE_FIXED_SAMPLE_LOCATIONS), &fixed_sample_locations);
        gl::get_texture_level_parameter_iv(gl_name(), 0, static_cast<gl::Get_texture_parameter>(GL_TEXTURE_INTERNAL_FORMAT),        &internal_format_i);
        gl::Internal_format internal_format = static_cast<gl::Internal_format>(internal_format_i);
        ERHE_VERIFY(width == create_info.width);
        ERHE_VERIFY(height == create_info.height);
        m_width           = width;
        m_height          = height;
        m_depth           = depth;
        m_sample_count    = samples;
        m_internal_format = internal_format;
        GLint target_i          {0};
        GLint immutable_format_i{0};
        GLint immutable_levels  {0};
        gl::get_texture_parameter_iv(gl_name(), static_cast<gl::Get_texture_parameter>(GL_TEXTURE_TARGET),           &target_i);
        gl::Texture_target texture_target = static_cast<gl::Texture_target>(target_i);
        m_target       = texture_target;
        gl::get_texture_parameter_iv(gl_name(), static_cast<gl::Get_texture_parameter>(GL_TEXTURE_IMMUTABLE_FORMAT), &immutable_format_i);
        gl::get_texture_parameter_iv(gl_name(), static_cast<gl::Get_texture_parameter>(GL_TEXTURE_IMMUTABLE_LEVELS), &immutable_levels);
        //gl::Internal_format immutable_internal_format = static_cast<gl::Internal_format>(immutable_format_i);
        gl::texture_parameter_i(gl_name(), gl::Texture_parameter_name::texture_min_filter, GL_NEAREST);
        gl::texture_parameter_i(gl_name(), gl::Texture_parameter_name::texture_mag_filter, GL_NEAREST);

        //// Restore original texture 2d bionding
        //gl::bind_texture(gl::Texture_target::texture_2d, original_texture_2d_binding);
        return;
    }

    switch (dimensions)
    {
        case 0:
        {
            ERHE_VERIFY(m_buffer != nullptr);
            ERHE_VERIFY(m_sample_count == 0);
            gl::texture_buffer(gl_name(), m_internal_format, m_buffer->gl_name());
            break;
        }

        case 1:
        {
            ERHE_VERIFY(m_sample_count == 0);
            gl::texture_storage_1d(gl_name(), m_level_count, m_internal_format, m_width);
            break;
        }

        case 2:
        {
            if (m_sample_count == 0)
            {
                gl::texture_storage_2d(
                    gl_name(),
                    m_level_count,
                    m_internal_format,
                    m_width,
                    m_height
                );
            }
            else
            {
                gl::texture_storage_2d_multisample(
                    gl_name(),
                    m_sample_count,
                    m_internal_format,
                    m_width,
                    m_height,
                    m_fixed_sample_locations
                        ? GL_TRUE
                        : GL_FALSE
                );
            }
            break;
        }

        case 3:
        {
            gl::texture_storage_3d(
                gl_name(),
                m_level_count,
                m_internal_format,
                m_width,
                m_height,
                m_depth
            );
            break;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

auto Texture::is_sparse() const -> bool
{
    return m_is_sparse;
}

auto Texture::get_sparse_tile_size() const -> Tile_size
{
    if (!m_is_sparse)
    {
        return Tile_size{0, 0, 0};
    }

    const auto i = Instance::sparse_tile_sizes.find(m_internal_format);
    if (i == Instance::sparse_tile_sizes.end())
    {
        return Tile_size{0, 0, 0};
    }
    return i->second;
}

void Texture::upload(
    const gl::Internal_format internal_format,
    const int                 width,
    const int                 height,
    const int                 depth)
{
    Expects(internal_format == m_internal_format);
    Expects(width  >= 1);
    Expects(height >= 1);
    Expects(width  <= m_width);
    Expects(height <= m_height);
    Expects(m_sample_count == 0);

    gl::Pixel_format format;
    gl::Pixel_type   type;
    ERHE_VERIFY(get_format_and_type(m_internal_format, format, type));
    switch (storage_dimensions(m_target))
    {
        case 1:
        {
            gl::texture_sub_image_1d(gl_name(), 0, 0, width, format, type, nullptr);
            break;
        }

        case 2:
        {
            gl::texture_sub_image_2d(gl_name(), 0, 0, 0, width, height, format, type, nullptr);
            break;
        }

        case 3:
        {
            gl::texture_sub_image_3d(gl_name(), 0, 0, 0, 0, width, height, depth, format, type, nullptr);
            break;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

void Texture::upload(
    const gl::Internal_format        internal_format,
    const gsl::span<const std::byte> data,
    const int                        width,
    const int                        height,
    const int                        depth,
    const int                        level,
    const int                        x,
    const int                        y,
    const int                        z
)
{
    Expects(internal_format == m_internal_format);
    Expects(width  >= 1);
    Expects(height >= 1);
    Expects(width  <= m_width);
    Expects(height <= m_height);

    gl::Pixel_format format;
    gl::Pixel_type   type;
    ERHE_VERIFY(get_format_and_type(m_internal_format, format, type));

    const auto row_stride = width * get_upload_pixel_byte_count(internal_format);
    const auto byte_count = row_stride * height;
    Expects(data.size_bytes() >= byte_count);
    const auto* data_pointer = reinterpret_cast<const void*>(data.data());

    switch (storage_dimensions(m_target))
    {
        case 1:
        {
            gl::texture_sub_image_1d(gl_name(), level, x, width, format, type, data_pointer);
            break;
        }

        case 2:
        {
            gl::texture_sub_image_2d(gl_name(), level, x, y, width, height, format, type, data_pointer);
            break;
        }

        case 3:
        {
            gl::texture_sub_image_3d(gl_name(), level, x, y, z, width, height, depth, format, type, data_pointer);
            break;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

void Texture::upload_subimage(
    const gl::Internal_format        internal_format,
    const gsl::span<const std::byte> data,
    const int                        src_row_length,
    const int                        src_x,
    const int                        src_y,
    const int                        width,
    const int                        height,
    const int                        level,
    const int                        x,
    const int                        y,
    const int                        z
)
{
    Expects(internal_format == m_internal_format);
    Expects(width  >= 1);
    Expects(height >= 1);
    Expects(width  <= m_width);
    Expects(height <= m_height);

    gl::Pixel_format format;
    gl::Pixel_type   type;
    ERHE_VERIFY(get_format_and_type(m_internal_format, format, type));

    const auto pixel_stride = get_upload_pixel_byte_count(internal_format);;
    const auto row_stride   = src_row_length * pixel_stride;
    const auto byte_count   = row_stride * height;
    Expects(data.size_bytes() >= byte_count);
    const std::size_t src_x_offset = src_x * pixel_stride;
    const std::size_t src_y_offset = src_y * row_stride;
    const char* data_pointer =
        reinterpret_cast<const char*>(data.data())
        + src_x_offset
        + src_y_offset;
    gl::pixel_store_i(gl::Pixel_store_parameter::unpack_row_length, src_row_length);

    switch (storage_dimensions(m_target))
    {
        case 1:
        {
            gl::texture_sub_image_1d(gl_name(), level, x, width, format, type, data_pointer);
            break;
        }

        case 2:
        {
            gl::texture_sub_image_2d(gl_name(), level, x, y, width, height, format, type, data_pointer);
            break;
        }

        case 3:
        {
            gl::texture_sub_image_3d(gl_name(), level, x, y, z, width, height, 1, format, type, data_pointer);
            break;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
    gl::pixel_store_i(gl::Pixel_store_parameter::unpack_row_length, 0);
}

void Texture::set_debug_label(const std::string& value)
{
    log_texture->trace("Texture {} name set to {}", gl_name(), value);
    m_debug_label = "(T) " + value;
    gl::object_label(
        gl::Object_identifier::texture,
        gl_name(),
        static_cast<GLsizei>(m_debug_label.length()),
        m_debug_label.c_str()
    );
}

auto Texture::debug_label() const -> const std::string&
{
    return m_debug_label;
}

auto Texture::target() const -> gl::Texture_target
{
    return m_target;
}

auto Texture::is_layered() const -> bool
{
    switch (m_target)
    {
        //using enum gl::Texture_target;
        case gl::Texture_target::texture_buffer:
        case gl::Texture_target::texture_1d:
        case gl::Texture_target::texture_2d:
        case gl::Texture_target::texture_2d_multisample:
        case gl::Texture_target::texture_rectangle:
        case gl::Texture_target::texture_cube_map:
        case gl::Texture_target::texture_3d:
        {
            return false;
        }

        case gl::Texture_target::texture_1d_array:
        case gl::Texture_target::texture_2d_array:
        case gl::Texture_target::texture_2d_multisample_array:
        case gl::Texture_target::texture_cube_map_array:
        {
            return true;
        }

        default:
        {
            ERHE_FATAL("Bad texture target");
        }
    }
}

auto Texture::width() const -> int
{
    return m_width;
}

auto Texture::height() const -> int
{
    return m_height;
}

auto Texture::depth() const -> int
{
    return m_depth;
}

auto Texture::sample_count() const -> int
{
    return m_sample_count;
}

auto operator==(const Texture& lhs, const Texture& rhs) noexcept -> bool
{
    Expects(lhs.gl_name() != 0);
    Expects(rhs.gl_name() != 0);

    return lhs.gl_name() == rhs.gl_name();
}

auto operator!=(const Texture& lhs, const Texture& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto create_dummy_texture() -> std::shared_ptr<Texture>
{
    const erhe::graphics::Texture::Create_info create_info{
        .width  = 2,
        .height = 2
    };

    auto texture = std::make_shared<Texture>(create_info);
    texture->set_debug_label("dummy");
    const std::array<uint8_t, 16> dummy_pixel{
        0xee, 0x11, 0xdd, 0xff,
        0xcc, 0x11, 0xbb, 0xff,
        0xee, 0x11, 0xdd, 0xff,
        0xcc, 0x11, 0xbb, 0xff
    };
    const gsl::span<const std::byte> image_data{
        reinterpret_cast<const std::byte*>(&dummy_pixel[0]),
        dummy_pixel.size()
    };

    texture->upload(
        create_info.internal_format,
        image_data,
        create_info.width,
        create_info.height
    );

    return texture;
}

void Texture_unit_cache::reset(unsigned int base_texture_unit)
{
    m_base_texture_unit = base_texture_unit;
    m_texture_units.clear();
}

auto Texture_unit_cache::allocate_texture_unit(uint64_t handle) -> std::optional<std::size_t>
{
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    const GLuint texture_name = erhe::graphics::get_texture_from_handle(handle);
    const GLuint sampler_name = erhe::graphics::get_sampler_from_handle(handle);
#endif

    for (std::size_t texture_unit = 0, end = m_texture_units.size(); texture_unit < end; ++texture_unit)
    {
        if (m_texture_units[texture_unit] == handle)
        {
            SPDLOG_LOGGER_TRACE(
                log_texture,
                "cache hit texture unit {} for texture {}, sampler {}",
                texture_unit,
                texture_name,
                sampler_name
            );
            return texture_unit;
        }
    }

    const std::size_t result = m_texture_units.size();
    m_texture_units.push_back(handle);
    SPDLOG_LOGGER_TRACE(
        log_texture,
        "allocted texture unit {} for texture {}, sampler {}",
        result,
        texture_name,
        sampler_name
    );
    return result;
}

Texture_unit_cache s_texture_unit_cache;

auto Texture_unit_cache::bind(uint64_t fallback_handle) -> size_t
{
    const GLuint fallback_texture_name = erhe::graphics::get_texture_from_handle(fallback_handle);
    const GLuint fallback_sampler_name = erhe::graphics::get_sampler_from_handle(fallback_handle);

    GLuint i{};
    GLuint end = std::min(
        static_cast<GLuint>(m_texture_units.size()),
        static_cast<GLuint>(erhe::graphics::Instance::limits.max_texture_image_units)
    );

    for (i = 0; i < end; ++i)
    {
        const uint64_t handle       = m_texture_units[i];
        const GLuint   texture_name = erhe::graphics::get_texture_from_handle(handle);
        const GLuint   sampler_name = erhe::graphics::get_sampler_from_handle(handle);

        if (handle != 0)
        {
#if !defined(NDEBUG)
            if (gl::is_texture(texture_name) == GL_TRUE)
            {
                gl::bind_texture_unit(m_base_texture_unit + i, texture_name);
                SPDLOG_LOGGER_TRACE(
                    log_texture,
                    "texture unit {} + {} = {}: bound texture {}",
                    m_base_texture_unit,
                    i,
                    m_base_texture_unit + i,
                    texture_name
                );
            }
            else
            {
                log_texture->warn(
                    "texture unit {} + {} = {}: {} is not a texture",
                    m_base_texture_unit,
                    i,
                    m_base_texture_unit + i,
                    texture_name
                );
                gl::bind_texture_unit(i, erhe::graphics::get_texture_from_handle(fallback_handle));
            }

            if (
                (sampler_name == 0) ||
                (gl::is_sampler(sampler_name) == GL_TRUE)
            )
            {
                gl::bind_sampler(i, sampler_name);
                SPDLOG_LOGGER_TRACE(
                    log_texture,
                    "texture unit {} + {} = {}: bound sampler {}",
                    m_base_texture_unit,
                    i,
                    m_base_texture_unit + i,
                    sampler_name
                );
            }
            else
            {
                gl::bind_sampler(i, erhe::graphics::get_sampler_from_handle(fallback_handle));
                log_texture->warn(
                    "texture unit {} + {} = {}: {} is not a sampler",
                    m_base_texture_unit,
                    i,
                    m_base_texture_unit + i,
                    sampler_name
                );
            }
#else
            gl::bind_texture_unit(m_base_texture_unit + i, texture_name);
            gl::bind_sampler     (m_base_texture_unit + i, sampler_name);
#endif
        }
        else
        {
            gl::bind_texture_unit(m_base_texture_unit + i, fallback_texture_name);
            gl::bind_sampler     (m_base_texture_unit + i, fallback_sampler_name);
        }
    }
    return m_texture_units.size();
}

} // namespace erhe::graphics
