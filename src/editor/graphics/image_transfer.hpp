#pragma once

#include "erhe/components/components.hpp"
#include "erhe/graphics/gl_objects.hpp"

#include <gsl/span>

#include <memory>

namespace editor
{

class Buffer;

class Image_transfer
    : public erhe::components::Component
{
public:
    class Slot
    {
    public:
        Slot();

        [[nodiscard]] auto span_for(
            const int                 width,
            const int                 height,
            const gl::Internal_format internal_format
        ) -> gsl::span<std::byte>;

        [[nodiscard]] auto gl_name() -> unsigned int
        {
            return pbo.gl_name();
        }

        gsl::span<std::byte>      span;
        std::size_t               capacity{0};
        erhe::graphics::Gl_buffer pbo;
    };

    static constexpr std::string_view c_label{"erhe::graphics::ImageTransfer"};
    static constexpr uint32_t         hash = compiletime_xxhash::xxh32(c_label.data(), c_label.size(), {});

    Image_transfer ();
    ~Image_transfer() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return hash; }
    void declare_required_components() override;
    void initialize_component       () override;

    // Public API
    [[nodiscard]] auto get_slot() -> Slot&;

private:
    std::size_t                          m_index{0};
    std::unique_ptr<std::array<Slot, 4>> m_slots;
};

} // namespace editor
