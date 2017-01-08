cmake_minimum_required(VERSION 3.0)
project(Util)

# Set compilation flag
set (CMAKE_CXX_FLAGS "-Wall -fno-rtti")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -mtune=native -DNDEBUG")

include(CTest)

set(HEADER_PATH ${PROJECT_SOURCE_DIR}/include)
include_directories (${HEADER_PATH})
add_library (util INTERFACE)
install (
	DIRECTORY ${HEADER_PATH}/Util
	DESTINATION include
)
install (
	TARGETS util
	DESTINATION lib
)
target_include_directories(util INTERFACE
  $<BUILD_INTERFACE:${HEADER_PATH}>
  $<INSTALL_INTERFACE:include>)

if (BUILD_TESTING)
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
		set_property(TARGET ${testname} PROPERTY CXX_STANDARD 14)
		set_property(TARGET ${testname} PROPERTY CXX_STANDARD_REQUIRED ON)
		add_test(${testname} ${testname})
	endmacro()

	add_unit_test(OptionalTest)
	add_unit_test(VariantTest)
	add_unit_test(StopWatchTest)
	add_unit_test(LambdaVisitorTest)
	add_unit_test(NotNullableTest)
endif()