add_library(slang SHARED IMPORTED GLOBAL)
target_include_directories(slang INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET slang PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.lib")
set_property(TARGET slang PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/windows-x64/release/slang.dll")
