cmake_minimum_required(VERSION 3.21)
project(monologue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia DBus Concurrent Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse)
pkg_check_modules(TURBOJPEG REQUIRED IMPORTED_TARGET libturbojpeg)

# Everything but main.cpp and x264_fast_hook.cpp, shared by the app and the tests.
file(GLOB monologue_sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(REMOVE_ITEM monologue_sources ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/x264_fast_hook.cpp)
add_library(monologue_core STATIC ${monologue_sources})
target_include_directories(monologue_core PUBLIC src)
target_link_libraries(monologue_core PUBLIC
  Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::DBus Qt6::Concurrent
  PkgConfig::PULSE PkgConfig::TURBOJPEG ${CMAKE_DL_LIBS})
target_compile_options(monologue_core PUBLIC -Wall -Wextra)

add_executable(monologue src/main.cpp src/x264_fast_hook.cpp src/resources.qrc)
target_link_libraries(monologue PRIVATE monologue_core ${CMAKE_DL_LIBS})
set_property(TARGET monologue PROPERTY ENABLE_EXPORTS ON)

enable_testing()
add_executable(monologue_tests tests/backend_tests.cpp src/x264_fast_hook.cpp)
target_link_libraries(monologue_tests PRIVATE monologue_core Qt6::Test ${CMAKE_DL_LIBS})
set_property(TARGET monologue_tests PROPERTY ENABLE_EXPORTS ON)
add_test(NAME monologue_tests COMMAND monologue_tests)
set_tests_properties(monologue_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)

# Drives the real Main.qml offscreen against a fake backend, so it carries the QML.
add_executable(monologue_ui_tests tests/ui_tests.cpp src/x264_fast_hook.cpp src/resources.qrc)
target_link_libraries(monologue_ui_tests PRIVATE monologue_core Qt6::Test ${CMAKE_DL_LIBS})
set_property(TARGET monologue_ui_tests PROPERTY ENABLE_EXPORTS ON)
add_test(NAME monologue_ui_tests COMMAND monologue_ui_tests)
set_tests_properties(monologue_ui_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)

# Records from the real camera and microphone, so only bin/test-camera runs it.
add_executable(camera_check tests/camera_check.cpp src/x264_fast_hook.cpp)
target_link_libraries(camera_check PRIVATE monologue_core ${CMAKE_DL_LIBS})
set_property(TARGET camera_check PROPERTY ENABLE_EXPORTS ON)
