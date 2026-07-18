cmake_minimum_required(VERSION 3.10)
project(mymesh VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 寻找依赖包
find_package(OpenMesh REQUIRED)
find_package(Eigen3 REQUIRED)

# 包含目录
include_directories(${OpenMesh_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIRS})

# 源文件
set(SOURCES
    my_traits.cpp
)

# 头文件
set(HEADERS
    my_traits.h
)

# 创建动态链接库
add_library(mymesh SHARED ${SOURCES} ${HEADERS})

if(WIN32)
    set_target_properties(mymesh PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# 链接库
target_link_libraries(mymesh OpenMeshCore OpenMeshTools Eigen3::Eigen)

# MSVC编译选项
if(MSVC)
    target_compile_definitions(mymesh PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()

# 设置属性（可选）
set_target_properties(mymesh PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${HEADERS}"
)

# 安装规则（如果需要）
install(TARGETS mymesh
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# 测试可执行文件（可选）
if(BUILD_TESTING)
    enable_testing()
    add_executable(test_mymesh test.cpp)  # 需要创建test.cpp
    target_link_libraries(test_mymesh mymesh)
endif()