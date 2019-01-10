cmake_minimum_required(VERSION 2.8)

project(phantom)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")
message(STATUS "-DSVNVERSION="${SVNVERSION})

set( V8_LIB_PATH "linux")
set( OS_NAME "OS_LINUX")
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	set( OS_NAME "OS_LINUX")
	set(PHANTOM_LINKER_FLAGS "-Wl,-dy -lpthread -lrt -ldl")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    MESSAGE(STATUS "current platform: Darwin")  
	set( OS_NAME "OS_MAC")
	set(PHANTOM_LINKER_FLAGS "-Wl, -lpthread -ldl")
	set( V8_LIB_PATH "mac")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  

set(PHANTOMCHAIN phantomchain)
set(PHANTOM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PHANTOM_ROOT_DIR ${PHANTOM_SRC_DIR}/..)
set(LIBRARY_OUTPUT_PATH ${PHANTOM_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PHANTOM_ROOT_DIR}/bin)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

execute_process(COMMAND make all WORKING_DIRECTORY ${PHANTOM_SRC_DIR}/3rd)

include_directories(
    ${PHANTOM_SRC_DIR}
    ${PHANTOM_SRC_DIR}/3rd/basic/include/v8
    ${PHANTOM_SRC_DIR}/3rd/protobuf/src
    ${PHANTOM_SRC_DIR}/3rd/jsoncpp/include
    ${PHANTOM_SRC_DIR}/3rd/sqlite 
    ${PHANTOM_SRC_DIR}/3rd/rocksdb/include
    ${PHANTOM_SRC_DIR}/3rd/pcre-8.39
    ${PHANTOM_SRC_DIR}/3rd/websocketpp
    ${PHANTOM_SRC_DIR}/3rd/bzip2-1.0.6
    ${PHANTOM_SRC_DIR}/3rd/zlib-1.2.8
    ${PHANTOM_SRC_DIR}/3rd/asio/include
    ${PHANTOM_SRC_DIR}/3rd/openssl/include
    ${PHANTOM_SRC_DIR}/3rd/http
    ${PHANTOM_SRC_DIR}/3rd/libscrypt
)

link_directories(
    ${PHANTOM_SRC_DIR}/3rd/protobuf/src/.libs
    ${PHANTOM_SRC_DIR}/3rd/rocksdb
    ${PHANTOM_SRC_DIR}/3rd/pcre-8.39/.libs
    ${PHANTOM_SRC_DIR}/3rd/jsoncpp/libs
    ${PHANTOM_SRC_DIR}/3rd/openssl
    ${PHANTOM_SRC_DIR}/3rd/zlib-1.2.8
    ${PHANTOM_SRC_DIR}/3rd/libscrypt
    ${PHANTOM_SRC_DIR}/3rd/bzip2-1.0.6
    ${PHANTOM_SRC_DIR}/3rd/basic/lib
    ${PHANTOM_SRC_DIR}/3rd/curl/lib/.libs/
    ${PHANTOM_SRC_DIR}/3rd/v8_target/${V8_LIB_PATH}/
    ${LIBRARY_OUTPUT_PATH}
)

set(PHANTOM_DEPENDS_LIBS protobuf rocksdb pcreposix pcrecpp pcre json ssl crypto z bz2 scrypt)

add_subdirectory(3rd/http)
add_subdirectory(3rd/ed25519-donna)
add_subdirectory(glue)
add_subdirectory(utils)
add_subdirectory(proto)
add_subdirectory(ledger)
add_subdirectory(common)
add_subdirectory(libphantom_tools)
add_subdirectory(overlay)
add_subdirectory(consensus)
add_subdirectory(daemon)
add_subdirectory(monitor)
add_subdirectory(main)

set(PHANTOM_SCRIPTS ${PHANTOM_ROOT_DIR}/deploy)
install(
    PROGRAMS ${PHANTOM_SCRIPTS}/phantom ${PHANTOM_SCRIPTS}/phantomd ${PHANTOM_SCRIPTS}/start-stop-daemon
    DESTINATION ${PHANTOMCHAIN}/scripts
)

install(
    DIRECTORY ${PHANTOM_ROOT_DIR}/bin/
    DESTINATION ${PHANTOMCHAIN}/bin
    FILES_MATCHING PATTERN "*.bin" PATTERN "*.dat"
)

install(
    DIRECTORY ${PHANTOM_ROOT_DIR}/build/win32/config/
    DESTINATION ${PHANTOMCHAIN}/config
    FILES_MATCHING PATTERN "phantom.json" PATTERN "phantom-mainnet.json" PATTERN "phantom-testnet.json"
)

install(
    DIRECTORY ${PHANTOM_ROOT_DIR}/build/win32/jslib/
    DESTINATION ${PHANTOMCHAIN}/jslib
    FILES_MATCHING PATTERN "*.js"
)

set(SUPPLEMENT ${PHANTOM_ROOT_DIR}/build/linux/MakeSupplement)
set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${PHANTOMCHAIN})
message(STATUS "install dir: ${FIX_DIR}")

file(WRITE ${SUPPLEMENT} "soft_link:\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/phantom /etc/init.d/phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/phantomd /etc/init.d/phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc1.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc1.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc2.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc3.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc4.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantom /etc/rc5.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc1.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc1.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc2.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc3.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc4.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/phantomd /etc/rc5.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S99phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/phantom;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S50phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/phantomd;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/scripts\n")
