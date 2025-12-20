cmake_minimum_required(VERSION 3.25)
project(FunFS VERSION 0.1)

set(FUNFS_SRC       "FunFS_src")
set(FUNFS_PLATFORM  "FunFS_platfrom")
set(FUNFS_EXEC      "FunFS_exe")


#-------------------------- Choose required options

option(USE_DEBUG "Apply debugging flags"                            YES)
option(USE_SANIT "Apply memory leak and boundary violation control" NO)

add_compile_definitions(
	"PAGE_SIZE=32"
	"PAGES_TOTAL=32"
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
	# src/tests_main.c
)

target_link_libraries(${FUNFS_EXEC}
	PUBLIC FunFS_src
	PUBLIC FunFS_platfrom
)

target_include_directories(${FUNFS_EXEC}
	PUBLIC ${PROJECT_SOURCE_DIR}/src
	PUBLIC ${PROJECT_SOURCE_DIR}/src/platform
)