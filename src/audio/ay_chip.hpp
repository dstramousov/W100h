#pragma once

#include <cstdint>
#include <memory>
#include <span>

namespace w100h::audio {

/**
 * @brief Owns one AY-3-8910 emulator instance and exposes register-level access.
 *
 * The wrapper hides the Ayumi dependency from the rest of the engine and renders
 * interleaved stereo floating-point PCM samples.
 */
class AyChip final {
public:
    /**
     * @brief Creates and resets an AY chip for the requested output sample rate.
     *
     * @param sample_rate Output sample rate in Hz. Must be positive.
     * @throws std::invalid_argument If sample_rate is not positive.
     * @throws std::runtime_error If the emulator cannot be configured.
     */
    explicit AyChip(int sample_rate);

    /**
     * @brief Destroys the owned emulator state.
     */
    ~AyChip();

    AyChip(const AyChip&) = delete;
    AyChip& operator=(const AyChip&) = delete;
    AyChip(AyChip&&) = delete;
    AyChip& operator=(AyChip&&) = delete;

    /**
     * @brief Resets all AY registers and emulator history to a silent state.
     */
    void reset();

    /**
     * @brief Writes one AY register and immediately applies the resulting state.
     *
     * @param register_index AY register index in the inclusive range 0..13.
     * @param value Raw register value. Reserved high bits are masked where required.
     * @throws std::out_of_range If register_index is greater than 13.
     */
    void write_register(std::uint8_t register_index, std::uint8_t value);

    /**
     * @brief Reads the last value written to one AY register.
     *
     * @param register_index AY register index in the inclusive range 0..13.
     * @return Current masked register value.
     * @throws std::out_of_range If register_index is greater than 13.
     */
    [[nodiscard]] std::uint8_t read_register(std::uint8_t register_index) const;

    /**
     * @brief Renders interleaved stereo PCM into the supplied buffer.
     *
     * @param interleaved_stereo Output samples in L,R,L,R order.
     * @throws std::invalid_argument If the buffer contains an odd sample count.
     */
    void render(std::span<float> interleaved_stereo);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace w100h::audio
