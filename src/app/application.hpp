#pragma once

namespace w100h::app {

/** @brief Owns the W100h application lifecycle and main event loop. */
class Application final {
public:
    /**
     * @brief Runs the application.
     * @param argc Argument count from main().
     * @param argv Argument vector from main().
     * @return Process exit code.
     */
    [[nodiscard]] int run(int argc, char* argv[]);
};

}  // namespace w100h::app
