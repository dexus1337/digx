cmake_minimum_required(VERSION 3.20)
project(digx VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure ZWODEE Engine directory path
set(zwodee_dir $ENV{ZWODEE_ROOT})
if(NOT zwodee_dir)
    message(FATAL_ERROR "ZWODEE_ROOT environment variable is not set. Please set it to the path of zwodee engine.")
endif()


# Platform Detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(DIGX_PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(DIGX_PLATFORM_LINUX)
endif()

if(MSVC)
    add_compile_options(/wd4251)
    add_compile_options(/wd4996)
endif()

# Find SDL3
find_package(SDL3 CONFIG REQUIRED)

# Find ZWODEE Library
find_library(zwodee_lib zwodee
    HINTS 
        "${zwodee_dir}/out/build/windows-x64-debug/bin"
        "${zwodee_dir}/out/build/linux-x64-debug/bin"
    REQUIRED
)

# Grab all source files recursively
file(GLOB_RECURSE source_files "source/*.cpp")
set(app_resources "assets/resources.rc")

# Executable target
add_executable(digx ${source_files} ${app_resources})

target_include_directories(digx 
    PRIVATE include
    PRIVATE "${zwodee_dir}/include"
)

target_link_libraries(digx 
    PRIVATE SDL3::SDL3
    PRIVATE ${zwodee_lib}
)

# Copy ZWODEE Shared Library next to executable so it runs out-of-the-box
if(WIN32)
    # Set runtime output folder directly to the engine's build directory where DLLs reside
    set_target_properties(digx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${zwodee_dir}/out/build/windows-x64-debug/bin"
    )

    # Get the directory where zwodee_dir is located
    get_filename_component(zwodee_dir_DIR ${zwodee_dir} DIRECTORY)
    
    add_custom_command(TARGET digx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${zwodee_dir}/out/build/windows-x64-debug/bin/assets"
    )
else()
    # Set runtime output folder directly to the engine's build directory where DLLs reside
    set_target_properties(digx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${zwodee_dir}/out/build/linux-x64-debug/bin"
    )

    # Get the directory where zwodee_dir is located
    get_filename_component(zwodee_dir_DIR ${zwodee_dir} DIRECTORY)
    
    add_custom_command(TARGET digx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${zwodee_dir}/out/build/linux-x64-debug/bin/assets"
    )
endif()
