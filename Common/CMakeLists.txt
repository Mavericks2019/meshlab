cmake_minimum_required(VERSION 3.10)
project(CommonLib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动处理 Qt 的 moc、uic 和 rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt5 核心模块
find_package(Qt5 COMPONENTS Core REQUIRED)

# 创建库
add_library(CommonLib STATIC
    CommonDefinitions.cpp
    CommonDefinitions.h
    CommonFunctions.h
    TimeProfiler.cpp
    TimeProfiler.h
    VKString.cpp
    VKString.h
    VKStringList.cpp
    VKStringList.h
)

# 包含头文件目录
target_include_directories(CommonLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接 Qt5 Core 库
target_link_libraries(CommonLib
    Qt5::Core
)

# 设置编译器定义（如果需要）
target_compile_definitions(CommonLib PRIVATE
    # 如果需要启用时间分析，取消下面的注释
    # CALCULATE_TIMING
)

# 安装规则（可选）
install(TARGETS CommonLib DESTINATION lib)
install(FILES 
    CommonDefinitions.h
    CommonFunctions.h
    TimeProfiler.h
    VKString.h
    VKStringList.h
    DESTINATION include/CommonLib
)