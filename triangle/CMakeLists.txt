cmake_minimum_required(VERSION 3.10)
project(triangle)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 创建动态库
add_library(triangle SHARED
    triangle.c
)

# 如果有其他源文件也需要包含在库中，可以添加到这里
# add_library(triangle SHARED
#     triangle.c
#     showme.c
#     tricali.c
# )

# 设置库的输出名称和版本（可选）
set_target_properties(triangle PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "triangle"
)

# 包含头文件目录
target_include_directories(triangle PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 如果triangle.c使用了数学库，可能需要链接数学库
# find_library(MATH_LIB m)
# target_link_libraries(triangle ${MATH_LIB})

# 可选：创建可执行文件（如果需要测试）
# add_executable(showme showme.c)
# target_link_libraries(showme triangle)

# add_executable(tricali tricali.c)
# target_link_libraries(tricali triangle)

# 安装规则（可选）
install(TARGETS triangle
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES triangle.h DESTINATION include)