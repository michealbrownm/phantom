#phantom common module CmakeLists.txt -- libphantom_common.a

set(LIB_phantom_COMMON phantom_common)
set(COMMON_SRC
    configure_base.cpp general.cpp storage.cpp private_key.cpp 
    daemon.cpp argument.cpp pb2json.cpp network.cpp data_secret_key.cpp key_store.cpp
)

#Generate static library files
add_library(${LIB_phantom_COMMON} ${COMMON_SRC}) 

#Specify compiling options for target objects
target_compile_options(${LIB_phantom_COMMON}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)
