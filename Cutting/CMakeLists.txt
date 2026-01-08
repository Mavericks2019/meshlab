cmake_minimum_required(VERSION 3.16)
project(QGLViewerWidget)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX AND NOT APPLE) 
    # 查找包
    find_package(OpenGL REQUIRED)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL Gui)

    # 查找OpenMesh
    find_package(OpenMesh REQUIRED)

    # 打印找到的包信息
    message(STATUS "Found Qt5: ${Qt5Core_VERSION}")
    message(STATUS "Found OpenGL: ${OPENGL_FOUND}")
    message(STATUS "Found OpenMesh: ${OPENMESH_FOUND}")

    # 包含目录
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common
        ${OpenGL_INCLUDE_DIRS}
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5OpenGL_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${OPENMESH_INCLUDE_DIRS}
    )

    # 修改此处：将 STATIC 改为 SHARED
    add_library(QGLViewerWidget SHARED
        QGLViewerWidget.cpp
    )

    # 设置目标属性
    target_include_directories(QGLViewerWidget PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # 设置动态库的版本信息（可选）
    set_target_properties(QGLViewerWidget PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )

    # 链接库
    target_link_libraries(QGLViewerWidget
        ${OpenGL_LIBRARIES}
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
        Qt5::Gui
        GLU
        ${OPENMESH_LIBRARIES}
    )

    # 添加CommonLib（如果存在）
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libCommonLib.a)
        target_link_libraries(QGLViewerWidget
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libCommonLib.a
        )
    endif()

    # 设置编译定义
    target_compile_definitions(QGLViewerWidget PRIVATE
        -DQT_CORE_LIB
        -DQT_GUI_LIB
        -DQT_WIDGETS_LIB
        -DQT_OPENGL_LIB
    )
elseif(WIN32)
    # Windows 特定配置
    message(STATUS "Building for Windows system")
    
    # 设置Qt5路径 - 修改为您的Qt安装路径
    set(QT5_ROOT "D:/qt/5.11.1/msvc2017_64")
    set(CMAKE_PREFIX_PATH "${QT5_ROOT};${CMAKE_PREFIX_PATH}")
    
    # 查找Qt5组件
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL)
    
    # 设置OpenGL
    find_package(OpenGL REQUIRED)
    
    # 设置OpenMesh路径
    # 如果使用vcpkg安装，可以使用以下路径
    set(OPENMESH_INCLUDE_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows/include")
    set(OPENMESH_LIB_DIR "C:/Users/Administrator/vcpkg/installed/x64-windows/lib")
    
    # 设置Common头文件和库文件路径
    set(COMMON_INCLUDE_DIR "D:/projects/AAAtlas/Common")
    set(COMMON_LIB_DIR "D:/projects/meshlabframework/libs")
    
    # 打印找到的包信息
    message(STATUS "Found Qt5: ${Qt5Core_VERSION}")
    message(STATUS "Found OpenGL: ${OPENGL_FOUND}")
    message(STATUS "OpenMesh include: ${OPENMESH_INCLUDE_DIR}")
    message(STATUS "OpenMesh lib: ${OPENMESH_LIB_DIR}")
    message(STATUS "Common include: ${COMMON_INCLUDE_DIR}")
    message(STATUS "Common lib: ${COMMON_LIB_DIR}")
    
    # 创建动态链接库
    add_library(QGLViewerWidget SHARED
        QGLViewerWidget.cpp
    )
    
    # 设置目标属性
    target_include_directories(QGLViewerWidget PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${OPENMESH_INCLUDE_DIR}
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5OpenGL_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
    )
    
    # 设置动态库的版本信息
    set_target_properties(QGLViewerWidget PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
        WINDOWS_EXPORT_ALL_SYMBOLS ON  # 导出所有符号
    )
    
    # 设置输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    
    # 链接库
    target_link_libraries(QGLViewerWidget
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
        Qt5::Gui
        opengl32.lib
        glu32.lib
        "${OPENMESH_LIB_DIR}/OpenMeshCore.lib"
        "${OPENMESH_LIB_DIR}/OpenMeshTools.lib"
    )
    
    # 添加CommonLib（如果存在）
    # 首先尝试寻找动态库，然后是静态库
    if(EXISTS "${COMMON_LIB_DIR}/CommonLib.lib")
        target_link_libraries(QGLViewerWidget
            "${COMMON_LIB_DIR}/CommonLib.lib"
        )
        message(STATUS "Found CommonLib: ${COMMON_LIB_DIR}/CommonLib.lib")
    elseif(EXISTS "${COMMON_LIB_DIR}/CommonLib.dll")
        # 如果有dll但没lib，可能需要手动处理
        message(STATUS "Found CommonLib DLL but no LIB file: ${COMMON_LIB_DIR}/CommonLib.dll")
    elseif(EXISTS "${COMMON_LIB_DIR}/libCommonLib.a")
        target_link_libraries(QGLViewerWidget
            "${COMMON_LIB_DIR}/libCommonLib.a"
        )
        message(STATUS "Found CommonLib: ${COMMON_LIB_DIR}/libCommonLib.a")
    else()
        message(WARNING "CommonLib not found in ${COMMON_LIB_DIR}")
    endif()
    
    # 设置编译器定义
    target_compile_definitions(QGLViewerWidget PRIVATE
        -DQT_CORE_LIB
        -DQT_GUI_LIB
        -DQT_WIDGETS_LIB
        -DQT_OPENGL_LIB
        -DNOMINMAX           # 防止Windows min/max宏与std::min/max冲突
        -D_USE_MATH_DEFINES  # 启用数学常量定义
        -DUNICODE
        -D_UNICODE
        # OpenMesh相关定义
        -DOM_STATIC_BUILD
        -D_USE_MATH_DEFINES
    )
    
    # 复制必要的DLL到输出目录
    add_custom_command(TARGET QGLViewerWidget POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMENT "Creating output directory"
    )
    
    # 复制Qt DLLs
    if(Qt5Core_DIR)
        # 获取Qt DLL路径
        get_target_property(Qt5Core_location Qt5::Core IMPORTED_LOCATION_RELEASE)
        get_filename_component(QT_DLL_DIR "${Qt5Core_location}" DIRECTORY)
        
        # 复制Qt DLLs
        set(QT_DLLS
            Qt5Core.dll
            Qt5Widgets.dll
            Qt5Gui.dll
            Qt5OpenGL.dll
        )
        
        foreach(DLL ${QT_DLLS})
            add_custom_command(TARGET QGLViewerWidget POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_DLL_DIR}/${DLL}"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
                COMMENT "Copying ${DLL} to output directory"
            )
        endforeach()
        
        # 复制平台插件
        add_custom_command(TARGET QGLViewerWidget POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_DLL_DIR}/../plugins/platforms/qwindows.dll"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/platforms/"
            COMMENT "Copying Qt platform plugin"
        )
    endif()
    
    # 复制OpenMesh DLLs
    if(EXISTS "${OPENMESH_LIB_DIR}/../bin/OpenMeshCore.dll")
        add_custom_command(TARGET QGLViewerWidget POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENMESH_LIB_DIR}/../bin/OpenMeshCore.dll"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying OpenMeshCore.dll"
        )
    endif()
    
    if(EXISTS "${OPENMESH_LIB_DIR}/../bin/OpenMeshTools.dll")
        add_custom_command(TARGET QGLViewerWidget POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENMESH_LIB_DIR}/../bin/OpenMeshTools.dll"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying OpenMeshTools.dll"
        )
    endif()
    
    # 复制CommonLib DLL（如果存在）
    if(EXISTS "${COMMON_LIB_DIR}/CommonLib.dll")
        add_custom_command(TARGET QGLViewerWidget POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${COMMON_LIB_DIR}/CommonLib.dll"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying CommonLib.dll"
        )
    endif()
    
    # MSVC编译器特定设置
    if(MSVC)
        # 设置编译器选项
        target_compile_options(QGLViewerWidget PRIVATE
            /W3           # 警告级别3
            /EHsc         # C++异常处理
            /MP           # 多处理器编译
        )
        
        # 禁用特定警告
        add_compile_options(
            /wd4251  # 禁用dll接口警告
            /wd4275  # 禁用非dll接口警告
            /wd4996  # 禁用安全警告
        )
        
        # 设置链接器选项
        set_target_properties(QGLViewerWidget PROPERTIES
            LINK_FLAGS "/INCREMENTAL:NO /DEBUG"
        )
    endif()
    
endif()