function(YukiriCopyRuntime target)
	if(MINGW)
		foreach(runtime libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
			execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${runtime}"
				OUTPUT_VARIABLE runtimePath OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(EXISTS "${runtimePath}")
				add_custom_command(TARGET ${target} POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
					"${runtimePath}" "$<TARGET_FILE_DIR:${target}>" VERBATIM)
			endif()
		endforeach()
	endif()
endfunction()
