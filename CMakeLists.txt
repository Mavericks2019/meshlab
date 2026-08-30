cmake_minimum_required(VERSION 3.21)

project(objViewer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

if(MSVC)
    add_compile_options(/utf-8 /bigobj)
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

add_library(aaatlas_triangle STATIC triangle/triangle.c)
target_compile_definitions(aaatlas_triangle PRIVATE TRILIBRARY NO_TIMER)
target_include_directories(aaatlas_triangle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(aaatlas_triangle PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    VS_GLOBAL_TrackFileAccess false
    VS_USER_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc-x64-host.props"
)
if(MSVC)
    target_compile_options(aaatlas_triangle PRIVATE /wd4477)
endif()

set(AAATLAS_CORE_SOURCES
    atlas/atlaspipeline.cpp
    atlas/atlaspipeline.h
    atlas/reference/ParaQuadCutting.cpp
    atlas/reference/ParaQuadCutting.h
    atlas/reference/ParaQuadChartDecomposition.cpp
    atlas/reference/ParaQuadChartDecomposition.h
    atlas/reference/PolySquareDeformation.cpp
    atlas/reference/PolySquareDeformation.h
    atlas/reference/ChartDeformation.cpp
    atlas/reference/ChartDeformation.h
    atlas/reference/ChartDeformation_Deform.cpp
    atlas/reference/ChartUntangle.cpp
    atlas/reference/ChartUntangle.h
    atlas/reference/MeshViewer/MeshDefinition.cpp
    atlas/reference/MeshViewer/MeshDefinition.h
    atlas/reference/MeshViewer/Mesh_doubleIO.cpp
    atlas/reference/MeshViewer/Mesh_doubleIO.h
    atlas/reference/Mosek/ConvexQuadOptimization.cpp
    atlas/reference/Mosek/ConvexQuadOptimization.h
    atlas/reference/Optimization/HLBFGS/HLBFGS.cpp
    atlas/reference/Optimization/HLBFGS/HLBFGS_BLAS.cpp
    atlas/reference/Optimization/HLBFGS/ICFS.cpp
    atlas/reference/Optimization/HLBFGS/LineSearch.cpp
    atlas/reference/Optimization/HLBFGS/Lite_Sparse_Matrix.cpp
    atlas/reference/Optimization/Numeric/FastMath.cpp
    atlas/reference/Scaffold/Parafun.cpp
    atlas/reference/Scaffold/Parafun.h
    atlas/reference/Scaffold/PardisoSolver.cpp
    atlas/reference/Scaffold/PardisoSolver.h
    atlas/reference/Scaffold/ScafCommon.cpp
    atlas/reference/Scaffold/ScafCommon.h
    atlas/reference/Scaffold/ScafData.cpp
    atlas/reference/Scaffold/ScafData.h
    atlas/reference/Scaffold/StateManager.cpp
    atlas/reference/Scaffold/StateManager.h
    atlas/reference/Scaffold/TriangleInterface.cpp
    atlas/reference/Scaffold/TriangleInterface.h
)

add_library(aaatlas_core STATIC ${AAATLAS_CORE_SOURCES})
target_include_directories(aaatlas_core PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/reference"
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/reference/rectpack2D/src"
)
target_compile_definitions(aaatlas_core PRIVATE
    Mesh=AAAtlasMesh
    MeshTraits=AAAtlasMeshTraits
    Mesh_doubleIO=AAAtlasMesh_doubleIO
    Parafun=AAAtlasParafun
    PardisoSolver=AAAtlasPardisoSolver
)
set_target_properties(aaatlas_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    VS_GLOBAL_TrackFileAccess false
    VS_USER_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc-x64-host.props"
)
if(MSVC)
    target_compile_options(aaatlas_core PRIVATE /wd4828)
endif()
target_link_libraries(aaatlas_core PRIVATE
    Qt5::Core
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
    aaatlas_triangle
)

set(OBJVIEWER_SOURCES
    main.cpp
    menu_utils.cpp
    menu_utils.h
    tab_manager.cpp
    tab_manager.h
    shaders.qrc

    glwidget/arapglwidget.cpp
    glwidget/arapglwidget.h
    glwidget/atlasparameterizationwidget.cpp
    glwidget/atlasparameterizationwidget.h
    glwidget/baseglwidget.cpp
    glwidget/baseglwidget.h
    glwidget/blackholewidget.cpp
    glwidget/blackholewidget.h
    glwidget/cgalglwidget.cpp
    glwidget/cgalglwidget.h
    glwidget/dchartswidget.cpp
    glwidget/dchartswidget.h
    glwidget/steady3ddissectionwidget.cpp
    glwidget/steady3ddissectionwidget.h
    glwidget/printableinterlockwidget.cpp
    glwidget/printableinterlockwidget.h
    glwidget/modelglwidget.cpp
    glwidget/modelglwidget.h
    glwidget/progressiveparameterizationwidget.cpp
    glwidget/progressiveparameterizationwidget.h
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

    progressive/reference/Parafun.cpp
    progressive/reference/Parafun.h
    progressive/reference/PardisoSolver.cpp
    progressive/reference/PardisoSolver.h
    progressive/reference/MeshDefinition.h
    tabs/progressive_parameterization_tab.h
    tabs/atlas_parameterization_tab.h
    tabs/dcharts_tab.h
    tabs/steady3ddissection_tab.h
    tabs/printable_interlock_tab.h

    dcharts/dchartsalgorithm.cpp
    dcharts/dchartsalgorithm.h

    dissection/steady3ddissectionalgorithm.cpp
    dissection/steady3ddissectionalgorithm.h

    interlocking/printableinterlockalgorithm.cpp
    interlocking/printableinterlockalgorithm.h

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
    aaatlas_core
)

if(WIN32)
    set(QT_PLATFORM_PLUGIN
        "$<IF:$<CONFIG:Debug>,$<TARGET_FILE_DIR:Qt5::Core>/../plugins/platforms/qwindowsd.dll,$<TARGET_FILE_DIR:Qt5::Core>/../plugins/platforms/qwindows.dll>"
    )
    add_custom_command(TARGET objViewer POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DRUNTIME_DIRECTORY=$<TARGET_FILE_DIR:Qt5::Core>"
            "-DDESTINATION_DIRECTORY=$<TARGET_FILE_DIR:objViewer>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy_runtime_dlls.cmake"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:objViewer>/platforms"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QT_PLATFORM_PLUGIN}"
            "$<TARGET_FILE_DIR:objViewer>/platforms"
        VERBATIM
    )
endif()

set_target_properties(objViewer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VS_GLOBAL_TrackFileAccess false
    VS_USER_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc-x64-host.props"
)

set_property(SOURCE atlas/atlaspipeline.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/atlaspipeline.h"
)
set_property(SOURCE glwidget/atlasparameterizationwidget.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/atlaspipeline.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/glwidget/atlasparameterizationwidget.h"
)
set_property(SOURCE tab_manager.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/glwidget/atlasparameterizationwidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tabs/atlas_parameterization_tab.h"
)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT objViewer)

install(TARGETS objViewer RUNTIME DESTINATION bin)
