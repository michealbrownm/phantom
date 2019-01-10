#phantom common module CmakeLists.txt -- libphantom_common.a

set(LIB_PHANTOM_COMMON phantom_common)
set(COMMON_SRC
    configure_base.cpp general.cpp storage.cpp private_key.cpp 
    daemon.cpp argument.cpp pb2json.cpp network.cpp data_secret_key.cpp key_store.cpp
)

#generate static library file
add_library(${LIB_PHANTOM_COMMON} ${COMMON_SRC}) 

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_COMMON}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
