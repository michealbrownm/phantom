#phantom main module CmakeLists.txt -- phantom

set(APP_PHANTOM phantom)

set(APP_PHANTOM_SRC
    main.cpp
    configure.cpp
    ../api/web_server.cpp
    ../api/web_server_query.cpp
    ../api/web_server_update.cpp
    ../api/web_server_command.cpp
    ../api/web_server_helper.cpp
    ../api/websocket_server.cpp
    ../api/console.cpp
)

set(INNER_LIBS phantom_glue phantom_ledger phantom_consensus phantom_overlay phantom_common phantom_utils phantom_proto phantom_http phantom_ed25519 phantom_monitor)
set(V8_LIBS v8_base v8_libbase v8_external_snapshot v8_libplatform v8_libsampler icui18n icuuc inspector)

#generate executable file
add_executable(${APP_PHANTOM} ${APP_PHANTOM_SRC})

#specify dependent libraries for target obj
	
IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${APP_PHANTOM}
    -Wl,-dn ${INNER_LIBS} -Wl,--start-group ${V8_LIBS} -Wl,--end-group ${PHANTOM_DEPENDS_LIBS} ${PHANTOM_LINKER_FLAGS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	add_definitions(${PHANTOM_LINKER_FLAGS})
	target_link_libraries(${APP_PHANTOM} ${INNER_LIBS} ${V8_LIBS} ${PHANTOM_DEPENDS_LIBS})
ENDIF () 


#specify compile options for target obj
target_compile_options(${APP_PHANTOM}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_PHANTOM}
    RUNTIME DESTINATION ${PHANTOMCHAIN}/bin
)
