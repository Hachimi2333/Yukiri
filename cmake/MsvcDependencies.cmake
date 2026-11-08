if(MSVC AND CMAKE_CXX_CL_SHOWINCLUDES_PREFIX MATCHES "^娉")
	# Repair UTF-8 diagnostics decoded through CP936 during compiler detection.
	set(CMAKE_CL_SHOWINCLUDES_PREFIX "注意: 包含文件:  ")
endif()
