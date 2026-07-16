cmake_minimum_required(VERSION 3.16)
project(fractals)

# Set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

set(SOURCE_FILES
  src/main.c
)

# Create executable target as usual
add_executable(fractals WIN32 ${SOURCE_FILES})
target_link_libraries(fractals PRIVATE SDL3::SDL3)
