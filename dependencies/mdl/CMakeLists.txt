add_library(mdl INTERFACE)
target_include_directories(mdl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")



function(executable_add_mdl_dependencies target_name)
	set(MDL_DLLS
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/nt-x86-64/lib/libmdl_sdk.dll"
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/nt-x86-64/lib/dds.dll"
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/nt-x86-64/lib/mdl_distiller.dll"
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/nt-x86-64/lib/nv_freeimage.dll"
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/nt-x86-64/lib/nv_openimageio.dll"
	)
	foreach(mdl_dll ${MDL_DLLS})
		message(STATUS "mdl_dll:"${mdl_dll})
		add_custom_command(TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${mdl_dll} $<TARGET_FILE_DIR:${target_name}>
		)
	endforeach(mdl_dll)
endfunction()
