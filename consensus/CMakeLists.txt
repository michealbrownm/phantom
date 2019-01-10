#phantom consensus module CmakeLists.txt -- libphantom_consensus.a

set(LIB_PHANTOM_CONSENSUS phantom_consensus)
#automaticall get src file in specifical dir
aux_source_directory(${PHANTOM_SRC_DIR}/consensus/ CONSENSUS_SRC)

#generate static library file
add_library(${LIB_PHANTOM_CONSENSUS} ${CONSENSUS_SRC})

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_CONSENSUS}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

