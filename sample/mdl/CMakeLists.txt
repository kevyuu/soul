add_executable(mdl_sample "mdl_sample.cpp")
target_link_libraries(mdl_sample 
	PRIVATE sample_common mdl)

executable_add_mdl_dependencies(mdl_sample)