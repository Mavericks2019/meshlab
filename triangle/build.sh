#!/bin/bash

# build.sh - 编译 Triangle 库

# 设置编译选项
CFLAGS="-std=c99 -Wall -Wextra -fPIC"
LDFLAGS="-lm"

# 处理平台差异
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    CFLAGS="$CFLAGS -D__int64='long long' -D\"unsigned __int64='unsigned long long'\""
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    CFLAGS="$CFLAGS -D__int64='long long' -D\"unsigned __int64='unsigned long long'\""
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    # Windows (MinGW/Cygwin)
    CFLAGS="$CFLAGS -D__int64=__int64 -D\"unsigned __int64=unsigned __int64\""
fi

echo "编译选项: $CFLAGS"
echo "链接选项: $LDFLAGS"

# 编译为动态库
echo "正在编译动态库..."
gcc $CFLAGS -shared triangle.c -o libtriangle.so $LDFLAGS

# 编译为静态库
echo "正在编译静态库..."
gcc $CFLAGS -c triangle.c -o triangle.o
ar rcs libtriangle.a triangle.o

# 编译示例程序
echo "正在编译示例程序..."
gcc $CFLAGS tricali.c -o tricali libtriangle.so $LDFLAGS
# 或者使用静态库: gcc $CFLAGS tricali.c -o tricali_static libtriangle.a $LDFLAGS

# 设置库路径（临时）
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

echo "编译完成！"
echo "动态库: libtriangle.so"
echo "静态库: libtriangle.a"
echo "示例程序: tricali"
echo ""
echo "运行示例: ./tricali"