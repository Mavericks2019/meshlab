cmake_minimum_required(VERSION 3.16)
project(QGLViewerWidget)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 查找包
find_package(OpenGL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL Gui)

# 查找OpenMesh
find_package(OpenMesh REQUIRED)

# 打印找到的包信息
message(STATUS "Found Qt5: ${Qt5Core_VERSION}")
message(STATUS "Found OpenGL: ${OPENGL_FOUND}")
message(STATUS "Found OpenMesh: ${OPENMESH_FOUND}")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${OpenGL_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${OPENMESH_INCLUDE_DIRS}
)

# 添加静态库
add_library(QGLViewerWidget STATIC
    QGLViewerWidget.cpp
)

# 设置目标属性
target_include_directories(QGLViewerWidget PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接库
target_link_libraries(QGLViewerWidget
    ${OpenGL_LIBRARIES}
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Gui
    ${OPENMESH_LIBRARIES}
)

# 添加CommonLib（如果存在）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libCommonLib.a)
    target_link_libraries(QGLViewerWidget
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libCommonLib.a
    )
endif()

# 设置编译定义
target_compile_definitions(QGLViewerWidget PRIVATE
    -DQT_CORE_LIB
    -DQT_GUI_LIB
    -DQT_WIDGETS_LIB
    -DQT_OPENGL_LIB
)

# 设置编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(QGLViewerWidget PRIVATE
        -Wall
        -Wextra
        -Wno-deprecated-declarations  # 忽略弃用警告
    )
endif()

# 安装目标
install(TARGETS QGLViewerWidget
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)