#phantom ledger module CmakeLists.txt -- libphantom_ledger.a

set(LIB_PHANTOM_LEDGER phantom_ledger)

#automaticall get src file in specifical dir
aux_source_directory(${PHANTOM_SRC_DIR}/ledger/ LEDGER_SRC)

#generate static library file
add_library(${LIB_PHANTOM_LEDGER} ${LEDGER_SRC})

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_LEDGER}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
