include_guard()

# Enable static analysis with clang-tidy
macro(enable_clang_tidy)
  find_program(CLANGTIDY clang-tidy)
  if(CLANGTIDY)
    # construct the clang-tidy command line
    set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY} -extra-arg=-Wno-unknown-warning-option)

    # set warnings as errors
    if(WARNINGS_AS_ERRORS)
      list(APPEND CMAKE_CXX_CLANG_TIDY -warnings-as-errors=*)
    endif()

    # C clang-tidy
    set(CMAKE_C_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY})

    # set C++ standard
    if(NOT
       "${CMAKE_CXX_STANDARD}"
       STREQUAL
       "")
      if("${CMAKE_CXX_CLANG_TIDY_DRIVER_MODE}" STREQUAL "cl")
        set(CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY} -extra-arg=/std:c++${CMAKE_CXX_STANDARD})
      else()
        set(CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY} -extra-arg=-std=c++${CMAKE_CXX_STANDARD})
      endif()
    endif()

    # set C standard
    if(NOT
       "${CMAKE_C_STANDARD}"
       STREQUAL
       "")
      if("${CMAKE_C_CLANG_TIDY_DRIVER_MODE}" STREQUAL "cl")
        set(CMAKE_C_CLANG_TIDY ${CMAKE_C_CLANG_TIDY} -extra-arg=/std:c${CMAKE_C_STANDARD})
      else()
        set(CMAKE_C_CLANG_TIDY ${CMAKE_C_CLANG_TIDY} -extra-arg=-std=c${CMAKE_C_STANDARD})
      endif()
    endif()

  else()
    message(${WARNING_MESSAGE} "clang-tidy requested but executable not found")
  endif()
endmacro()
