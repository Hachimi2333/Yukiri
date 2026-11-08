cmake_minimum_required(VERSION 3.28)
project(Yukiri VERSION 0.1.0 LANGUAGES C CXX)

option(YUKIRI_BUILD_DEMO "Build the interactive widget gallery" ${PROJECT_IS_TOP_LEVEL})
option(YUKIRI_BUILD_TESTS "Build headless unit tests" ${PROJECT_IS_TOP_LEVEL})
option(YUKIRI_ENABLE_VALIDATION "Request Vulkan validation when available" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

add_library(Yukiri STATIC src/Core/Version.cpp)
add_library(Yukiri::Yukiri ALIAS Yukiri)
target_compile_features(Yukiri PUBLIC cxx_std_23)
target_include_directories(Yukiri PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_include_directories(Yukiri PRIVATE src ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(Yukiri PRIVATE SDL_MAIN_HANDLED VK_NO_PROTOTYPES
	$<$<BOOL:${YUKIRI_ENABLE_VALIDATION}>:YUKIRI_ENABLE_VALIDATION>
)
target_link_libraries(Yukiri PRIVATE SDL3::SDL3-static volk GPUOpen::VulkanMemoryAllocator
	freetype harfbuzz YukiriUnibreak)

if(MSVC)
	target_compile_options(Yukiri PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
	target_compile_options(Yukiri PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(YUKIRI_BUILD_TESTS)
	enable_testing()
	include(CTest)
endif()
