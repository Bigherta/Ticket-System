cmake_minimum_required(VERSION 3.10)

project(Ticket-System-2025 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ticket_system
	BPlusTree/main.cpp
)

target_include_directories(ticket_system PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/BPlusTree
)

target_compile_options(ticket_system PRIVATE
	$<$<CONFIG:Release>:-O2>
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Debug>:-fsanitize=address>
	$<$<CONFIG:Debug>:-fsanitize=undefined>
	$<$<CONFIG:Debug>:-fsanitize=leak>
)

target_link_options(ticket_system PRIVATE
	$<$<CONFIG:Debug>:-fsanitize=address>
	$<$<CONFIG:Debug>:-fsanitize=undefined>
	$<$<CONFIG:Debug>:-fsanitize=leak>
)

if(BUILD_TESTING)
	add_test(
		NAME ticket_system_smoke
		COMMAND ticket_system
	)
endif()
