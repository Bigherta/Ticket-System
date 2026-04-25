cmake_minimum_required(VERSION 3.10)

project(TicketSystem2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Collect all .cpp sources under src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(ticket_system ${SRC_FILES})

target_include_directories(ticket_system PRIVATE
	${CMAKE_SOURCE_DIR}/include
)

# Basic compiler warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(ticket_system PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Build-type specific flags
target_compile_options(ticket_system PRIVATE
	$<$<CONFIG:Release>:-O2>
	$<$<CONFIG:Debug>:-g>
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(ticket_system PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>)
	target_link_options(ticket_system PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>)
endif()

# Keep the historical output name 'code' (optional)
set_target_properties(ticket_system PROPERTIES OUTPUT_NAME code)

if(BUILD_TESTING)
	add_test(
		NAME ticket_system_smoke
		COMMAND ticket_system
	)
endif()
