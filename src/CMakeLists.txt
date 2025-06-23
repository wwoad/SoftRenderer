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

add_compile_options(-march=broadwell)

include_directories(${glm_PATH})

add_library(${SRCLIB} STATIC ${SRC}
)

target_link_libraries(${SRCLIB} PRIVATE
    Qt6::Core
    Qt6::Gui
)
