cmake_minimum_required(VERSION 3.20)
project(digx VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure ZWODEE Engine directory path
set(ZWODEE_DIR "c:/Users/PC/Documents/git/dexus1337/zwodee" CACHE PATH "Path to the Zwodee Engine root directory")

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
find_library(ZWODEE_LIB zwodee
    HINTS 
        "${ZWODEE_DIR}/out/build/windows-x64-debug/bin"
        "${ZWODEE_DIR}/out/build/linux-x64-debug/bin"
        "${ZWODEE_DIR}/bin"
        "${ZWODEE_DIR}/build"
    REQUIRED
)

# Grab all source files recursively
file(GLOB_RECURSE source_files "source/*.cpp")
set(app_resources "assets/resources.rc")

# Executable target
add_executable(digx ${source_files} ${app_resources})

target_include_directories(digx 
    PRIVATE include
    PRIVATE "${ZWODEE_DIR}/include"
)

target_link_libraries(digx 
    PRIVATE SDL3::SDL3
    PRIVATE ${ZWODEE_LIB}
)

# Set runtime output folder directly to the engine's build directory where DLLs reside
set_target_properties(digx PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ZWODEE_DIR}/out/build/windows-x64-debug/bin"
)

# Copy ZWODEE Shared Library next to executable so it runs out-of-the-box
if(WIN32)
    # Get the directory where ZWODEE_LIB is located
    get_filename_component(ZWODEE_LIB_DIR ${ZWODEE_LIB} DIRECTORY)
    
    add_custom_command(TARGET digx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${ZWODEE_DIR}/out/build/windows-x64-debug/bin/assets"
    )
endif()
