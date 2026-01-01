cmake_minimum_required(VERSION 3.10)
project(MeshIO)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenMesh 库
find_package(OpenMesh REQUIRED)

# 查找 Eigen3 库
find_package(Eigen3 REQUIRED)

# 创建静态库
add_library(MeshIO STATIC
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