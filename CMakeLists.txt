cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
    glwidget/cgalglwidget.h  # 新增
    glwidget/cgalglwidget.cpp  # 新增
    glwidget/shortestpathglwidget.h  # 新增
    glwidget/shortestpathglwidget.cpp  # 新增
    glwidget/uvparamwidget.cpp  # 新增
    glwidget/simplesquarewidget.cpp
    glwidget/simplesquarewidget.h
    meshutils/my_traits.cpp
    glwidget/uvparamwidget_extended.cpp
    tabs/dualview_simple_tab.h
    # glwidget/glwidget_core.cpp
    # glwidget/glwidget.h
    # glwidget/glwidget_curvature.cpp
    # glwidget/glwidget_mesh_loader.cpp
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
    CGAL::CGAL  # 新增
)

# 如果 CommonLib 存在，链接它
if(EXISTS ${COMMONLIB_LIBRARY})
    set(COMMONLIB_LIBRARY "../lib/libCommonLib.a")
endif()

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)