cmake_minimum_required(VERSION 3.15)
project(wfs_cppclient VERSION 1.0.0 LANGUAGES CXX)

# Import GNUInstallDirs to provide standardized installation paths
include(GNUInstallDirs)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Set MSVC specific compiler options
if(MSVC)
  add_compile_options(/utf-8)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Set vcpkg path
set(VCPKG_ROOT "C:/dev/vcpkg")
set(CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")
set(CMAKE_INCLUDE_PATH "${VCPKG_ROOT}/installed/x64-windows/include")

# Find dependencies
find_package(fmt CONFIG REQUIRED)
find_package(Thrift CONFIG REQUIRED)

# Source files
set(WFS_CLIENT_SRC
    src/wfs_client.cpp
    src/wfs_client_impl.cpp
    gen-cpp/WfsIface.cpp
    gen-cpp/wfs_types.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# WFS client library
add_library(wfs_client SHARED ${WFS_CLIENT_SRC})

# Set target output name and version
set_target_properties(wfs_client PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME WfsClient
)

target_include_directories(wfs_client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp
)

target_link_libraries(wfs_client
    PRIVATE
        thrift::thrift
        fmt::fmt
)


# Force generation of import library
set_target_properties(wfs_client PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

# Use .def file to control exports on Windows
if(MSVC)
    target_sources(wfs_client PRIVATE src/wfs_client.def)
endif()

# Add export macro definitions
target_compile_definitions(wfs_client
    PRIVATE
        WFS_CLIENT_EXPORTS
        WIN32_LEAN_AND_MEAN
        THRIFT_USE_STATIC_LIBS
        THRIFT_STATIC_DEFINE
    PUBLIC
        WFS_CLIENT_DLL
)

# Example application
add_executable(wfs_client_example 
    examples/wfs_client_sample.cpp
    gen-cpp/WfsIface.cpp
    gen-cpp/wfs_types.cpp
)

target_include_directories(wfs_client_example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp
)

# Add compile definitions to prevent duplicates and symbol exports
target_compile_definitions(wfs_client_example
    PRIVATE
        NOMINMAX 
        THRIFT_STATIC_DEFINE
        HAVE_GETTIMEOFDAY
)

target_link_libraries(wfs_client_example
    PRIVATE
        wfs_client
        thrift::thrift
        fmt::fmt
)
# Add linking options to handle Thrift symbol export issues
if(MSVC)
    # Add linking options for wfs_client
    target_link_options(wfs_client PRIVATE 
        "/IGNORE:4217"  # Ignore GlobalOutput symbol warning
    )
    
    # Add linking options for wfs_client_example
    target_link_options(wfs_client_example PRIVATE 
        "/IGNORE:4217"  # Ignore GlobalOutput symbol warning
    )
endif()

# Copy dependency DLLs to output directory
if(WIN32)
  # Check if DLL files exist
  set(THRIFT_DLL "${VCPKG_ROOT}/installed/x64-windows/bin/thrift.dll")
  set(FMT_DLL "${VCPKG_ROOT}/installed/x64-windows/bin/fmt.dll")
  
  if(EXISTS "${THRIFT_DLL}")
    add_custom_command(TARGET wfs_client_example POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying ${THRIFT_DLL} to $<TARGET_FILE_DIR:wfs_client_example>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${THRIFT_DLL}"
        $<TARGET_FILE_DIR:wfs_client_example>
    )
  else()
    message(WARNING "Thrift DLL not found at ${THRIFT_DLL}")
  endif()
  
  if(EXISTS "${FMT_DLL}")
    add_custom_command(TARGET wfs_client_example POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying ${FMT_DLL} to $<TARGET_FILE_DIR:wfs_client_example>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMT_DLL}"
        $<TARGET_FILE_DIR:wfs_client_example>
    )
  else()
    message(WARNING "fmt DLL not found at ${FMT_DLL}")
  endif()
endif()

# Set installation rules
install(TARGETS wfs_client
        EXPORT wfs_client-targets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY include/ DESTINATION include)

# Also install Thrift-generated header files to include
install(FILES
    gen-cpp/wfs_types.h
    DESTINATION include/wfs_client/gen-cpp
)

# Export configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wfs_client-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Create cmake configuration directory
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cmake")

# Configure path variables for config.cmake.in
set(PACKAGE_CMAKE_INSTALL_INCLUDEDIR include)
set(PACKAGE_CMAKE_INSTALL_LIBDIR lib)

# Configure file content
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/wfs_client-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/wfs_client-config.cmake"
    @ONLY
)

# Install CMake configuration files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/wfs_client-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wfs_client-config-version.cmake"
    DESTINATION lib/cmake/wfs_client
)

# Install export targets
install(EXPORT wfs_client-targets
    FILE wfs_client-targets.cmake
    NAMESPACE wfs::
    DESTINATION lib/cmake/wfs_client
)

# Output information
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "VCPKG_ROOT: ${VCPKG_ROOT}")
message(STATUS "thrift_VERSION: ${thrift_VERSION}")
message(STATUS "fmt_VERSION: ${fmt_VERSION}") 