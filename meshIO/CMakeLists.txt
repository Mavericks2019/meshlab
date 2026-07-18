cmake_minimum_required(VERSION 3.10)
project(MeshIO)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用位置无关代码（PIC），这是动态库必需的
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(VCPKG_ROOT "C:/Users/Administrator/vcpkg/installed/x64-windows")

# 查找 OpenMesh (works on all platforms with vcpkg toolchain)
find_package(OpenMesh REQUIRED)

# 查找 Eigen3 库
find_package(Eigen3 REQUIRED)

# 创建动态库
add_library(MeshIO SHARED
    Mesh_doubleIO.cpp
    MeshDefinition.cpp
)

if(WIN32)
    set_target_properties(MeshIO PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# 预定义宏（使用target_compile_definitions确保正确的编译器标志）
target_compile_definitions(MeshIO PRIVATE
    _USE_MATH_DEFINES
    NOMINMAX
)

# 包含目录设置
target_include_directories(MeshIO PRIVATE
    ${OpenMesh_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
)

# 库链接
target_link_libraries(MeshIO PRIVATE
    OpenMeshCore
    OpenMeshTools
    Eigen3::Eigen
)

if(WIN32)
    # 复制OpenMesh DLLs到输出目录
    add_custom_command(TARGET MeshIO POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_ROOT}/bin/OpenMeshCore.dll"
            $<TARGET_FILE_DIR:MeshIO>/
        COMMENT "Copying OpenMeshCore.dll to output directory"
    )
    add_custom_command(TARGET MeshIO POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_ROOT}/bin/OpenMeshTools.dll"
            $<TARGET_FILE_DIR:MeshIO>/
        COMMENT "Copying OpenMeshTools.dll to output directory"
    )

    # 设置运行时库搜索路径
    set_target_properties(MeshIO PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:MeshIO>;%PATH%"
    )
endif()

# 安装设置
install(TARGETS MeshIO
    DESTINATION lib
)

# 安装头文件
install(FILES Mesh_doubleIO.h MeshDefinition.h
    DESTINATION include
)