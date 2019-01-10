#phantom glue module CmakeLists.txt -- libphantom_glue.a

set(LIB_PHANTOM_GLUE phantom_glue)

#automaticall get src file in specifical dir
aux_source_directory(${PHANTOM_SRC_DIR}/glue/ GLUE_SRC)

#generate static library file
add_library(${LIB_PHANTOM_GLUE} ${GLUE_SRC})

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_GLUE}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
