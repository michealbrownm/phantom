#phantom glue module CmakeLists.txt -- libphantom_glue.a

set(LIB_PHANTOM_GLUE phantom_glue)

#Automatically get src files from the specified directory
aux_source_directory(${PHANTOM_SRC_DIR}/glue/ GLUE_SRC)

#Generate static library files
add_library(${LIB_PHANTOM_GLUE} ${GLUE_SRC})

#Specify compiling options for target objects
target_compile_options(${LIB_PHANTOM_GLUE}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
