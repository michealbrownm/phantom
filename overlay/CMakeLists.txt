#phantom overlay module CmakeLists.txt -- libphantom_overlay.a

set(LIB_PHANTOM_OVERLAY phantom_overlay)
#automaticall get src file in specifical dir
aux_source_directory(${PHANTOM_SRC_DIR}/overlay/ OVERLAY_SRC)

#generate static library file
add_library(${LIB_PHANTOM_OVERLAY} ${OVERLAY_SRC})

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_OVERLAY}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
