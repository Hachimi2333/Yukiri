cmake_minimum_required(VERSION 3.28)
project(Yukiri VERSION 0.1.0 LANGUAGES C CXX)

option(YUKIRI_BUILD_DEMO "Build the interactive widget gallery" ${PROJECT_IS_TOP_LEVEL})
option(YUKIRI_BUILD_TESTS "Build headless unit tests" ${PROJECT_IS_TOP_LEVEL})
option(YUKIRI_ENABLE_VALIDATION "Request Vulkan validation when available" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(cmake/Dependencies.cmake)
include(cmake/Runtime.cmake)

add_library(Yukiri STATIC src/Core/Version.cpp src/Platform/Application.cpp src/Platform/Window.cpp)
target_sources(Yukiri PRIVATE src/Render/RenderDevice.cpp src/Render/Resources.cpp
	src/Render/WindowRenderer.cpp src/Render/Pipeline.cpp src/Render/Vma.cpp)
target_sources(Yukiri PRIVATE src/Render/Canvas.cpp src/UI/Widget.cpp src/UI/WidgetTree.cpp
	src/Layout/StackPanel.cpp src/Layout/Grid.cpp src/Styling/Theme.cpp src/Platform/WindowInput.cpp)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
foreach(stage vert frag)
	if(stage STREQUAL "vert")
		set(shaderName UiVertexSpirv)
	else()
		set(shaderName UiFragmentSpirv)
	endif()
	set(shaderBinary "${CMAKE_CURRENT_BINARY_DIR}/generated/Ui.${stage}.spv")
	set(shaderHeader "${CMAKE_CURRENT_BINARY_DIR}/generated/${shaderName}.hpp")
	add_custom_command(OUTPUT "${shaderHeader}"
		COMMAND Vulkan::glslc --target-env=vulkan1.4 "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Ui.${stage}" -o "${shaderBinary}"
		COMMAND "${CMAKE_COMMAND}" "-DINPUT=${shaderBinary}" "-DOUTPUT=${shaderHeader}" "-DNAME=${shaderName}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedSpirv.cmake"
		DEPENDS "shaders/Ui.${stage}" cmake/EmbedSpirv.cmake VERBATIM)
	target_sources(Yukiri PRIVATE "${shaderHeader}")
endforeach()
add_library(Yukiri::Yukiri ALIAS Yukiri)
target_compile_features(Yukiri PUBLIC cxx_std_23)
target_include_directories(Yukiri PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_include_directories(Yukiri PRIVATE src ${Vulkan_INCLUDE_DIRS} "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(Yukiri PRIVATE SDL_MAIN_HANDLED VK_NO_PROTOTYPES
	$<$<BOOL:${YUKIRI_ENABLE_VALIDATION}>:YUKIRI_ENABLE_VALIDATION>
)
target_link_libraries(Yukiri PRIVATE SDL3::SDL3-static volk GPUOpen::VulkanMemoryAllocator
	freetype harfbuzz YukiriUnibreak)

if(MSVC)
	target_compile_options(Yukiri PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
	set_source_files_properties(src/Render/Vma.cpp PROPERTIES COMPILE_OPTIONS /w)
else()
	target_compile_options(Yukiri PRIVATE -Wall -Wextra -Wpedantic)
	set_source_files_properties(src/Render/Vma.cpp PROPERTIES COMPILE_OPTIONS -w)
endif()

if(YUKIRI_BUILD_TESTS)
	enable_testing()
	include(CTest)
	add_executable(YukiriPlatformSmoke tests/PlatformSmoke.cpp)
	target_link_libraries(YukiriPlatformSmoke PRIVATE Yukiri::Yukiri)
	YukiriCopyRuntime(YukiriPlatformSmoke)
	add_executable(YukiriTests tests/SignalTests.cpp tests/LayoutTests.cpp tests/WidgetTreeTests.cpp)
	target_link_libraries(YukiriTests PRIVATE Yukiri::Yukiri Catch2::Catch2WithMain)
	target_include_directories(YukiriTests PRIVATE src)
	YukiriCopyRuntime(YukiriTests)
	if(MSVC)
		target_compile_options(YukiriTests PRIVATE /utf-8)
	endif()
	catch_discover_tests(YukiriTests)
endif()
