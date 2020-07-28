cmake_minimum_required(VERSION 3.5)
project(stream_logger
    DESCRIPTION "Log stdin/stdout/stderr of subprocess"
    HOMEPAGE_URL "https://github.com/madebr/stream_logger"
    LANGUAGES CXX
)

find_package(Boost REQUIRED COMPONENTS filesystem program_options)
find_package(Threads REQUIRED)

add_executable(stream_logger
    stream_logger.cpp
)
target_link_libraries(stream_logger PRIVATE Boost::filesystem Boost::program_options Threads::Threads)
if(MSVC)
else()
    target_compile_options(stream_logger PRIVATE -Wall -Werror)
endif()
target_compile_definitions(stream_logger PRIVATE ASIO_DISABLE_THREADS)
set_property(TARGET stream_logger PROPERTY CXX_STANDARD 14)
