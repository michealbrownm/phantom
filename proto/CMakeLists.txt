#phantom proto module CmakeLists.txt -- libphantom_proto.a

set(LIB_PHANTOM_PROTO phantom_proto)
set(PROTO_SRC
    cpp/common.pb.cc cpp/consensus.pb.cc cpp/chain.pb.cc
 	cpp/overlay.pb.cc cpp/merkeltrie.pb.cc cpp/monitor.pb.cc
)

#generate static library file
add_library(${LIB_PHANTOM_PROTO} ${PROTO_SRC}) 

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_PROTO}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)
