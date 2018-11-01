#phantom tools module CmakeLists.txt -- libphantomtools.so

set(LIB_PHANTOM_TOOLS phantomtools)
set(TOOLS_SRC
    lib_phantom_tools.cpp
)

set(INNER_TOOLS_LIBS phantom_common phantom_utils phantom_ed25519)
set(PHANTOM_TOOLS_DEPENDS_LIBS json ssl crypto scrypt)


#Generate share library files
add_library(${LIB_PHANTOM_TOOLS} SHARED ${TOOLS_SRC}) 

target_link_libraries(${LIB_PHANTOM_TOOLS} ${INNER_TOOLS_LIBS} ${PHANTOM_TOOLS_DEPENDS_LIBS})

#Specify compiling options for target objects
target_compile_options(${LIB_PHANTOM_TOOLS}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
