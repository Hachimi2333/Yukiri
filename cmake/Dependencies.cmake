find_package(Vulkan 1.4 REQUIRED COMPONENTS glslc)

foreach(dependency SDL volk VulkanMemoryAllocator freetype harfbuzz libunibreak SDL_image lunasvg)
	if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/${dependency}/.git")
		message(FATAL_ERROR "Missing ${dependency}. Run git submodule update --init --recursive.")
	endif()
endforeach()

set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN ON CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)

set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_WIC OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
foreach(format ANI AVIF GIF JXL LBM PCX PNM QOI SVG TGA TIF WEBP XCF XPM XV)
	set(SDLIMAGE_${format} OFF CACHE BOOL "" FORCE)
endforeach()
foreach(format BMP JPG PNG)
	set(SDLIMAGE_${format} ON CACHE BOOL "" FORCE)
endforeach()
foreach(format ANI AVIF BMP GIF JPG PNG TGA WEBP)
	set(SDLIMAGE_${format}_SAVE OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(third_party/SDL_image EXCLUDE_FROM_ALL)

set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_PLUTOVG OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/lunasvg EXCLUDE_FROM_ALL)

set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/volk EXCLUDE_FROM_ALL)
target_include_directories(volk PUBLIC $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIRS}>)

set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON)
add_subdirectory(third_party/freetype EXCLUDE_FROM_ALL)
if(MSVC)
	target_compile_options(freetype PRIVATE /utf-8)
endif()

set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU_DEMO OFF CACHE STRING "" FORCE)
set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GOBJECT OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/harfbuzz EXCLUDE_FROM_ALL)

set(unibreakRoot "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libunibreak/src")
add_library(YukiriUnibreak STATIC
	${unibreakRoot}/unibreakbase.c
	${unibreakRoot}/unibreakdef.c
	${unibreakRoot}/linebreak.c
	${unibreakRoot}/linebreakdata.c
	${unibreakRoot}/linebreakdef.c
	${unibreakRoot}/eastasianwidthdef.c
	${unibreakRoot}/emojidef.c
	${unibreakRoot}/wordbreak.c
	${unibreakRoot}/graphemebreak.c
)
target_include_directories(YukiriUnibreak PUBLIC $<BUILD_INTERFACE:${unibreakRoot}>)

if(YUKIRI_BUILD_TESTS)
	set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
	set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
	add_subdirectory(third_party/Catch2 EXCLUDE_FROM_ALL)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/extras")
	include(Catch)
endif()
