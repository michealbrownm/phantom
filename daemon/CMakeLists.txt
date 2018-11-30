#phantom daemon module CmakeLists.txt

set(DEAMON_DEPENDS_LIB ssl crypto)

# -- phantom_phantomd -- #
set(BIN_PHANTOM_ID phantomd)

add_executable(${BIN_PHANTOM_ID} ./phantom/main.cpp)


IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${BIN_PHANTOM_ID} -Wl,-dn ${DEAMON_DEPENDS_LIB} ${PHANTOM_LINKER_FLAGS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	target_link_libraries(${BIN_PHANTOM_ID} -Wl, ${DEAMON_DEPENDS_LIB} ${PHANTOM_LINKER_FLAGS})
ENDIF () 



target_compile_options(${BIN_PHANTOM_ID}
    PUBLIC -std=c++11
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${BIN_PHANTOM_ID}
    RUNTIME DESTINATION ${PHANTOMCHAIN}/bin
)

