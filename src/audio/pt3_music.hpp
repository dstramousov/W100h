#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace w100h::audio {

/**
 * @brief Validated PT3 music resource, optionally containing a 2-chip TurboSound pair.
 *
 * File I/O and structural validation are performed before the resource reaches the
 * realtime audio callback. The stored byte buffer includes one trailing zero byte
 * for compatibility with the upstream PT3 decoder while payload_size() retains the
 * original file size.
 */
class Pt3Music final {
public:
    /**
     * @brief Loads and validates a PT3 or 02TS TurboSound file.
     *
     * @param path PT3 file path.
     * @return Validated PT3 resource.
     * @throws std::runtime_error If the file cannot be read or is malformed.
     */
    [[nodiscard]] static Pt3Music load(const std::filesystem::path& path);

    /**
     * @brief Validates PT3 bytes already present in memory.
     *
     * @param data PT3 or 02TS TurboSound payload.
     * @return Validated PT3 resource.
     * @throws std::runtime_error If the payload is malformed or unsupported.
     */
    [[nodiscard]] static Pt3Music parse(std::span<const std::uint8_t> data);

    /**
     * @brief Returns the number of AY chips required by the resource.
     */
    [[nodiscard]] std::size_t chip_count() const noexcept { return chip_count_; }

    /**
     * @brief Returns the original payload size, excluding the compatibility sentinel.
     */
    [[nodiscard]] std::size_t payload_size() const noexcept { return payload_size_; }

    /**
     * @brief Returns mutable bytes required by the upstream C PT3 decoder.
     *
     * The decoder only copies from this buffer; mutable access is exposed because its
     * legacy API does not accept const input.
     */
    [[nodiscard]] std::uint8_t* data() noexcept { return storage_.data(); }

private:
    Pt3Music(std::vector<std::uint8_t> storage, std::size_t payload_size,
             std::size_t chip_count);

    std::vector<std::uint8_t> storage_;
    std::size_t payload_size_ = 0;
    std::size_t chip_count_ = 0;
};

}  // namespace w100h::audio
