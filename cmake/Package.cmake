include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS Yukiri SDL3-static volk harfbuzz freetype YukiriUnibreak
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT Development)

set(YUKIRI_PACKAGE_COMPILER "${CMAKE_CXX_COMPILER_ID}")
set(YUKIRI_PACKAGE_POINTER_SIZE "${CMAKE_SIZEOF_VOID_P}")
set(YUKIRI_PACKAGE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
set(YUKIRI_PACKAGE_SYSTEM_LIBRARIES "dwrite;dwmapi;comctl32;kernel32;user32;gdi32;winmm;imm32;ole32;oleaut32;version;uuid;advapi32;setupapi;shell32")
if(MINGW)
	list(APPEND YUKIRI_PACKAGE_SYSTEM_LIBRARIES m)
endif()
configure_package_config_file(cmake/YukiriConfig.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfig.input.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Yukiri")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfig.cmake"
	INPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfig.input.cmake")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfigVersion.cmake"
	VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/generated/YukiriConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Yukiri" COMPONENT Development)

set(licenseDirectory "${CMAKE_INSTALL_DATADIR}/licenses/Yukiri")
install(FILES LICENSE DESTINATION "${licenseDirectory}" COMPONENT Development)
foreach(dependency SDL volk VulkanMemoryAllocator freetype harfbuzz libunibreak)
	if(dependency STREQUAL "SDL")
		set(license "LICENSE.txt")
	elseif(dependency STREQUAL "volk")
		set(license "LICENSE.md")
	elseif(dependency STREQUAL "VulkanMemoryAllocator")
		set(license "LICENSE.txt")
	elseif(dependency STREQUAL "freetype")
		set(license "docs/FTL.TXT")
	elseif(dependency STREQUAL "harfbuzz")
		set(license "COPYING")
	else()
		set(license "LICENCE")
	endif()
	install(FILES "third_party/${dependency}/${license}" DESTINATION "${licenseDirectory}"
		RENAME "${dependency}.txt" COMPONENT Development)
endforeach()
install(FILES third_party/harfbuzz/src/ms-use/COPYING DESTINATION "${licenseDirectory}"
	RENAME HarfBuzz-Unicode.txt COMPONENT Development)
install(FILES third_party/SDL/src/hidapi/LICENSE.txt third_party/SDL/src/hidapi/LICENSE-bsd.txt
	DESTINATION "${licenseDirectory}/SDL-hidapi" COMPONENT Development)
install(FILES README.md README.zh-CN.md AGENTS.md FUTURE.md LICENSE
	DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/Yukiri" COMPONENT Documentation)
install(DIRECTORY docs/ DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/Yukiri/docs" COMPONENT Documentation)
install(DIRECTORY assets/ DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/Yukiri/assets" COMPONENT Documentation)
if(TARGET YukiriDemo)
	install(TARGETS YukiriDemo RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Demo)
	if(MINGW)
		get_filename_component(compilerDirectory "${CMAKE_CXX_COMPILER}" DIRECTORY)
		foreach(licenseRoot "${compilerDirectory}/../licenses" "${compilerDirectory}/../share/licenses")
			foreach(component gcc gcc-libs winpthreads mingw-w64)
				if(IS_DIRECTORY "${licenseRoot}/${component}")
					install(DIRECTORY "${licenseRoot}/${component}/" DESTINATION "${licenseDirectory}/MinGW/${component}" COMPONENT Demo)
				endif()
			endforeach()
		endforeach()
		foreach(runtime libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
			execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${runtime}"
				OUTPUT_VARIABLE runtimePath OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(EXISTS "${runtimePath}")
				install(FILES "${runtimePath}" DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Demo)
			endif()
		endforeach()
	endif()
endif()
