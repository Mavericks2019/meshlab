cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# ===================== Windows 路径配置 =====================
set(VCPKG_ROOT "C:/Users/Administrator/vcpkg/installed/x64-windows")
set(QT5_ROOT "D:/Qt/5.11.1/msvc2017_64")

# 设置 Qt5 搜索路径
set(CMAKE_PREFIX_PATH "${QT5_ROOT};${CMAKE_PREFIX_PATH}")

# ===================== 查找依赖包 =====================
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

# 添加着色器资源文件
qt5_add_resources(RESOURCE_FILES shaders.qrc)

find_package(OpenMesh REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(CGAL REQUIRED COMPONENTS Core)
include_directories(${CGAL_INCLUDE_DIRS})

# CGAL_USE_FILE 兼容性
if(DEFINED CGAL_USE_FILE)
  include(${CGAL_USE_FILE})
endif()

find_package(OpenGL REQUIRED)

# GLM (header-only)
find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS "${VCPKG_ROOT}/include")
if(GLM_INCLUDE_DIR)
    message(STATUS "Found GLM: ${GLM_INCLUDE_DIR}")
endif()

# ===================== 手动构建的库路径 =====================
# ANN 库
set(ANN_INCLUDE_DIR "D:/ann_lib/src")
set(ANN_LIBRARY "D:/ann_lib/build/Release/ANN.lib")

# OpenVolumeMesh 库 (手动构建)
set(OVM_INCLUDE_DIR "D:/OpenVolumeMesh/src")
set(OVM_BUILD_INCLUDE_DIR "D:/OpenVolumeMesh/build/src")  # 生成的头文件
set(OVM_LIBRARY "D:/OpenVolumeMesh/build/Build/lib/Release/OpenVolumeMesh.lib")

# VolumeMeshViewer include (项目内的include/目录)
set(VWGT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# ===================== 子项目库路径 (Windows .lib) =====================
set(COMMONLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/Common/build/Release/CommonLib.lib")
set(MESHIO_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/meshIO/build/Release/MeshIO.lib")
set(MYMESH_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/meshutils/build/Release/mymesh.lib")
set(CUTBASE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/Cutting/build/Release/QGLViewerWidget.lib")

# 检查子项目库
foreach(LIB_PATH IN ITEMS COMMONLIB_LIBRARY MESHIO_LIBRARY MYMESH_LIBRARY CUTBASE_LIBRARY)
    if(EXISTS ${${LIB_PATH}})
        message(STATUS "Found ${LIB_PATH}: ${${LIB_PATH}}")
    else()
        message(WARNING "${LIB_PATH} not found at ${${LIB_PATH}}")
    endif()
endforeach()

# ===================== 包含目录 =====================
include_directories(
    ${VWGT_INCLUDE_DIR}
    ${ANN_INCLUDE_DIR}
    ${OVM_INCLUDE_DIR}
    ${OVM_BUILD_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/meshIO
    ${CMAKE_CURRENT_SOURCE_DIR}/meshutils
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Cutting
)

# ===================== 可执行文件 =====================
add_executable(${PROJECT_NAME}
    main.cpp
    menu_utils.cpp
    menu_utils.h
    tab_manager.cpp
    tab_manager.h

    # glwidget 文件
    glwidget/baseglwidget.h
    glwidget/baseglwidget.cpp
    glwidget/modelglwidget.cpp
    glwidget/modelglwidget.h
    glwidget/cgalglwidget.h
    glwidget/cgalglwidget.cpp
    glwidget/shortestpathglwidget.h
    glwidget/shortestpathglwidget.cpp
    glwidget/uvparamwidget.cpp
    glwidget/uvparamwidget.h
    glwidget/simplesquarewidget.cpp
    glwidget/simplesquarewidget.h
    glwidget/uvparamwidget_extended.cpp
    glwidget/uvparamwidget_extended.h
    glwidget/relasticwidget.cpp
    glwidget/relasticwidget.h
    glwidget/blackholewidget.cpp
    glwidget/blackholewidget.h
    glwidget/relativisticwidget.cpp
    glwidget/relativisticwidget.h
    glwidget/simplesquarewidget_parameterization.cpp
    glwidget/arapglwidget.cpp
    glwidget/arapglwidget.h

    # tabs (header-only)
    tabs/model_tab.h
    tabs/basic_tab.h
    tabs/cgal_tab.h
    tabs/shortestpath_tab.h
    tabs/uvparam_tab.h
    tabs/dualview_tab.h
    tabs/dualview_extended_tab.h
    tabs/dualview_simple_tab.h
    tabs/relastic_tab.h
    tabs/relativistic_tab.h
    tabs/blackhole_tab.h
    tabs/openmesh_viewer_tab.h
    tabs/new_cgal_uv_tab.h
    tabs/interactive_volume_tab.h

    # include/ 源文件
    include/auxiliary_function_mesh.cpp
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

    ${RESOURCE_FILES}
)

# ===================== 目标包含目录 =====================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${VWGT_INCLUDE_DIR}
    ${ANN_INCLUDE_DIR}
    ${OVM_INCLUDE_DIR}
    ${OVM_BUILD_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/meshIO
    ${CMAKE_CURRENT_SOURCE_DIR}/meshutils
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Cutting
    ${OpenMesh_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

# ===================== 链接库 =====================
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::OpenGL
    opengl32
    glu32
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
    CGAL::CGAL
    ${ANN_LIBRARY}
    ${OVM_LIBRARY}
    ${COMMONLIB_LIBRARY}
    ${MESHIO_LIBRARY}
    ${MYMESH_LIBRARY}
    ${CUTBASE_LIBRARY}
)

# ===================== MSVC编译选项 =====================
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        _USE_MATH_DEFINES
        UNICODE
        _UNICODE
        OM_STATIC_BUILD
    )
    target_compile_options(${PROJECT_NAME} PRIVATE
        /EHsc
        /MP
        /utf-8
        /wd4251
        /wd4275
        /wd4996
        /wd4100
        /wd4267
    )
endif()

# ===================== 编译后复制 DLL =====================
set(QT_BIN_DIR "${QT5_ROOT}/bin")
set(QT_PLUGINS_DIR "${QT5_ROOT}/plugins")
set(VCPKG_BIN_DIR "${VCPKG_ROOT}/bin")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Qt DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_BIN_DIR}/Qt5Core.dll"
        "${QT_BIN_DIR}/Qt5Gui.dll"
        "${QT_BIN_DIR}/Qt5Widgets.dll"
        "${QT_BIN_DIR}/Qt5OpenGL.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    # vcpkg DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VCPKG_BIN_DIR}/OpenMeshCore.dll"
        "${VCPKG_BIN_DIR}/OpenMeshTools.dll"
        "${VCPKG_BIN_DIR}/gmp-10.dll"
        "${VCPKG_BIN_DIR}/gmpxx-4.dll"
        "${VCPKG_BIN_DIR}/mpfr-6.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    # Qt platform plugin
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_PLUGINS_DIR}/platforms/qwindows.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/"
    # Sub-project DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Common/build/Release/CommonLib.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/meshIO/build/Release/MeshIO.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/meshutils/build/bin/Release/mymesh.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/Cutting/build/Release/QGLViewerWidget.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying runtime DLLs to output directory"
)

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
