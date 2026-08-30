cmake_minimum_required(VERSION 3.21)

project(objViewer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(MSVC)
    add_compile_options(/utf-8 /bigobj /W3 /wd4267 /wd4305 /wd4996 /wd4477 /wd4101 /wd4244 /wd4834)
    add_compile_definitions(NOMINMAX _USE_MATH_DEFINES)
endif()

# Prefer local vendored deps when present (avoids network during configure).
set(MESHLAB_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(OVM_SOURCE_DIR "${MESHLAB_THIRD_PARTY}/OpenVolumeMesh")
if(NOT EXISTS "${OVM_SOURCE_DIR}/CMakeLists.txt")
    include(FetchContent)
    set(FETCHCONTENT_QUIET ON)
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
    FetchContent_GetProperties(openvolumemesh)
    if(NOT openvolumemesh_POPULATED)
        FetchContent_Populate(openvolumemesh)
        set(OVM_SOURCE_DIR "${openvolumemesh_SOURCE_DIR}")
    endif()
endif()

set(OVM_ENABLE_APPLICATIONS OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_UNITTESTS OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OVM_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(OVM_ENABLE_DEPRECATED_APIS ON CACHE BOOL "" FORCE)
add_subdirectory("${OVM_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/openvolumemesh-build" EXCLUDE_FROM_ALL)
if(TARGET OpenVolumeMesh)
    set_target_properties(OpenVolumeMesh PROPERTIES
        FOLDER "ThirdParty"
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Gui OpenGL Widgets)
find_package(OpenGL REQUIRED)
find_package(OpenMesh CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(CGAL CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

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

    triangle/triangle.c
    third_party/ann_shim/ANN.cpp
    third_party/ann_shim/ANN.h
    ${AAATLAS_CORE_SOURCES}
)

add_executable(objViewer WIN32 ${OBJVIEWER_SOURCES})

target_include_directories(objViewer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshutils"
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/reference"
    "${CMAKE_CURRENT_SOURCE_DIR}/atlas/reference/rectpack2D/src"
    "${MESHLAB_THIRD_PARTY}/ann_shim"
    "${MESHLAB_THIRD_PARTY}/nanoflann/include"
)

set_source_files_properties(${AAATLAS_CORE_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS
        "Mesh=AAAtlasMesh;MeshTraits=AAAtlasMeshTraits;Mesh_doubleIO=AAAtlasMesh_doubleIO;Parafun=AAAtlasParafun;PardisoSolver=AAAtlasPardisoSolver"
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTORCC ON
)

set_source_files_properties(triangle/triangle.c PROPERTIES
    COMPILE_DEFINITIONS "TRILIBRARY;NO_TIMER"
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTORCC ON
)

set_source_files_properties(
    third_party/ann_shim/ANN.cpp
    PROPERTIES
        SKIP_AUTOMOC ON
        SKIP_AUTOUIC ON
        SKIP_AUTORCC ON
)

if(MSVC)
    set_source_files_properties(
        triangle/triangle.c
        ${AAATLAS_CORE_SOURCES}
        PROPERTIES COMPILE_OPTIONS "/W0"
    )
endif()

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

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES ${OBJVIEWER_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT objViewer)

install(TARGETS objViewer RUNTIME DESTINATION bin)
