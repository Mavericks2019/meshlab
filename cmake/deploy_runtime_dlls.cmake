if(NOT DEFINED RUNTIME_DIRECTORY OR NOT IS_DIRECTORY "${RUNTIME_DIRECTORY}")
    message(FATAL_ERROR "Runtime DLL directory does not exist: ${RUNTIME_DIRECTORY}")
endif()

if(NOT DEFINED DESTINATION_DIRECTORY)
    message(FATAL_ERROR "Runtime DLL destination was not provided")
endif()

file(GLOB runtime_dlls "${RUNTIME_DIRECTORY}/*.dll")
if(NOT runtime_dlls)
    message(FATAL_ERROR "No runtime DLLs found in: ${RUNTIME_DIRECTORY}")
endif()

file(MAKE_DIRECTORY "${DESTINATION_DIRECTORY}")
foreach(runtime_dll IN LISTS runtime_dlls)
    get_filename_component(runtime_name "${runtime_dll}" NAME)
    file(COPY_FILE
        "${runtime_dll}"
        "${DESTINATION_DIRECTORY}/${runtime_name}"
        ONLY_IF_DIFFERENT
    )
endforeach()
