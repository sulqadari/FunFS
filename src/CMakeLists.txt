add_library(${FUNFS_SRC}
	# main.c
	debug.c
	flash_emu.c
	iso7816_apdu.c
	iso7816_cmd.c
	iso7816_helpers.c
)

target_include_directories(${FUNFS_SRC}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)