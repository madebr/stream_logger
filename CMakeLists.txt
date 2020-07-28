cmake_minimum_required(VERSION 3.5)
project(stream_logger
    DESCRIPTION "Log stdin/stdout/stderr of subprocess"
    HOMEPAGE_URL "https://github.com/madebr/stream_logger"
    LANGUAGES C CXX
)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup(TARGETS)
endif()

find_package(Boost REQUIRED COMPONENTS filesystem program_options)
find_package(Threads REQUIRED)

include(CheckIncludeFile)
include(CheckSymbolExists)
check_include_file(unistd.h HAS_UNISTD_H)
check_symbol_exists(GetStdHandle "windows.h" HAS_GETSTDHANDLE)
configure_file(stream_logger_config.h.in stream_logger_config.h)

add_executable(stream_logger
    stream_logger.cpp
)
target_include_directories(stream_logger PRIVATE "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>")
target_link_libraries(stream_logger PRIVATE
    Boost::disable_autolinking
    Boost::filesystem
    Boost::program_options
    Threads::Threads
)
if(WIN32)
    target_compile_definitions(stream_logger PRIVATE _WIN32_WINNT=0x0601)
endif()
if(MSVC)
else()
    target_compile_options(stream_logger PRIVATE -Wall -Werror)
endif()
target_compile_definitions(stream_logger PRIVATE ASIO_DISABLE_THREADS)
set_property(TARGET stream_logger PROPERTY CXX_STANDARD 14)

install(TARGETS stream_logger)
