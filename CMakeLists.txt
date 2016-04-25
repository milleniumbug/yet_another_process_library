cmake_minimum_required(VERSION 3.3)
project(yet_another_process_library)

find_package(Boost 1.54 COMPONENTS system filesystem REQUIRED)
find_package(Threads)

file(GLOB_RECURSE INCLUDES "include/*.hpp")
file(GLOB SOURCES "src/*.cpp")

set(SOURCE_FILES
	${INCLUDES}
	${SOURCES})
	
add_library(yet_another_process_library ${SOURCE_FILES})
target_include_directories(yet_another_process_library
PUBLIC
	include)
if(MSVC)
	target_compile_options(yet_another_process_library PRIVATE /W4)
else()
	target_compile_options(yet_another_process_library PRIVATE -Wall -Wextra -pedantic -Wno-unused-parameter)
endif()
set_property(TARGET yet_another_process_library PROPERTY CXX_STANDARD 11)
set_property(TARGET yet_another_process_library PROPERTY CXX_EXTENSIONS OFF)
target_link_libraries(yet_another_process_library Threads::Threads ${Boost_LIBRARIES})

file(GLOB EXAMPLE_FILES "examples/*.cpp")
foreach(sample ${EXAMPLE_FILES})
	get_filename_component(sample_we ${sample} NAME_WE)
	add_executable(${sample_we} ${sample})
	set_property(TARGET ${sample_we} PROPERTY CXX_STANDARD 14)
	target_link_libraries(${sample_we} yet_another_process_library Threads::Threads ${Boost_LIBRARIES})
	target_include_directories(${sample_we} PUBLIC include)
endforeach()