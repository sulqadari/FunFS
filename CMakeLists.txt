cmake_minimum_required(VERSION 3.25)
project(FunFS VERSION 0.1)

set(FUNFS_SRC  "Fun_Fs_src")
set(FUNFS_UDP  "Fun_Fs_udp")
set(FUNFS_EXEC "Fun_FS_exe")


#-------------------------- Choose required options

option(USE_DEBUG "Apply debugging flags"                            YES)
option(USE_SANIT "Apply memory leak and boundary violation control" NO)

add_compile_definitions(
	"PAGE_SIZE=1024"
	"PAGES_TOTAL=64"
	"ITABLE_MEMORY_OCCUPATION_IN_PERCENTS=50"
	"DF_CHILDREN_LIST_LEN=7"
)
#--------------------------


if (USE_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -O0 -g -fsanitize=address -static-libasan -fno-omit-frame-pointer")
	add_compile_definitions(
		"FUNFS_DBG_MODE"
	)
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -Ofast")
endif()

add_subdirectory(src)
add_subdirectory(src/platform)

add_executable(${FUNFS_EXEC}
	main.c
)

target_link_libraries(${FUNFS_EXEC}
	PUBLIC Fun_Fs_src
)

target_include_directories(${FUNFS_EXEC}
	PUBLIC ${PROJECT_SOURCE_DIR}/src
	PUBLIC ${PROJECT_SOURCE_DIR}/src/platform
)