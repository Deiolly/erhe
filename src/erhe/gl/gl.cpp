#include "erhe/gl/gl.hpp"
#include "erhe/gl/enum_string_functions.hpp"
#include "erhe/gl/dynamic_load.hpp"
#include "erhe/log/log.hpp"
#include "erhe/toolkit/verify.hpp"

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <iostream>

#if !defined(WIN32)
# include <csignal>
#endif

namespace gl
{

std::shared_ptr<spdlog::logger> log_gl;

static bool enable_error_checking = true;

void initialize_logging()
{
    log_gl = erhe::log::make_logger("gl", spdlog::level::info);
}

void set_error_checking(const bool enable)
{
    enable_error_checking = enable;
}

void check_error()
{
    if (!enable_error_checking)
    {
        return;
    }

#if defined(ERHE_DLOAD_ALL_GL_SYMBOLS)
    auto error_code = static_cast<gl::Error_code>(gl::glGetError());
#else
    auto error_code = static_cast<gl::Error_code>(glGetError());
#endif

    if (error_code != gl::Error_code::no_error)
    {
        log_gl->error("{}", gl::c_str(error_code));
        //error_fmt(log_gl, "{}", gl::c_str(error_code));
#if defined(WIN32)
        DebugBreak();
#else
        raise(SIGTRAP);
#endif
    }
}

auto size_of_type(const gl::Draw_elements_type type) -> size_t
{
    switch (type)
    {
        //using enum gl::Draw_elements_type;
        case gl::Draw_elements_type::unsigned_byte:  return 1;
        case gl::Draw_elements_type::unsigned_short: return 2;
        case gl::Draw_elements_type::unsigned_int:   return 4;
        default:
            ERHE_FATAL("Bad draw elements index type");
    }
}

auto size_of_type(const gl::Vertex_attrib_type type) -> size_t
{
    switch (type)
    {
        //using enum gl::Vertex_attrib_type;
        case gl::Vertex_attrib_type::byte:
        case gl::Vertex_attrib_type::unsigned_byte:
        {
            return 1;
        }

        case gl::Vertex_attrib_type::half_float:
        case gl::Vertex_attrib_type::short_:
        case gl::Vertex_attrib_type::unsigned_short:
        {
            return 2;
        }

        case gl::Vertex_attrib_type::fixed:
        case gl::Vertex_attrib_type::float_:
        case gl::Vertex_attrib_type::int_:
        case gl::Vertex_attrib_type::int_2_10_10_10_rev:
        case gl::Vertex_attrib_type::unsigned_int:
        case gl::Vertex_attrib_type::unsigned_int_10f_11f_11f_rev:
        case gl::Vertex_attrib_type::unsigned_int_2_10_10_10_rev:
        {
            return 4;
        }

        case gl::Vertex_attrib_type::double_:
        {
            return 8;
        }

        default:
        {
            ERHE_FATAL("Bad vertex attribute type");
        }
    }
}

} // namespace gl
