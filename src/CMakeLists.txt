cmake_minimum_required(VERSION 3.16)

set(SRC
    Shader.h
    FunctionSIMD.h
    HelperFunction.h
    BasicDataStructure.h
    SRFrameBuffer.h SRFrameBuffer.cpp
    Texture.h Texture.cpp
    SRendererDevice.h SRendererDevice.cpp
    threadpool.h threadpool.cpp
)

set(LIBRARY_OUTPUT_PATH ${LIBPATH})

include_directories(${tbb_PATH})
include_directories(${glm_PATH})

add_compile_options(-march=broadwell)

add_library(${SRCLIB} STATIC ${SRC}
)
target_compile_options(${SRCLIB} PRIVATE -Wno-changes-meaning)

target_link_libraries(SoftRendererCore PRIVATE tbb)
target_link_libraries(${SRCLIB} PRIVATE
    Qt6::Core
    Qt6::Gui
)
