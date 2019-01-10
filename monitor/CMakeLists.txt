#phantom monitor module CmakeLists.txt -- libphantom_monitor.a
set(LIB_PHANTOM_MONITOR phantom_monitor)

#automaticall get src file in specifical dir
aux_source_directory(${PHANTOM_SRC_DIR}/monitor/ MONITOR_SRC)

#generate static library file
add_library(${LIB_PHANTOM_MONITOR} ${MONITOR_SRC})

#specify compile options for target obj
target_compile_options(${LIB_PHANTOM_MONITOR}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)