cmake_minimum_required(VERSION 3.10)
project(MeshIO)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用位置无关代码（PIC），这是动态库必需的
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Windows 系统使用指定路径引入 OpenMesh
if(WIN32)
    # 添加OpenMesh的头文件路径
    include_directories("D:/projects/AAAtlas/include")
    
    # 添加OpenMesh的库文件路径
    link_directories("D:/projects/libs")
    
    # 预定义宏（与参考代码一致）
    add_definitions(
        _USE_MATH_DEFINES
        NOMINMAX
    )
else()
    # Linux 系统使用 find_package 查找 OpenMesh 库
    find_package(OpenMesh REQUIRED)
endif()

# 查找 Eigen3 库
find_package(Eigen3 REQUIRED)

# 创建动态库
add_library(MeshIO SHARED
    Mesh_doubleIO.cpp
    MeshDefinition.cpp
)

if(WIN32)
    # Windows 下的包含目录设置
    target_include_directories(MeshIO PRIVATE
        "D:/projects/AAAtlas/include"
        ${EIGEN3_INCLUDE_DIRS}
    )
    
    # Windows 下的库链接
    target_link_libraries(MeshIO PRIVATE
        "D:/projects/AAAtlas/lib/OpenMeshCore.lib"
        "D:/projects/AAAtlas/lib/OpenMeshTools.lib"
    )
    
    # 添加动态库的dll文件路径到运行时搜索路径
    # 复制OpenMeshCore.dll到输出目录
    if(EXISTS "D:/projects/AAAtlas/lib/OpenMeshCore.dll")
        add_custom_command(TARGET MeshIO POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "D:/projects/AAAtlas/lib/OpenMeshCore.dll"
                $<TARGET_FILE_DIR:MeshIO>/
            COMMENT "Copying OpenMeshCore.dll to output directory"
        )
    endif()
    
    # 复制OpenMeshTools.dll到输出目录
    if(EXISTS "D:/projects/AAAtlas/lib/OpenMeshTools.dll")
        add_custom_command(TARGET MeshIO POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "D:/projects/AAAtlas/lib/OpenMeshTools.dll"
                $<TARGET_FILE_DIR:MeshIO>/
            COMMENT "Copying OpenMeshTools.dll to output directory"
        )
    endif()
    
    # 设置运行时库搜索路径（用于Visual Studio调试）
    set_target_properties(MeshIO PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:MeshIO>;%PATH%"
    )
else()
    # Linux 下的包含目录设置
    target_include_directories(MeshIO PRIVATE
        ${OpenMesh_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
    )
    
    # Linux 下的库链接
    target_link_libraries(MeshIO PRIVATE
        ${OpenMesh_LIBRARIES}
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