cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

#set(COMMONLIB_INCLUDE_DIR ../lib/include)
#set(COMMONLIB_LIBRARY ../lib/libCommonLib.a)

# 检查 CommonLib 是否存在
#if(NOT EXISTS ${COMMONLIB_INCLUDE_DIR} OR NOT EXISTS ${COMMONLIB_LIBRARY})
#    message(WARNING "CommonLib not found at expected location. Some features may be disabled.")
#else()
#    # 添加头文件路径
#    include_directories(${COMMONLIB_INCLUDE_DIR})
    
#    # 添加库文件路径
#    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)
#endif()

# 查找Qt5组件
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
# 添加着色器资源文件
qt5_add_resources(RESOURCE_FILES shaders.qrc)

find_package(OpenMesh REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(CGAL REQUIRED)
include_directories(${CGAL_INCLUDE_DIRS})

# 添加 MeshIO 库的配置
# 假设 libMeshIO.so 在你指定的路径
set(MESHIO_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/meshIO/build/libMeshIO.so)

# 检查 MeshIO 库是否存在
if(NOT EXISTS ${MESHIO_LIBRARY})
    message(WARNING "MeshIO library not found at ${MESHIO_LIBRARY}. You may need to build it first.")
else()
    message(STATUS "Found MeshIO library: ${MESHIO_LIBRARY}")
endif()

set(MY_TRI ${CMAKE_CURRENT_SOURCE_DIR}/meshutils/build/lib/libmymesh.so)

# 检查 MY_TRI 库是否存在
if(NOT EXISTS ${MY_TRI})
    message(WARNING "MeshIO library not found at ${MY_TRI}. You may need to build it first.")
else()
    message(STATUS "Found MY_TRI library: ${MY_TRI}")
endif()

set(COMMONLIB_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Common/build/libCommonLib.so)

# 检查 COMMONLIB_LIBRARY 库是否存在
if(NOT EXISTS ${COMMONLIB_LIBRARY})
    message(WARNING "MeshIO library not found at ${COMMONLIB_LIBRARY}. You may need to build it first.")
else()
    message(STATUS "Found COMMONLIB_LIBRARY library: ${COMMONLIB_LIBRARY}")
endif()

set(CUTBASE ${CMAKE_CURRENT_SOURCE_DIR}/Cutting/build/libQGLViewerWidget.so)

# 检查 CUTBASE 库是否存在
if(NOT EXISTS ${CUTBASE})
    message(WARNING "MeshIO library not found at ${CUTBASE}. You may need to build it first.")
else()
    message(STATUS "Found CUTBASE library: ${CUTBASE}")
endif()

# 添加 MeshIO 头文件路径（假设头文件在 meshIO 源码目录）
set(MESHIO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/meshIO)
include_directories(${MESHIO_INCLUDE_DIR})

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    glwidget/baseglwidget.h
    glwidget/baseglwidget.cpp
    glwidget/modelglwidget.cpp
    glwidget/modelglwidget.h
    glwidget/cgalglwidget.h
    glwidget/cgalglwidget.cpp
    glwidget/shortestpathglwidget.h
    glwidget/shortestpathglwidget.cpp
    glwidget/uvparamwidget.cpp
    glwidget/simplesquarewidget.cpp
    glwidget/simplesquarewidget.h
    glwidget/uvparamwidget_extended.cpp
    glwidget/relasticwidget.cpp      # 新增：相对论可视化widget
    glwidget/relasticwidget.h        # 新增：相对论可视化widget头文件
    glwidget/relativisticwidget.cpp
    tabs/dualview_simple_tab.h
    glwidget/simplesquarewidget_parameterization.cpp
    tabs/model_tab.h
    tabs/basic_tab.h
    tabs/cgal_tab.h
    tabs/shortestpath_tab.h
    tabs/uvparam_tab.h
    tabs/dualview_tab.h
    tabs/dualview_extended_tab.h
    tabs/dualview_simple_tab.h
    tabs/relastic_tab.h              # 新增：相对论可视化tab
    glwidget/arapglwidget.cpp
    glwidget/arapglwidget.h
    menu_utils.cpp
    menu_utils.h
    tab_manager.cpp
    tab_manager.h
    # 新增文件
    tabs/openmesh_viewer_tab.h
    tabs/new_cgal_uv_tab.h
    ${RESOURCE_FILES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::OpenGL
    GL
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
    CGAL::CGAL
    # 添加 MeshIO 库
    ${MESHIO_LIBRARY}
    ${MY_TRI}
    ${CUTBASE}
    ${COMMONLIB_LIBRARY}
)

# 如果 CommonLib 存在，链接它
#if(EXISTS ${COMMONLIB_LIBRARY})
#    set(COMMONLIB_LIBRARY "../lib/libCommonLib.a")
#endif()

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)