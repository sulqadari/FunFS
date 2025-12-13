cmake_minimum_required(VERSION 3.25)
project(FunFS VERSION 0.1)
set(FUNFS_EXEC "FunFSexec")


#-------------------------- Choose required options

option(USE_DEBUG "Apply debugging flags"                            YES)
option(USE_SANIT "Apply memory leak and boundary violation control" NO)

#--------------------------


if (USE_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -O0 -g3 -fsanitize=address -static-libasan -fno-omit-frame-pointer")
	add_compile_definitions(
		"FUNFS_DBG_MODE"
	)

else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -Ofast")
endif()

add_subdirectory(src)