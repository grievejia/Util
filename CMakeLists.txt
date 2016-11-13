cmake_minimum_required(VERSION 3.0)
project(Util)

# Check for C++14 support
include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if (NOT COMPILER_SUPPORTS_CXX14)
	message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

# Set compilation flag
set (CMAKE_CXX_FLAGS " -std=c++14 -Wall -fno-rtti -fPIC")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -mtune=native -DNDEBUG")

option (BUILD_TESTS "build all unit tests" OFF)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)
include_directories (${HEADER_PATH})
add_library (Util INTERFACE
)

if (BUILD_TESTS)
	enable_testing()
	set(UNITTEST_PATH ${PROJECT_SOURCE_DIR}/unittest)

	set(GTEST_INSTALL_PATH ${PROJECT_BINARY_DIR}/gtest)
	include(ExternalProject)
	ExternalProject_Add(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG master
		INSTALL_DIR ${GTEST_INSTALL_PATH}
		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	)
	include_directories(${GTEST_INSTALL_PATH}/include)
	link_directories(${GTEST_INSTALL_PATH}/lib)

	macro(add_unit_test testname)
		add_executable(${testname} ${UNITTEST_PATH}/${testname}.cpp)
		add_dependencies(${testname} googletest)
		target_link_libraries(${testname} Util gtest gtest_main pthread)
		add_test(${testname} ${testname})
	endmacro()

	add_unit_test(OptionalTest)
	add_unit_test(VariantTest)
endif()

install (
	DIRECTORY ${HEADER_PATH}/Util
	DESTINATION include
)
install (
	TARGETS Util
	DESTINATION lib
)