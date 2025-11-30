add_executable(${FUNFS_EXEC}
	main.c	
	flash_emu.c
	iso7816.c
	debug.c
)

target_include_directories(${FUNFS_EXEC}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)