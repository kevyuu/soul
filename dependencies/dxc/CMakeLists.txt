add_library(dxc SHARED IMPORTED GLOBAL)
target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET dxc PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/dxcompiler.lib")
set_property(TARGET dxc PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/dxcompiler.dll")

function(executable_add_dxc_dependencies target_name)
	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:dxc> $<TARGET_FILE_DIR:${target_name}>
	)
endfunction()
