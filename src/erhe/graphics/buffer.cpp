#include "erhe/graphics/buffer.hpp"
#include "erhe/gl/enum_string_functions.hpp"
#include "erhe/gl/enum_bit_mask_operators.hpp"
#include "erhe/gl/wrapper_enums.hpp"
#include "erhe/gl/wrapper_functions.hpp"
#include "erhe/graphics/configuration.hpp"
#include "erhe/graphics/graphics_log.hpp"
#include "erhe/toolkit/verify.hpp"

#include <fmt/format.h>

#include <sstream>
#include <vector>

namespace erhe::graphics
{

auto Buffer::gl_name() const noexcept -> unsigned int
{
    return m_handle.gl_name();
}

Buffer::Buffer(
    const gl::Buffer_target       target,
    const std::size_t             capacity_byte_count,
    const gl::Buffer_storage_mask storage_mask
) noexcept
    : m_target             {target}
    , m_capacity_byte_count{capacity_byte_count}
    , m_storage_mask       {storage_mask}
{
    log_buffer->trace(
        "Buffer::Buffer(target = {}, capacity_byte_count = {}, storage_mask = {}) name = {}",
        gl::c_str(target),
        capacity_byte_count,
        gl::to_string(storage_mask),
        gl_name()
    );

    ERHE_VERIFY(capacity_byte_count > 0);

    gl::named_buffer_storage(
        gl_name(),
        static_cast<GLintptr>(m_capacity_byte_count),
        nullptr,
        storage_mask
    );

    Ensures(gl_name() != 0);
    Ensures(m_capacity_byte_count > 0);
}

Buffer::Buffer(
    const gl::Buffer_target          target,
    const std::size_t                capacity_byte_count,
    const gl::Buffer_storage_mask    storage_mask,
    const gl::Map_buffer_access_mask map_buffer_access_mask
) noexcept
    : m_target             {target}
    , m_capacity_byte_count{capacity_byte_count}
    , m_storage_mask       {storage_mask}
{
    log_buffer->trace(
        "Buffer::Buffer(target = {}, capacity_byte_count = {}, storage_mask = {}, map_buffer_access_mask = {}) name = {}",
        gl::c_str(target),
        capacity_byte_count,
        gl::to_string(storage_mask),
        gl::to_string(map_buffer_access_mask),
        gl_name()
    );

    ERHE_VERIFY(capacity_byte_count > 0);

    gl::named_buffer_storage(
        gl_name(),
        static_cast<GLintptr>(m_capacity_byte_count),
        nullptr,
        storage_mask
    );

    map_bytes(0, capacity_byte_count, map_buffer_access_mask);

    Ensures(gl_name() != 0);
    Ensures(m_capacity_byte_count > 0);
}

Buffer::Buffer(
    const gl::Buffer_target          target,
    const std::size_t                capacity_byte_count,
    const gl::Buffer_storage_mask    storage_mask,
    const gl::Map_buffer_access_mask map_buffer_access_mask,
    const std::string_view           debug_label
) noexcept
    : m_target             {target}
    , m_capacity_byte_count{capacity_byte_count}
    , m_storage_mask       {storage_mask}
{
    log_buffer->trace(
        "Buffer::Buffer(target = {}, capacity_byte_count = {}, storage_mask = {}, map_buffer_access_mask = {}) name = {} {}",
        gl::c_str(target),
        capacity_byte_count,
        gl::to_string(storage_mask),
        gl::to_string(map_buffer_access_mask),
        gl_name(),
        debug_label
    );

    set_debug_label(debug_label);

    ERHE_VERIFY(capacity_byte_count > 0);

    gl::named_buffer_storage(
        gl_name(),
        static_cast<GLintptr>(m_capacity_byte_count),
        nullptr,
        storage_mask
    );

    map_bytes(0, capacity_byte_count, map_buffer_access_mask);

    Ensures(gl_name() != 0);
    Ensures(m_capacity_byte_count > 0);
}

Buffer::Buffer()
{
}

Buffer::~Buffer() noexcept
{
}

Buffer::Buffer(Buffer&& other) noexcept
{
    m_handle                 = std::move(other.m_handle);
    m_debug_label            = std::move(other.m_debug_label);
    m_target                 = other.m_target;
    m_capacity_byte_count    = other.m_capacity_byte_count;
    m_next_free_byte         = other.m_next_free_byte;
    m_storage_mask           = other.m_storage_mask;
    m_map                    = other.m_map;
    m_map_byte_offset        = other.m_map_byte_offset;
    m_map_buffer_access_mask = other.m_map_buffer_access_mask;
}

auto Buffer::operator=(Buffer&& other) noexcept -> Buffer&
{
    m_handle                 = std::move(other.m_handle);
    m_debug_label            = std::move(other.m_debug_label);
    m_target                 = other.m_target;
    m_capacity_byte_count    = other.m_capacity_byte_count;
    m_next_free_byte         = other.m_next_free_byte;
    m_storage_mask           = other.m_storage_mask;
    m_map                    = other.m_map;
    m_map_byte_offset        = other.m_map_byte_offset;
    m_map_buffer_access_mask = other.m_map_buffer_access_mask;
    return *this;
}

auto Buffer::map() const -> gsl::span<std::byte>
{
    return m_map;
}

auto Buffer::target() const noexcept -> gl::Buffer_target
{
    return m_target;
}

void Buffer::set_debug_label(const std::string_view label) noexcept
{
    m_debug_label = fmt::format("(B) {}", label);
    gl::object_label(
        gl::Object_identifier::buffer,
        gl_name(),
        static_cast<GLsizei>(m_debug_label.length()),
        m_debug_label.c_str()
    );
}

auto Buffer::debug_label() const noexcept -> const std::string&
{
    return m_debug_label;
}

auto Buffer::allocate_bytes(
    const std::size_t byte_count,
    const std::size_t alignment
) noexcept -> std::size_t
{
    const std::lock_guard<std::mutex> lock{m_allocate_mutex};

    while ((m_next_free_byte % alignment) != 0)
    {
        ++m_next_free_byte;
    }
    const auto offset = m_next_free_byte;
    m_next_free_byte += byte_count;
    ERHE_VERIFY(m_next_free_byte <= m_capacity_byte_count);

    log_buffer->trace("buffer {}: allocated {} bytes at offset {}", gl_name(), byte_count, offset);
    return offset;
}

auto Buffer::map_all_bytes(
    const gl::Map_buffer_access_mask access_mask
) noexcept -> gsl::span<std::byte>
{
    Expects(m_map.empty());
    Expects(gl_name() != 0);

    log_buffer->trace(
        "Buffer::map_all_bytes(access_mask = {}) target = {}, name = {}",
        gl::to_string(access_mask),
        gl::c_str(m_target),
        gl_name()
    );
    //const log::Indenter indenter;

    const std::size_t byte_count = m_capacity_byte_count;

    m_map_byte_offset = 0;
    m_map_buffer_access_mask = access_mask;

    auto* const map_pointer = reinterpret_cast<std::byte*>(
        gl::map_named_buffer_range(
            gl_name(),
            m_map_byte_offset,
            static_cast<GLsizeiptr>(byte_count),
            m_map_buffer_access_mask
        )
    );
    ERHE_VERIFY(map_pointer != nullptr);

    log_buffer->trace(
        ":m_map_byte_offset = {}, m_map_byte_count = {}, m_map_pointer = {}",
        m_map_byte_offset,
        byte_count,
        fmt::ptr(map_pointer)
    );

    m_map = gsl::span<std::byte>(map_pointer, byte_count);

    Ensures(!m_map.empty());

    return m_map;
}

auto Buffer::map_bytes(
    const std::size_t                byte_offset,
    const std::size_t                byte_count,
    const gl::Map_buffer_access_mask access_mask
) noexcept -> gsl::span<std::byte>
{
    ERHE_VERIFY(byte_count > 0);
    Expects(m_map.empty());
    Expects(gl_name() != 0);

    log_buffer->trace(
        "Buffer::map_bytes(byte_offset = {}, byte_count = {}, access_mask = {}) target = {}, name = {}",
        byte_offset,
        byte_count,
        gl::to_string(access_mask),
        gl::c_str(m_target),
        gl_name()
    );
    //const log::Indenter indenter;

    ERHE_VERIFY(byte_offset + byte_count <= m_capacity_byte_count);

    m_map_byte_offset = static_cast<GLsizeiptr>(byte_offset);
    m_map_buffer_access_mask = access_mask;

    auto* const map_pointer = reinterpret_cast<std::byte*>(
        gl::map_named_buffer_range(
            gl_name(),
            m_map_byte_offset,
            static_cast<GLsizeiptr>(byte_count),
            m_map_buffer_access_mask
        )
    );
    ERHE_VERIFY(map_pointer != nullptr);

    log_buffer->trace(
        ":m_map_byte_offset = {}, m_map_byte_count = {}, m_map_pointer = {}",
        m_map_byte_offset,
        byte_count,
        fmt::ptr(map_pointer)
    );

    m_map = gsl::span<std::byte>(map_pointer, byte_count);

    Ensures(!m_map.empty());

    return m_map;
}

void Buffer::unmap() noexcept
{
    Expects(!m_map.empty());
    Expects(gl_name() != 0);

    log_buffer->trace(
        "Buffer::unmap() target = {}, byte_offset = {}, byte_count = {}, pointer = {}, name = {}",
        gl::c_str(m_target),
        m_map_byte_offset,
        m_map.size(),
        reinterpret_cast<intptr_t>(m_map.data()),
        gl_name()
    );
    //const log::Indenter indented;
    //Log::set_text_color(erhe::log::Console_color::GREY);

    const auto res = gl::unmap_named_buffer(gl_name());
    ERHE_VERIFY(res == GL_TRUE);

    m_map_byte_offset = std::numeric_limits<std::size_t>::max();

    m_map = gsl::span<std::byte>();

    Ensures(m_map.empty());
}

void Buffer::flush_bytes(
    const std::size_t byte_offset,
    const std::size_t byte_count
) noexcept
{
    Expects((m_map_buffer_access_mask & gl::Map_buffer_access_mask::map_flush_explicit_bit) == gl::Map_buffer_access_mask::map_flush_explicit_bit);
    Expects(gl_name() != 0);

    // unmap will do flush
    ERHE_VERIFY(byte_offset + byte_count <= m_capacity_byte_count);

    log_buffer->trace(
       "Buffer::flush(byte_offset = {}, byte_count = {}) target = {}, m_mapped_ptr = {} name = {}",
        byte_offset,
        byte_count,
        gl::c_str(m_target),
        reinterpret_cast<intptr_t>(m_map.data()),
        gl_name()
    );

    gl::flush_mapped_named_buffer_range(
        gl_name(),
        static_cast<GLintptr>(byte_offset),
        static_cast<GLsizeiptr>(byte_count)
    );
}

void Buffer::dump() const noexcept
{
    Expects(gl_name() != 0);

    const std::size_t byte_count{m_capacity_byte_count};
    const std::size_t word_count{byte_count / sizeof(uint32_t)};

    int mapped{GL_FALSE};
    gl::get_named_buffer_parameter_iv(
        gl_name(),
        gl::Buffer_p_name::buffer_mapped,
        &mapped
    );

    uint32_t* data {nullptr};
    bool      unmap{false};
    if (mapped == GL_FALSE)
    {
        data = reinterpret_cast<uint32_t*>(
            gl::map_named_buffer_range(
                gl_name(),
                0,
                byte_count,
                gl::Map_buffer_access_mask::map_read_bit
            )
        );
        unmap = (data != nullptr);
    }

    std::vector<uint32_t> storage;
    if (data == nullptr)
    {
        // This happens if we already had buffer mapped
        storage.resize(word_count + 1);
        data = storage.data();
        gl::get_named_buffer_sub_data(gl_name(), 0, word_count * sizeof(uint32_t), data);
    }

    std::stringstream ss;
    for (std::size_t i = 0; i < word_count; ++i)
    {
        if (i % 16u == 0)
        {
            ss << fmt::format("%08x: ", static_cast<unsigned int>(i));
        }

        ss << fmt::format("%08x ", data[i]);

        if (i % 16u == 15u)
        {
            ss << "\n";
        }
    }
    log_buffer->trace("\n{}", ss.str());

    if (unmap)
    {
        gl::unmap_named_buffer(gl_name());
    }
}

void Buffer::flush_and_unmap_bytes(const std::size_t byte_count) noexcept
{
    Expects(gl_name() != 0);

    const bool flush_explicit =
        (m_map_buffer_access_mask & gl::Map_buffer_access_mask::map_flush_explicit_bit) == gl::Map_buffer_access_mask::map_flush_explicit_bit;

    log_buffer->trace("flush_and_unmap(byte_count = {}) name = {}", byte_count, gl_name());

    // If explicit is not selected, unmap will do full flush
    // so we do manual flush only if explicit is selected
    if (flush_explicit)
    {
        flush_bytes(0, byte_count);
    }

    unmap();
}

auto Buffer::free_capacity_bytes() const noexcept -> std::size_t
{
    return m_capacity_byte_count - m_next_free_byte;
}

auto Buffer::capacity_byte_count() const noexcept -> std::size_t
{
    return m_capacity_byte_count;
}

auto operator==(const Buffer& lhs, const Buffer& rhs) noexcept -> bool
{
    return lhs.gl_name() == rhs.gl_name();
}

auto operator!=(const Buffer& lhs, const Buffer& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

} // namespace erhe::graphics
