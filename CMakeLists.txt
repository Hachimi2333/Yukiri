cmake_minimum_required(VERSION 3.28)
project(Yukiri VERSION 0.1.0 LANGUAGES C CXX)
include(cmake/MsvcDependencies.cmake)

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
target_sources(Yukiri PRIVATE src/Text/Unicode.cpp src/Text/TextEditor.cpp src/Text/TextEngine.cpp
	src/Text/GlyphAtlas.cpp src/Platform/FontDiscovery.cpp src/Platform/WindowText.cpp src/Controls/TextBox.cpp)
target_sources(Yukiri PRIVATE src/Platform/Appearance.cpp src/Platform/WindowChrome.cpp)
target_sources(Yukiri PRIVATE src/Controls/Label.cpp src/Controls/Button.cpp src/Controls/CheckBox.cpp
	src/Controls/RadioButton.cpp src/Controls/ToggleSwitch.cpp src/Controls/Slider.cpp
	src/Controls/ProgressBar.cpp src/Controls/ComboBox.cpp src/Controls/TabView.cpp src/Controls/ScrollView.cpp)
if(WIN32)
	target_link_libraries(Yukiri PRIVATE dwrite dwmapi comctl32)
	target_compile_definitions(Yukiri PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()
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

if(YUKIRI_BUILD_DEMO)
	add_executable(YukiriDemo WIN32 demo/Main.cpp demo/Gallery.cpp demo/ResponsivePanel.cpp)
	target_link_libraries(YukiriDemo PRIVATE Yukiri::Yukiri)
	if(MSVC)
		target_compile_options(YukiriDemo PRIVATE /utf-8)
		target_link_options(YukiriDemo PRIVATE /ENTRY:mainCRTStartup)
	endif()
	YukiriCopyRuntime(YukiriDemo)
endif()

if(YUKIRI_BUILD_TESTS)
	enable_testing()
	include(CTest)
	add_executable(YukiriPlatformSmoke tests/PlatformSmoke.cpp)
	target_link_libraries(YukiriPlatformSmoke PRIVATE Yukiri::Yukiri)
	if(WIN32)
		target_link_libraries(YukiriPlatformSmoke PRIVATE SDL3::SDL3-static)
	endif()
	YukiriCopyRuntime(YukiriPlatformSmoke)
	add_executable(YukiriTests tests/SignalTests.cpp tests/LayoutTests.cpp tests/WidgetTreeTests.cpp tests/TextTests.cpp tests/StyleTests.cpp tests/ControlTests.cpp)
	target_link_libraries(YukiriTests PRIVATE Yukiri::Yukiri Catch2::Catch2WithMain)
	target_include_directories(YukiriTests PRIVATE src)
	target_include_directories(YukiriTests PRIVATE third_party/freetype/include "${CMAKE_CURRENT_BINARY_DIR}/third_party/freetype/include" third_party/harfbuzz/src)
	YukiriCopyRuntime(YukiriTests)
	if(MSVC)
		target_compile_options(YukiriTests PRIVATE /utf-8)
		target_compile_options(YukiriPlatformSmoke PRIVATE /utf-8)
	endif()
	catch_discover_tests(YukiriTests)
	file(GLOB_RECURSE publicHeaders CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include" "include/Yukiri/*.hpp")
	set(headerSources)
	foreach(header IN LISTS publicHeaders)
		string(REPLACE "/" "_" headerFile "${header}")
		set(checkSource "${CMAKE_CURRENT_BINARY_DIR}/generated/header_${headerFile}.cpp")
		file(GENERATE OUTPUT "${checkSource}" CONTENT "#include <${header}>\n")
		list(APPEND headerSources "${checkSource}")
	endforeach()
	add_library(YukiriHeaderChecks OBJECT ${headerSources})
	target_link_libraries(YukiriHeaderChecks PRIVATE Yukiri::Yukiri)
endif()

include(cmake/Package.cmake)
