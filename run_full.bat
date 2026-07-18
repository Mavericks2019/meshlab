@echo off
REM Black Hole Viewer (Full) - Run Script
set "QT_DIR=D:\Qt\5.11.1\msvc2017_64"
set "VCPKG_DIR=C:\Users\Administrator\vcpkg\installed\x64-windows"
set "PATH=%QT_DIR%\bin;%VCPKG_DIR%\bin;%PATH%"
set "QT_PLUGIN_PATH=%QT_DIR%\plugins"
start "" "D:\blackholecpp_full\build\Release\objViewer.exe"
echo Full Black Hole Viewer launched!
