cmake_minimum_required(VERSION 3.10)
project(MeshIO)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用位置无关代码（PIC），这是动态库必需的
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 OpenMesh 库
find_package(OpenMesh REQUIRED)

# 查找 Eigen3 库
find_package(Eigen3 REQUIRED)

# 创建动态库（将 STATIC 改为 SHARED）
add_library(MeshIO SHARED
    Mesh_doubleIO.cpp
    MeshDefinition.cpp
)

# 设置包含目录
target_include_directories(MeshIO PRIVATE
    ${OpenMesh_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
)

# 链接 OpenMesh 库
target_link_libraries(MeshIO PRIVATE
    ${OpenMesh_LIBRARIES}
)

# 安装设置
install(TARGETS MeshIO
    DESTINATION lib
)

# 安装头文件
install(FILES Mesh_doubleIO.h MeshDefinition.h
    DESTINATION include
)