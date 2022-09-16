add_library(slang::main SHARED IMPORTED GLOBAL)
target_include_directories(slang::main INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET slang::main PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.lib")
set_property(TARGET slang::main PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.dll")

add_library(slang::glslang SHARED IMPORTED GLOBAL)
set_property(TARGET slang::glslang PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.lib")
set_property(TARGET slang::glslang PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang-glslang.dll")

add_library(slang::llvm SHARED IMPORTED GLOBAL)
set_property(TARGET slang::llvm PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.lib")
set_property(TARGET slang::llvm PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang-llvm.dll")

add_library(slang::slang INTERFACE IMPORTED GLOBAL)
set_property(TARGET slang::slang PROPERTY INTERFACE_LINK_LIBRARIES slang::main slang::glslang slang::llvm)

function(executable_add_slang_dependencies target_name)
	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:slang::main> $<TARGET_FILE_DIR:${target_name}>
	)

	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:slang::glslang> $<TARGET_FILE_DIR:${target_name}>
	)

	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:slang::llvm> $<TARGET_FILE_DIR:${target_name}>
	)
endfunction()
