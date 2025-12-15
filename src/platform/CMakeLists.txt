add_library(${FUNFS_PLATFORM}
	${CMAKE_CURRENT_SOURCE_DIR}/simulator/simu_udp.c
)

target_include_directories(${FUNFS_PLATFORM}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/simulator
	PUBLIC ${PROJECT_SOURCE_DIR}/src
)