# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.20)
project(OpenEmulsion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RESOLVE_OPENFX_ROOT "C:/ProgramData/Blackmagic Design/DaVinci Resolve/Support/Developer/OpenFX" CACHE PATH "DaVinci Resolve OpenFX developer directory")
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "OpenEmulsion currently supports Windows x64 only.")
endif()

set(OFX_SUPPORT "${RESOLVE_OPENFX_ROOT}/Support")
set(OFX_API "${RESOLVE_OPENFX_ROOT}/OpenFX-1.4")
foreach(required IN ITEMS
    "${OFX_API}/include/ofxCore.h"
    "${OFX_SUPPORT}/include/ofxsImageEffect.h"
    "${OFX_SUPPORT}/include/ofxsProcessing.h"
    "${OFX_SUPPORT}/Library/ofxsCore.cpp")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "Resolve OpenFX developer files not found: ${required}\nSet RESOLVE_OPENFX_ROOT to the Developer/OpenFX directory.")
    endif()
endforeach()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/GrainMath.h")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/GrainMath.h" GRAIN_MATH_SOURCE)
configure_file(src/GrainOpenCL.h.in "${CMAKE_CURRENT_BINARY_DIR}/GrainOpenCL.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/ColorMath.h")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/ColorMath.h" COLOR_MATH_SOURCE)
configure_file(src/ColorOpenCL.h.in "${CMAKE_CURRENT_BINARY_DIR}/ColorOpenCL.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/FilmResponseMath.h")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/FilmResponseMath.h" FILM_RESPONSE_SOURCE)
configure_file(src/FilmResponseOpenCL.h.in "${CMAKE_CURRENT_BINARY_DIR}/FilmResponseOpenCL.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/HalationMath.h")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/HalationMath.h" HALATION_MATH_SOURCE)
configure_file(src/HalationOpenCL.h.in "${CMAKE_CURRENT_BINARY_DIR}/HalationOpenCL.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/BloomMath.h")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/BloomMath.h" BLOOM_MATH_SOURCE)
configure_file(src/BloomOpenCL.h.in "${CMAKE_CURRENT_BINARY_DIR}/BloomOpenCL.h" @ONLY)

add_library(OpenEmulsion SHARED
    src/OpenEmulsion.cpp
    src/OpenCLKernel.cpp
    "${OFX_SUPPORT}/Library/ofxsCore.cpp"
    "${OFX_SUPPORT}/Library/ofxsImageEffect.cpp"
    "${OFX_SUPPORT}/Library/ofxsInteract.cpp"
    "${OFX_SUPPORT}/Library/ofxsLog.cpp"
    "${OFX_SUPPORT}/Library/ofxsMultiThread.cpp"
    "${OFX_SUPPORT}/Library/ofxsParams.cpp"
    "${OFX_SUPPORT}/Library/ofxsProperty.cpp"
    "${OFX_SUPPORT}/Library/ofxsPropertyValidation.cpp"
)

target_include_directories(OpenEmulsion PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${OFX_API}/include"
    "${OFX_SUPPORT}/include"
)

target_compile_definitions(OpenEmulsion PRIVATE NOMINMAX)

if(MINGW)
    target_compile_options(OpenEmulsion PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_link_options(OpenEmulsion PRIVATE -static-libgcc -static-libstdc++ -static)
endif()

set_target_properties(OpenEmulsion PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Win64"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Win64"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Keep bundle layout stable with both single- and multi-configuration generators.
foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(OpenEmulsion PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Win64"
        LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Win64")
endforeach()

# Run even without a relink so resource-only edits reach both bundles.
set(PLUGIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(BUNDLE_RESOURCES "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Resources")
set(DIST_RESOURCES "${CMAKE_SOURCE_DIR}/../dist/OpenEmulsion.ofx.bundle/Contents/Resources")
add_custom_target(OpenEmulsionResources
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_RESOURCES}" "${DIST_RESOURCES}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/../LICENSE" "${CMAKE_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
        "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/../LICENSE" "${CMAKE_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
        "${CMAKE_SOURCE_DIR}/../dist/OpenEmulsion.ofx.bundle/Contents/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/../licenses"
        "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/Licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/../licenses"
        "${CMAKE_SOURCE_DIR}/../dist/OpenEmulsion.ofx.bundle/Contents/Licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PLUGIN_RESOURCES}/org.openemulsion.film.png"
        "${PLUGIN_RESOURCES}/org.openemulsion.film.svg"
        "${BUNDLE_RESOURCES}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PLUGIN_RESOURCES}/org.openemulsion.film.png"
        "${PLUGIN_RESOURCES}/org.openemulsion.film.svg"
        "${DIST_RESOURCES}/"
    DEPENDS "${PLUGIN_RESOURCES}/org.openemulsion.film.png" "${PLUGIN_RESOURCES}/org.openemulsion.film.svg"
    VERBATIM
)
add_dependencies(OpenEmulsion OpenEmulsionResources)

add_custom_command(TARGET OpenEmulsion POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/../dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/../LICENSE"
        "${CMAKE_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
        "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle/Contents/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/OpenEmulsion.ofx.bundle"
        "${CMAKE_SOURCE_DIR}/../dist/OpenEmulsion.ofx.bundle"
)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME PluginIconPackaging
        COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DBUILD_ROOT=${CMAKE_BINARY_DIR}"
            "-DDIST_ROOT=${CMAKE_SOURCE_DIR}/../dist"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginIconTests.cmake")
    add_executable(HalationTests tests/HalationTests.cpp src/OpenCLKernel.cpp)
    target_include_directories(HalationTests PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(HalationTests PRIVATE NOMINMAX)
    if(MINGW)
        target_link_options(HalationTests PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
    add_test(NAME HalationRegression COMMAND HalationTests)
    add_executable(ModuleControlStateTests tests/ModuleControlStateTests.cpp)
    target_include_directories(ModuleControlStateTests PRIVATE src)
    if(MINGW)
        target_link_options(ModuleControlStateTests PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
    add_test(NAME ModuleControlStateRegression COMMAND ModuleControlStateTests)
endif()
