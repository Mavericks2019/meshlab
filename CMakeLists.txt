cmake_minimum_required(VERSION 3.21)

project(objViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

if(MSVC)
    add_compile_options(/MP /utf-8 /bigobj)
    add_compile_definitions(NOMINMAX _USE_MATH_DEFINES)
endif()

include(FetchContent)

# ANN 1.1.2 is not available in the Visual Studio vcpkg registry. Build the
# small upstream library directly so the project remains self-contained.
FetchContent_Declare(
    ann
    URL https://www.cs.umd.edu/~mount/ANN/Files/1.1.2/ann_1.1.2.tar.gz
    URL_HASH SHA256=eea03f2e224b66813226d775053316675375dcec45bd263674c052d9324a49a5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(ann)
file(GLOB ANN_SOURCES CONFIGURE_DEPENDS "${ann_SOURCE_DIR}/src/*.cpp")
add_library(ann STATIC ${ANN_SOURCES})
target_include_directories(ann PUBLIC
    "${ann_SOURCE_DIR}/include"
    "${ann_SOURCE_DIR}/include/ANN"
)
target_compile_definitions(ann PUBLIC DLL_EXPORTS)
set_target_properties(ann PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# This project uses legacy OpenVolumeMesh mesh aliases.
set(OVM_ENABLE_APPLICATIONS OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_UNITTESTS OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OVM_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_DEPRECATED_APIS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    openvolumemesh
    GIT_REPOSITORY https://github.com/OpenVolumeMesh/OpenVolumeMesh.git
    GIT_TAG v3.4.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(openvolumemesh)

find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Gui OpenGL Widgets)
find_package(OpenGL REQUIRED)
find_package(OpenMesh CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(CGAL CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(OBJVIEWER_SOURCES
    main.cpp
    menu_utils.cpp
    menu_utils.h
    tab_manager.cpp
    tab_manager.h
    shaders.qrc

    glwidget/arapglwidget.cpp
    glwidget/arapglwidget.h
    glwidget/baseglwidget.cpp
    glwidget/baseglwidget.h
    glwidget/blackholewidget.cpp
    glwidget/blackholewidget.h
    glwidget/cgalglwidget.cpp
    glwidget/cgalglwidget.h
    glwidget/modelglwidget.cpp
    glwidget/modelglwidget.h
    glwidget/relasticwidget.cpp
    glwidget/relasticwidget.h
    glwidget/relativisticwidget.cpp
    glwidget/relativisticwidget.h
    glwidget/shortestpathglwidget.cpp
    glwidget/shortestpathglwidget.h
    glwidget/simplesquarewidget.cpp
    glwidget/simplesquarewidget.h
    glwidget/simplesquarewidget_parameterization.cpp
    glwidget/uvparamwidget.cpp
    glwidget/uvparamwidget.h
    glwidget/uvparamwidget_extended.cpp
    glwidget/uvparamwidget_extended.h

    Common/CommonDefinitions.cpp
    Common/CommonDefinitions.h
    Common/CommonFunctions.h
    Common/TimeProfiler.cpp
    Common/TimeProfiler.h
    Common/VKString.cpp
    Common/VKString.h
    Common/VKStringList.cpp
    Common/VKStringList.h

    meshutils/my_traits.cpp
    meshutils/my_traits.h

    include/InteractiveWidget.cpp
    include/InteractiveWidget.h
    include/MeshDefinition.cpp
    include/MeshDefinition.h
    include/MeshShowSlot.cpp
    include/MeshViewerDraw.cpp
    include/MeshViewerWidget.cpp
    include/MeshViewerWidget.h
    include/OpenGLHeaders.h
    include/QGLViewerWidget.cpp
    include/QGLViewerWidget.h
    include/auxiliary_function_mesh.cpp
)

add_executable(objViewer WIN32 ${OBJVIEWER_SOURCES})
target_include_directories(objViewer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshutils"
)
target_link_libraries(objViewer PRIVATE
    Qt5::Core
    Qt5::Concurrent
    Qt5::Gui
    Qt5::OpenGL
    Qt5::Widgets
    OpenGL::GL
    OpenGL::GLU
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
    CGAL::CGAL
    glm::glm
    OpenVolumeMesh::OpenVolumeMesh
    ann
)

set_target_properties(objViewer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT objViewer)

install(TARGETS objViewer RUNTIME DESTINATION bin)
