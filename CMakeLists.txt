cmake_minimum_required(VERSION 4.0)
project(helios-imgui LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_IFC_OUTPUT_DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_IFC_OUTPUT_DIRECTORY}")
endif()

set(HELIOS_ECS_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-ecs" CACHE PATH "Local path to helios-ecs repository")
set(HELIOS_ENGINE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-engine" CACHE PATH "Local path to helios-engine repository")
set(HELIOS_GLFW_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-glfw" CACHE PATH "Local path to helios-glfw repository")

set(_HELIOS_IMGUI_BUILD_TESTING "${BUILD_TESTING}")
set(_HELIOS_IMGUI_BUILD_BENCHMARKS "${BUILD_BENCHMARKS}")
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

if(NOT TARGET helios::ecs)
    if(EXISTS "${HELIOS_ECS_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ECS_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-ecs-build")
        message(STATUS "Using local helios-ecs: ${HELIOS_ECS_LOCAL_PATH}")
    else()
        FetchContent_Declare(helios_ecs
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-ecs.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_ecs)
    endif()
endif()

if(NOT TARGET helios::engine)
    if(EXISTS "${HELIOS_ENGINE_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ENGINE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-engine-build")
        message(STATUS "Using local helios-engine: ${HELIOS_ENGINE_LOCAL_PATH}")
    else()
        FetchContent_Declare(helios_engine
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-engine.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_engine)
    endif()
endif()

if(NOT TARGET helios::glfw)
    if(EXISTS "${HELIOS_GLFW_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_GLFW_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-glfw-build")
        message(STATUS "Using local helios-glfw: ${HELIOS_GLFW_LOCAL_PATH}")
    else()
        FetchContent_Declare(helios_glfw
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-glfw.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_glfw)
    endif()
endif()

set(BUILD_TESTING "${_HELIOS_IMGUI_BUILD_TESTING}" CACHE BOOL "" FORCE)
set(BUILD_BENCHMARKS "${_HELIOS_IMGUI_BUILD_BENCHMARKS}" CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(NOT TARGET thirdparty::glfw)
    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
    add_library(thirdparty::glfw ALIAS glfw)
endif()

if(NOT TARGET thirdparty::glad)
    FetchContent_Declare(glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.8
            SOURCE_SUBDIR cmake
    )
    FetchContent_MakeAvailable(glad)
    glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE LOADER API gl:core=4.6)
    add_library(thirdparty::glad ALIAS glad_gl_core_46)
endif()

if(NOT TARGET thirdparty::opengl)
    add_library(thirdparty_opengl INTERFACE)
    target_link_libraries(thirdparty_opengl INTERFACE thirdparty::glad thirdparty::glfw)
    add_library(thirdparty::opengl ALIAS thirdparty_opengl)
endif()

if(NOT TARGET imgui)
    FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC thirdparty::glfw thirdparty::glad)
endif()

if(NOT TARGET thirdparty::imgui)
    add_library(thirdparty::imgui ALIAS imgui)
endif()

FetchContent_GetProperties(imgui)
if(imgui_SOURCE_DIR AND TARGET imgui)
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui PUBLIC thirdparty::glfw thirdparty::glad)

    get_target_property(_HELIOS_IMGUI_IMGUI_SOURCES imgui SOURCES)
    if(NOT _HELIOS_IMGUI_IMGUI_SOURCES)
        set(_HELIOS_IMGUI_IMGUI_SOURCES "")
    endif()

    list(FIND _HELIOS_IMGUI_IMGUI_SOURCES "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp" _HELIOS_IMGUI_HAS_OPENGL3_BACKEND)
    if(_HELIOS_IMGUI_HAS_OPENGL3_BACKEND EQUAL -1 AND EXISTS "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
        target_sources(imgui PRIVATE "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
    endif()
endif()

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MODULE_INTERFACE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")

list(FILTER CPP_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")
list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")

option(HELIOS_IMGUI_ENABLE_LEGACY_CAMERA_WIDGET "Build the legacy CameraWidget module" OFF)
if(NOT HELIOS_IMGUI_ENABLE_LEGACY_CAMERA_WIDGET)
    list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/widgets/CameraWidget\\.ixx$")
endif()

add_library(helios_imgui STATIC)
add_library(helios::imgui ALIAS helios_imgui)
set_property(TARGET helios_imgui PROPERTY CXX_SCAN_FOR_MODULES ON)
set_target_properties(helios_imgui PROPERTIES EXPORT_NAME imgui)
target_compile_features(helios_imgui PUBLIC cxx_std_23)

target_sources(helios_imgui PRIVATE ${CPP_FILES})

if(MODULE_INTERFACE_FILES)
    target_sources(helios_imgui PUBLIC
            FILE_SET modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
            FILES ${MODULE_INTERFACE_FILES}
    )
endif()

target_link_libraries(helios_imgui
        PUBLIC
        $<BUILD_INTERFACE:helios::ecs>
        $<BUILD_INTERFACE:helios::engine>
        $<BUILD_INTERFACE:helios::glfw>
        $<INSTALL_INTERFACE:helios::ecs>
        $<INSTALL_INTERFACE:helios::engine>
        $<INSTALL_INTERFACE:helios::glfw>
        PRIVATE
        $<BUILD_INTERFACE:thirdparty::imgui>
        $<BUILD_INTERFACE:thirdparty::glfw>
        $<BUILD_INTERFACE:thirdparty::glad>
        $<BUILD_INTERFACE:thirdparty::opengl>
)

target_include_directories(helios_imgui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

set(HELIOS_IMGUI_ENABLE_PACKAGE_DEFAULT OFF)
option(HELIOS_IMGUI_ENABLE_PACKAGE "Enable install/export package files" ${HELIOS_IMGUI_ENABLE_PACKAGE_DEFAULT})

if(HELIOS_IMGUI_ENABLE_PACKAGE)
    install(TARGETS helios_imgui
            EXPORT helios-imguiTargets
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            FILE_SET modules DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    set(HELIOS_IMGUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/helios-imgui")

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helios-imguiConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-imguiConfig.cmake"
            INSTALL_DESTINATION "${HELIOS_IMGUI_INSTALL_CMAKEDIR}"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/helios-imguiConfigVersion.cmake"
            VERSION "0.1.0"
            COMPATIBILITY SameMajorVersion
    )

    install(EXPORT helios-imguiTargets
            FILE helios-imguiTargets.cmake
            NAMESPACE helios::
            DESTINATION "${HELIOS_IMGUI_INSTALL_CMAKEDIR}"
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/helios-imguiConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-imguiConfigVersion.cmake"
            DESTINATION "${HELIOS_IMGUI_INSTALL_CMAKEDIR}"
    )
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp")

    if(TEST_SOURCES)
        add_executable(helios-imgui-tests ${TEST_SOURCES})
        target_link_libraries(helios-imgui-tests PRIVATE helios_imgui GTest::gtest_main)

        include(GoogleTest)
        gtest_discover_tests(helios-imgui-tests)
    endif()
endif()




