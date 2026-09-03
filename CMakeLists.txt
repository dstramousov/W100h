cmake_minimum_required(VERSION 3.28)

project(
    W100h
    VERSION 0.0.3
    DESCRIPTION "A small retro-styled AY/PT3 music player"
    LANGUAGES C CXX
)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(W100H_SDL_VERSION "3.4.10" CACHE STRING "SDL3 version used by W100h")
set(W100H_AYUMI_COMMIT "07c08b4874c359169e4a028edf73f046d8b763e2" CACHE STRING
    "Ayumi commit used by W100h")
set(W100H_PT3PLAYER_COMMIT "aaa5c321466d955b8a96509216a2e5cbc860d7b3" CACHE STRING
    "Volutar pt3player commit used by W100h")

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-${W100H_SDL_VERSION}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    ayumi
    GIT_REPOSITORY https://github.com/true-grue/ayumi.git
    GIT_TAG ${W100H_AYUMI_COMMIT}
)
FetchContent_MakeAvailable(ayumi)

add_library(w100h_ayumi STATIC "${ayumi_SOURCE_DIR}/ayumi.c")
target_compile_features(w100h_ayumi PRIVATE c_std_17)
target_include_directories(w100h_ayumi PUBLIC "${ayumi_SOURCE_DIR}")
if(UNIX AND NOT APPLE)
    target_link_libraries(w100h_ayumi PUBLIC m)
endif()

FetchContent_Declare(
    pt3player
    GIT_REPOSITORY https://github.com/Volutar/pt3player.git
    GIT_TAG ${W100H_PT3PLAYER_COMMIT}
)
FetchContent_MakeAvailable(pt3player)

add_library(w100h_pt3player STATIC "${pt3player_SOURCE_DIR}/pt3player.c")
target_compile_features(w100h_pt3player PRIVATE c_std_17)
target_include_directories(w100h_pt3player PUBLIC "${pt3player_SOURCE_DIR}")

add_executable(w100h)

target_sources(
    w100h
    PRIVATE
        src/main.cpp
        src/app/application.cpp
        src/audio/audio_output.cpp
        src/audio/audio_system.cpp
        src/audio/ay_chip.cpp
        src/audio/pt3_music.cpp
        src/audio/pt3_player.cpp
        src/core/command_line.cpp
        src/core/config.cpp
        src/library/music_library.cpp
        src/render/renderer.cpp
        src/ui/pixel_font.cpp
        src/ui/player_view.cpp
)

target_compile_features(w100h PRIVATE cxx_std_23)
target_include_directories(w100h PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(w100h PRIVATE SDL3::SDL3 w100h_ayumi w100h_pt3player)

if(MSVC)
    target_compile_options(w100h PRIVATE /W4)
else()
    target_compile_options(w100h PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(
    w100h
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include(CTest)

if(BUILD_TESTING)
    add_executable(w100h_ay_chip_test
        tests/ay_chip_test.cpp
        src/audio/ay_chip.cpp
    )
    target_compile_features(w100h_ay_chip_test PRIVATE cxx_std_23)
    target_include_directories(w100h_ay_chip_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(w100h_ay_chip_test PRIVATE w100h_ayumi)
    if(MSVC)
        target_compile_options(w100h_ay_chip_test PRIVATE /W4)
    else()
        target_compile_options(w100h_ay_chip_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME ay_chip COMMAND w100h_ay_chip_test)

    add_executable(w100h_pt3_music_test
        tests/pt3_music_test.cpp
        src/audio/pt3_music.cpp
    )
    target_compile_features(w100h_pt3_music_test PRIVATE cxx_std_23)
    target_include_directories(w100h_pt3_music_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(w100h_pt3_music_test PRIVATE /W4)
    else()
        target_compile_options(w100h_pt3_music_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME pt3_music COMMAND w100h_pt3_music_test)
    set_tests_properties(pt3_music PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(w100h_pt3_player_test
        tests/pt3_player_test.cpp
        tests/stubs/pt3player_stub.c
        src/audio/pt3_player.cpp
        src/audio/pt3_music.cpp
        src/audio/ay_chip.cpp
    )
    target_compile_features(w100h_pt3_player_test PRIVATE cxx_std_23)
    target_include_directories(w100h_pt3_player_test
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/stubs"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(w100h_pt3_player_test PRIVATE w100h_ayumi)
    if(MSVC)
        target_compile_options(w100h_pt3_player_test PRIVATE /W4)
    else()
        target_compile_options(w100h_pt3_player_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME pt3_player COMMAND w100h_pt3_player_test)
    set_tests_properties(pt3_player PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(w100h_config_test
        tests/config_test.cpp
        src/core/config.cpp
        src/library/music_library.cpp
    )
    target_compile_features(w100h_config_test PRIVATE cxx_std_23)
    target_include_directories(w100h_config_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(w100h_config_test PRIVATE /W4)
    else()
        target_compile_options(w100h_config_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME config COMMAND w100h_config_test)

    add_executable(w100h_command_line_test
        tests/command_line_test.cpp
        src/core/command_line.cpp
    )
    target_compile_features(w100h_command_line_test PRIVATE cxx_std_23)
    target_include_directories(w100h_command_line_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(w100h_command_line_test PRIVATE /W4)
    else()
        target_compile_options(w100h_command_line_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME command_line COMMAND w100h_command_line_test)

    add_executable(w100h_music_library_test
        tests/music_library_test.cpp
        src/library/music_library.cpp
    )
    target_compile_features(w100h_music_library_test PRIVATE cxx_std_23)
    target_include_directories(w100h_music_library_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(w100h_music_library_test PRIVATE /W4)
    else()
        target_compile_options(w100h_music_library_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME music_library COMMAND w100h_music_library_test)
endif()
