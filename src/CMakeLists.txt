file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)
foreach(v ${all_srcs})
    string(REGEX MATCH "src/.*" relative_path ${v})
    string(REGEX REPLACE "src/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v})

    set(have_python_h FALSE)
    find_python_header(${v} have_python_h)
    if (have_python_h) 
        target_link_libraries(${target_name} ${Python_LIBRARIES})
    else()
    endif()
    
endforeach()