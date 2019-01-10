#phantom http module CmakeLists.txt -- libphantom_http.a

set(LIB_PHANTOM_HTTP phantom_http)
aux_source_directory(${PHANTOM_SRC_DIR}/3rd/http/ HTTP_SRC)

add_library(${LIB_PHANTOM_HTTP} ${HTTP_SRC})

target_compile_options(${LIB_PHANTOM_HTTP}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)
