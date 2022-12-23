
cmake_minimum_required(VERSION 3.15)

# set the project name
project(SandboxApp)

# specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

add_executable(sandboxed_executable main.c)
target_link_libraries(sandboxed_executable cap)

#add_subdirectory(thirdparty)
#add_subdirectory(src-lib)
#add_subdirectory(src-exe)
#add_subdirectory(tests)

# install (TARGETS MainExe)
