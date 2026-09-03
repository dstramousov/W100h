#include <SDL3/SDL_main.h>

#include <exception>
#include <iostream>

#include "app/application.hpp"

int main(int argc, char* argv[]) {
    try {
        w100h::app::Application application;
        return application.run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
