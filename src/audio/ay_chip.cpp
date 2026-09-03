#include "audio/ay_chip.hpp"

extern "C" {
#include <ayumi.h>
}

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace w100h::audio {
namespace {

constexpr double kAyClockHz = 1'773'400.0;
constexpr double kChipOutputGain = 1.0 / 3.0;
constexpr std::size_t kRegisterCount = 14;

[[nodiscard]] std::uint8_t mask_register_value(std::uint8_t register_index, std::uint8_t value) {
    switch (register_index) {
        case 1:
        case 3:
        case 5:
        case 13:
            return static_cast<std::uint8_t>(value & 0x0F);
        case 6:
            return static_cast<std::uint8_t>(value & 0x1F);
        case 8:
        case 9:
        case 10:
            return static_cast<std::uint8_t>(value & 0x1F);
        default:
            return value;
    }
}

}  // namespace

struct AyChip::Impl {
    explicit Impl(int output_sample_rate) : sample_rate{output_sample_rate} { configure(); }

    void configure() {
        if (!ayumi_configure(&chip, 0, kAyClockHz, sample_rate)) {
            throw std::runtime_error{"Ayumi configuration failed for requested sample rate"};
        }
        ayumi_set_pan(&chip, 0, 0.25, 1);
        ayumi_set_pan(&chip, 1, 0.50, 1);
        ayumi_set_pan(&chip, 2, 0.75, 1);
        registers.fill(0);
    }

    void apply_tone(int channel) {
        const std::size_t low_index = static_cast<std::size_t>(channel) * 2;
        const int period = static_cast<int>(registers[low_index]) |
                           (static_cast<int>(registers[low_index + 1] & 0x0F) << 8);
        ayumi_set_tone(&chip, channel, period);
    }

    void apply_mixer(int channel) {
        const std::uint8_t mixer = registers[7];
        const bool tone_disabled = (mixer & (1U << channel)) != 0;
        const bool noise_disabled = (mixer & (1U << (channel + 3))) != 0;
        const bool envelope_enabled = (registers[8 + channel] & 0x10) != 0;
        ayumi_set_mixer(&chip, channel, tone_disabled ? 1 : 0, noise_disabled ? 1 : 0,
                        envelope_enabled ? 1 : 0);
    }

    void apply_envelope_period() {
        const int period = static_cast<int>(registers[11]) |
                           (static_cast<int>(registers[12]) << 8);
        ayumi_set_envelope(&chip, period);
    }

    ayumi chip{};
    std::array<std::uint8_t, kRegisterCount> registers{};
    int sample_rate = 0;
};

AyChip::AyChip(int sample_rate) {
    if (sample_rate <= 0) {
        throw std::invalid_argument{"AY sample rate must be positive"};
    }
    impl_ = std::make_unique<Impl>(sample_rate);
}

AyChip::~AyChip() = default;

void AyChip::reset() { impl_->configure(); }

void AyChip::write_register(std::uint8_t register_index, std::uint8_t value) {
    if (register_index >= kRegisterCount) {
        throw std::out_of_range{"AY register index must be in range 0..13"};
    }

    impl_->registers[register_index] = mask_register_value(register_index, value);

    switch (register_index) {
        case 0:
        case 1:
            impl_->apply_tone(0);
            break;
        case 2:
        case 3:
            impl_->apply_tone(1);
            break;
        case 4:
        case 5:
            impl_->apply_tone(2);
            break;
        case 6:
            ayumi_set_noise(&impl_->chip, impl_->registers[6]);
            break;
        case 7:
            impl_->apply_mixer(0);
            impl_->apply_mixer(1);
            impl_->apply_mixer(2);
            break;
        case 8:
        case 9:
        case 10: {
            const int channel = static_cast<int>(register_index - 8);
            ayumi_set_volume(&impl_->chip, channel, impl_->registers[register_index] & 0x0F);
            impl_->apply_mixer(channel);
            break;
        }
        case 11:
        case 12:
            impl_->apply_envelope_period();
            break;
        case 13:
            ayumi_set_envelope_shape(&impl_->chip, impl_->registers[13]);
            break;
        default:
            break;
    }
}

std::uint8_t AyChip::read_register(std::uint8_t register_index) const {
    if (register_index >= kRegisterCount) {
        throw std::out_of_range{"AY register index must be in range 0..13"};
    }
    return impl_->registers[register_index];
}

std::uint8_t AyChip::channel_visual_level(std::uint8_t channel) const {
    if (channel >= 3) {
        throw std::out_of_range{"AY channel index must be in range 0..2"};
    }

    const auto& state = impl_->chip.channels[channel];
    if (!state.e_on) {
        return static_cast<std::uint8_t>(std::clamp(state.volume, 0, 15));
    }

    const int envelope = std::clamp(impl_->chip.envelope, 0, 31);
    return static_cast<std::uint8_t>((envelope * 15 + 15) / 31);
}

bool AyChip::channel_noise_enabled(std::uint8_t channel) const {
    if (channel >= 3) {
        throw std::out_of_range{"AY channel index must be in range 0..2"};
    }
    return impl_->chip.channels[channel].n_off == 0;
}

bool AyChip::channel_envelope_enabled(std::uint8_t channel) const {
    if (channel >= 3) {
        throw std::out_of_range{"AY channel index must be in range 0..2"};
    }
    return impl_->chip.channels[channel].e_on != 0;
}

void AyChip::render(std::span<float> interleaved_stereo) {
    if ((interleaved_stereo.size() % 2U) != 0U) {
        throw std::invalid_argument{"AY render buffer must contain complete stereo frames"};
    }

    for (std::size_t sample = 0; sample < interleaved_stereo.size(); sample += 2) {
        ayumi_process(&impl_->chip);
        ayumi_remove_dc(&impl_->chip);
        interleaved_stereo[sample] = static_cast<float>(impl_->chip.left * kChipOutputGain);
        interleaved_stereo[sample + 1] = static_cast<float>(impl_->chip.right * kChipOutputGain);
    }
}

}  // namespace w100h::audio
