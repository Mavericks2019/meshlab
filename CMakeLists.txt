cmake_minimum_required(VERSION 3.5)
project(objViewer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
    meshutils/my_traits.cpp
    glwidget/uvparamwidget_extended.cpp
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

# 设置安装路径
install(TARGETS ${PROJECT_NAME} DESTINATION bin)