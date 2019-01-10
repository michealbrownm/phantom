#phantom ed25519 module CmakeLists.txt -- libphantom_ed25519.a

set(LIB_PHANTOM_ED25519 phantom_ed25519)
aux_source_directory(${PHANTOM_SRC_DIR}/3rd/ed25519-donna/ ED25519_SRC)
add_library(${LIB_PHANTOM_ED25519} ${ED25519_SRC}) 

target_compile_options(${LIB_PHANTOM_ED25519} 
	PUBLIC -DASIO_STANDALONE 
	PUBLIC -fPIC
)
