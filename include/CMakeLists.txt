cmake_minimum_required(VERSION 3.5)
project(VolumeMeshViewer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置共享库选项
set(BUILD_SHARED_LIBS ON)
# 启用 Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 基础路径
set(PROJECT_ROOT "/opt/Volume_Framework/Volume_Framework/VolumeMeshProcessing_Base")
set(VOLUME_MESH_VIEWER_DIR "${PROJECT_ROOT}/VolumeMeshViewer")

# 查找必需依赖
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)
find_package(OpenMesh REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenVolumeMesh REQUIRED)

# ANN库路径
set(ANN_INCLUDE_DIR "${PROJECT_ROOT}/Include/ANN")
set(ANN_LIBRARY "${PROJECT_ROOT}/lib/libANN.so")

# 创建共享库
add_library(VolumeMeshViewer SHARED
    # VolumeMeshViewer目录源文件
    ${VOLUME_MESH_VIEWER_DIR}/auxiliary_function_mesh.cpp
    ${VOLUME_MESH_VIEWER_DIR}/InteractiveWidget.cpp
    ${VOLUME_MESH_VIEWER_DIR}/MeshDefinition.cpp
    ${VOLUME_MESH_VIEWER_DIR}/MeshShowSlot.cpp
    ${VOLUME_MESH_VIEWER_DIR}/MeshViewerDraw.cpp
    ${VOLUME_MESH_VIEWER_DIR}/MeshViewerWidget.cpp
    ${VOLUME_MESH_VIEWER_DIR}/QGLViewerWidget.cpp
)

# 包含头文件目录
target_include_directories(VolumeMeshViewer PUBLIC
    ${VOLUME_MESH_VIEWER_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${ANN_INCLUDE_DIR}
    ${OpenVolumeMesh_INCLUDE_DIRS}
)

target_include_directories(VolumeMeshViewer PRIVATE
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${OpenMesh_INCLUDE_DIRS}
)

# 链接依赖库
target_link_libraries(VolumeMeshViewer
    Qt5::Widgets
    Qt5::Core
    Qt5::OpenGL
    OpenGL::GL
    OpenGL::GLU
    OpenMeshCore
    OpenMeshTools
    ${ANN_LIBRARY}
    ${OpenVolumeMesh_LIBRARIES}
)

# Linux系统链接
if(UNIX AND NOT APPLE)
    target_link_libraries(VolumeMeshViewer
        pthread
        dl
        m
    )
endif()

# 设置输出目录
set_target_properties(VolumeMeshViewer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则（可选）
install(TARGETS VolumeMeshViewer
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${VOLUME_MESH_VIEWER_DIR}/
    DESTINATION include/VolumeMeshViewer
    FILES_MATCHING PATTERN "*.h"
)