set(PROJECT_NAME OpenGL-Examples)

################################################################################
# Source groups
################################################################################
set(Headers
    "src/ExampleLayer.h"
)
source_group("Headers" FILES ${Headers})

set(Sources
    "src/ExampleApp.cpp"
    "src/ExampleLayer.cpp"
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Headers}
    ${Sources}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE OpenGL-Examples)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    VS_GLOBAL_KEYWORD "Win32Proj"
#)
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "OpenGL-Examples"
    TARGET_NAME_RELEASE "OpenGL-Examples"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64/OpenGL-Examples/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64/OpenGL-Examples/"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebug
    >
    $<$<CONFIG:Release>:
        MultiThreaded
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/vendor;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/vendor/glm;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/vendor/Glad/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OpenGL-Core/vendor/imgui"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "GLCORE_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "GLCORE_RELEASE"
    ">"
    # "GLCORE_PLATFORM_WINDOWS;"
    "UNICODE;"
    "_UNICODE"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /ZI
        >
        $<$<CONFIG:Release>:
            /Ox;
            /Oi;
            /GF;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        /MP;
        /std:c++17;
        /Gm-;
        /W3;
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG;
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /DEBUG:FULL;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    OpenGL-Core
)

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL-Core
)

