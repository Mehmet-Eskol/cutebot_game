cmake_minimum_required(VERSION 3.16)
project(VoltAdventure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# --- Oyun mantığı (OpenGL'den bağımsız; test edilebilir) ---------------------
add_library(volt_core STATIC
    src/Level.cpp src/Level.h
    src/Player.cpp src/Player.h
    src/Game.cpp src/Game.h)
target_link_libraries(volt_core PUBLIC Qt6::Core Qt6::Gui)
target_include_directories(volt_core PUBLIC src)

# --- Oyun (OpenGL render + pencere) ------------------------------------------
qt_add_executable(volt
    src/main.cpp
    src/Mesh.cpp src/Mesh.h
    src/Renderer.cpp src/Renderer.h
    src/RobotModel.cpp src/RobotModel.h
    src/GameWidget.cpp src/GameWidget.h)
target_link_libraries(volt PRIVATE volt_core Qt6::Widgets Qt6::OpenGLWidgets)

# --- Başsız simülasyon testi (isteğe bağlı) ----------------------------------
option(VOLT_BUILD_TESTS "Seviye geçilebilirlik simülasyonunu derle" ON)
if(VOLT_BUILD_TESTS)
  add_executable(volt_sim tests/sim_test.cpp)
  target_link_libraries(volt_sim PRIVATE volt_core)
endif()
