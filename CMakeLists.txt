cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

set(COMMONLIB_INCLUDE_DIR ../lib/include)
set(COMMONLIB_LIBRARY ../lib/libCommonLib.a)

# 检查 CommonLib 是否存在
if(NOT EXISTS ${COMMONLIB_INCLUDE_DIR} OR NOT EXISTS ${COMMONLIB_LIBRARY})
    message(WARNING "CommonLib not found at expected location. Some features may be disabled.")
else()
    # 添加头文件路径
    include_directories(${COMMONLIB_INCLUDE_DIR})
    
    # 添加库文件路径
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)
endif()

# 查找Qt5组件
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
# 添加着色器资源文件
qt5_add_resources(RESOURCE_FILES shaders.qrc)

find_package(OpenMesh REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(CGAL REQUIRED)
include_directories(${CGAL_INCLUDE_DIRS})

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
    meshutils/my_traits.cpp
    glwidget/uvparamwidget_extended.cpp
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
    glwidget/arapglwidget.cpp
    glwidget/arapglwidget.h
    menu_utils.cpp
    menu_utils.h
    tab_manager.cpp    # 添加这个
    tab_manager.h      # 添加这个
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
)

# 如果 CommonLib 存在，链接它
if(EXISTS ${COMMONLIB_LIBRARY})
    set(COMMONLIB_LIBRARY "../lib/libCommonLib.a")
endif()

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)