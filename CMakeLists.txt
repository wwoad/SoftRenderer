cmake_minimum_required(VERSION 3.16)

project(SoftRenderer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

set(QT_LIBRARIES Qt6::Core Qt6::Gui)
set(PROJECT ${PROJECT_SOURCE_DIR})
set(SRCPATH ${PROJECT_SOURCE_DIR}/src)
set(glm_PATH ${PROJECT_SOURCE_DIR}/dependences)
set(assimp_PATH ${PROJECT_SOURCE_DIR}/dependences/assimp/include)
set(tbb_PATH ${PROJECT_SOURCE_DIR}/dependences/tbb/include)
set(LIBPATH ${PROJECT_SOURCE_DIR}/libs)
set(SRCLIB SoftRendererCore)

add_compile_options(-mavx -mfma)
add_compile_options(-mavx2 -mfma)


find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_subdirectory(src)

set(PROJECT_SOURCES
    Mat.h
    RenderWidget.h RenderWidget.cpp RenderWidget.ui
    Widget.h Widget.cpp Widget.ui
    Camera.h Camera.cpp
    Mesh.h Mesh.cpp
    Model.h Model.cpp
    BlinnPhongShader.h BlinnPhongShader.cpp
    main.cpp
)

include_directories(${PROJECT})
include_directories(${glm_PATH})
include_directories(${assimp_PATH})
include_directories(${SRCPATH})
include_directories(${tbb_PATH})


link_directories(${PROJECT_SOURCE_DIR}/libs)

qt_add_executable(SoftRenderer ${PROJECT_SOURCES}
)

target_compile_options(SoftRenderer PRIVATE -Wno-changes-meaning)
target_link_libraries(SoftRenderer PRIVATE assimp)
target_link_libraries(SoftRenderer PRIVATE tbb)
target_link_libraries(SoftRenderer PRIVATE SoftRendererCore Qt6::Widgets Qt6::Core Qt6::Gui)


