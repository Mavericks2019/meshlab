# Windows build

## Prerequisites

- Windows 10 or 11, x64
- Visual Studio 2022 with the **Desktop development with C++** workload
- Git
- CMake 3.21 or newer (the Visual Studio CMake component is sufficient)
- vcpkg

Install and bootstrap vcpkg from PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg.git D:\dev\vcpkg
D:\dev\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "D:\dev\vcpkg"
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "D:\dev\vcpkg", "User")
```

Restart Visual Studio after setting `VCPKG_ROOT`.

Before the first configure, clone the local dependency checkouts used by CMake
(SSH recommended when HTTPS is flaky):

```powershell
git clone --depth 1 --branch v3.4.1 git@github.com:OpenVolumeMesh/OpenVolumeMesh.git third_party/OpenVolumeMesh
git clone --depth 1 git@github.com:jlblancoc/nanoflann.git third_party/nanoflann
```

## Configure and build

```powershell
git clone --branch windows-vs-build https://github.com/Mavericks2019/meshlab.git
cd meshlab
cmake --preset vs2022-x64
cmake --build --preset vs2022-release
```

You can also open the repository folder directly in Visual Studio and select the
`Visual Studio 2022 x64` configure preset. The generated solution focuses on a
single application project (`objViewer`); atlas/triangle/ANN shim sources are
compiled into that executable. OpenVolumeMesh remains a small excluded
dependency library under `ThirdParty`.

The vcpkg manifest installs CGAL, Eigen3, GLM, OpenMesh, and Qt 5 Base
automatically. The first configure can take a long time because Qt and CGAL
may need to be built locally; later incremental builds are much faster.
