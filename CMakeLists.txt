cmake_minimum_required(VERSION 3.21)

project(NickelUpdater LANGUAGES C CXX)

option(BUILD_TESTING "Build NickelUpdater unit tests" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Network)

execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE NICKELUPDATER_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT NICKELUPDATER_VERSION)
    set(NICKELUPDATER_VERSION dev)
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/NickelHook")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/NickelHook/nhplugin.json"
    "{\"Keys\":[\"nh.nickelupdater\"]}\n")

file(GLOB NICKELUPDATER_SOURCES CONFIGURE_DEPENDS
    "src/*.cc"
    "src/*.h"
    "NickelHook/*.c"
    "NickelHook/*.h"
)
add_library(nickelupdater SHARED
    ${NICKELUPDATER_SOURCES}
)
set_target_properties(nickelupdater PROPERTIES
    BUILD_RPATH "/usr/local/Kobo;/usr/local/Qt-5.2.1-arm/lib"
    INSTALL_RPATH "/usr/local/Kobo;/usr/local/Qt-5.2.1-arm/lib"
)
target_include_directories(nickelupdater PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/NickelHook"
    "${CMAKE_CURRENT_BINARY_DIR}/NickelHook"
)
target_compile_definitions(nickelupdater PRIVATE "NH_VERSION=\"${NICKELUPDATER_VERSION}\"")

set(NICKELUPDATER_WARNINGS
    -Wall -Wextra -Werror -Wformat=2
)
set(NICKELUPDATER_CXX_WARNINGS
    -Wno-missing-field-initializers
    -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-qual
    -Wredundant-decls -Wlogical-op -Wfloat-equal -Wundef
)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(nickelupdater PRIVATE
        -O2 -pthread ${NICKELUPDATER_WARNINGS}
        -Wno-error=unused-result
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(nickelupdater PRIVATE -O2 -pthread ${NICKELUPDATER_WARNINGS})
else()
    message(FATAL_ERROR "NickelUpdater requires a GNU or Clang C compiler")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND NICKELUPDATER_CXX_WARNINGS -Wlogical-op)
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "NickelUpdater requires a GNU or Clang C++ compiler")
endif()
target_compile_options(nickelupdater PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:-O2;-pthread;${NICKELUPDATER_WARNINGS};${NICKELUPDATER_CXX_WARNINGS}>"
)
if(UNIX)
    target_link_options(nickelupdater PRIVATE -pthread -Wl,--as-needed -Wl,--no-undefined)
endif()
target_link_libraries(nickelupdater PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    ${CMAKE_DL_LIBS}
)

add_custom_target(koboroot
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageKoboRoot.cmake"
    DEPENDS nickelupdater
    VERBATIM
)

if(BUILD_TESTING)
    include(CTest)
    find_package(Qt5 REQUIRED COMPONENTS Test)

    file(GLOB NICKELUPDATER_TEST_SOURCES CONFIGURE_DEPENDS
        "tests/*.cc"
        "tests/*.h"
    )
    add_executable(nickelupdater_tests
        ${NICKELUPDATER_TEST_SOURCES}
        src/Constants.cc
        src/Log.cc
        src/UserConfig.cc
    )
    target_include_directories(nickelupdater_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/NickelHook"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_options(nickelupdater_tests PRIVATE -Wno-deprecated-declarations)
    target_link_libraries(nickelupdater_tests PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME NickelUpdater COMMAND nickelupdater_tests)
endif()
