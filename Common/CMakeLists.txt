cmake_minimum_required(VERSION 3.10)
project(CommonLib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 根据平台设置不同的编译选项
if(WIN32)
    # Windows特定的设置
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)  # 自动导出所有符号
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")  # 警告等级4
    
    # Windows上使用你提供的指定Qt5路径的方式
    set(QT5_ROOT "D:/qt/5.11.1/msvc2017_64")
    set(CMAKE_PREFIX_PATH "${QT5_ROOT};${CMAKE_PREFIX_PATH}")
    
    # 如果找不到Qt5，提供明确的错误信息
    if(NOT EXISTS "${QT5_ROOT}/lib/cmake/Qt5")
        message(FATAL_ERROR "
        ============================================
        找不到Qt5库！
        
        请确保Qt5已正确安装。
        你的Qt5路径应该是: ${QT5_ROOT}
        
        检查该路径下是否有:
        - bin/ 目录
        - lib/cmake/Qt5/ 目录
        - include/ 目录
        
        如果路径不正确，请修改CMakeLists.txt中的QT5_ROOT变量。
        ============================================")
    endif()
else()
    # Linux/macOS设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 启用自动处理 Qt 的 moc、uic 和 rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt5 核心模块
find_package(Qt5 COMPONENTS Core REQUIRED)

# 创建动态库
add_library(CommonLib SHARED
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

# 设置编译器定义
if(WIN32)
    # Windows特定的定义
    target_compile_definitions(CommonLib PRIVATE
        NOMINMAX  # 避免Windows.h定义min/max宏
        WIN32_LEAN_AND_MEAN  # 减少Windows头文件包含
        # 如果需要启用时间分析，取消下面的注释
        # CALCULATE_TIMING
    )
else()
    target_compile_definitions(CommonLib PRIVATE
        # 如果需要启用时间分析，取消下面的注释
        # CALCULATE_TIMING
    )
endif()

# 设置动态库属性
if(WIN32)
    # Windows动态库命名约定
    set_target_properties(CommonLib PROPERTIES
        DEBUG_POSTFIX "d"  # 调试版本加"d"后缀
        VERSION 1.0.0
        SOVERSION 1
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE  # 自动导出符号
    )
else()
    # Linux/macOS动态库命名
    set_target_properties(CommonLib PROPERTIES
        DEBUG_POSTFIX "d"
        VERSION 1.0.0
        SOVERSION 1
    )
endif()

# 复制必要的DLL到输出目录（仅Windows）
if(WIN32)
    # 确保输出目录存在
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    
    # 复制Qt5的DLL
    add_custom_command(TARGET CommonLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT5_ROOT}/bin/Qt5Core$<$<CONFIG:Debug>:d>.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        COMMENT "Copying Qt5 Core DLL"
    )
endif()

# 安装规则
install(TARGETS CommonLib
    RUNTIME DESTINATION bin  # Windows的DLL放在bin目录
    LIBRARY DESTINATION lib  # Linux的.so放在lib目录
    ARCHIVE DESTINATION lib  # 静态库/导入库放在lib目录
)

install(FILES 
    CommonDefinitions.h
    CommonFunctions.h
    TimeProfiler.h
    VKString.h
    VKStringList.h
    DESTINATION include/CommonLib
)

# Windows特定的额外设置
if(WIN32 AND MSVC)
    # MSVC编译器设置
    target_compile_options(CommonLib PRIVATE
        /MP  # 启用多处理器编译
        /EHsc  # 启用C++异常处理
        /utf-8  # 强制使用UTF-8编码
    )
    
    # 禁用某些警告
    target_compile_options(CommonLib PRIVATE
        /wd4251  # 禁用dll接口警告
        /wd4275  # 禁用非dll接口警告
    )
    
    # 设置字符集
    target_compile_definitions(CommonLib PRIVATE
        UNICODE
        _UNICODE
    )
    
    # 如果使用Qt，可能需要添加Qt的bin目录到PATH以便找到DLL
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    file(TO_CMAKE_PATH "${QT_BIN_DIR}" QT_BIN_DIR)
    
    # 在构建时将Qt的bin目录添加到PATH
    set_property(TARGET CommonLib PROPERTY
        VS_DEBUGGER_ENVIRONMENT "PATH=${QT_BIN_DIR};%PATH%"
    )
endif()

# 可选：添加一个配置选项，让用户可以自定义Qt5路径
option(CUSTOM_QT5_PATH "Use custom Qt5 installation path" OFF)
if(CUSTOM_QT5_PATH AND WIN32)
    set(QT5_ROOT "" CACHE PATH "Path to Qt5 installation (e.g., D:/qt/5.11.1/msvc2017_64)")
    if(QT5_ROOT)
        set(CMAKE_PREFIX_PATH "${QT5_ROOT};${CMAKE_PREFIX_PATH}")
        message(STATUS "Using custom Qt5 path: ${QT5_ROOT}")
    endif()
endif()