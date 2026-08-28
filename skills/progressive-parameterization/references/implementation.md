# Repository Implementation

Read this reference for code changes, UI behavior, compilation, debugging, and
verification in this repository.

## Source map

| Concern | Location |
|---|---|
| Build target and dependencies | `CMakeLists.txt`, `vcpkg.json` |
| Portable VS presets | `CMakePresets.json` |
| Windows setup | `BUILD_WINDOWS.md` |
| Solver state and outer algorithm | `progressive/reference/Parafun.*` |
| Sparse linear solve | `progressive/reference/PardisoSolver.*` |
| Progressive mesh traits | `progressive/reference/MeshDefinition.h` |
| Snapshot conversion and worker | `glwidget/progressiveparameterizationwidget.*` |
| Tab controls | `tabs/progressive_parameterization_tab.h` |
| Tab registration/lifecycle | `tab_manager.*`, `menu_utils.cpp` |
| Small valid sample | `models/progressive_sample.obj` |

## Paper-to-code mapping

| Operation | Symbol |
|---|---|
| Read fixed source `M` | `Parafun(filename)` / `mesh` |
| Normalize source scale | `init()` / `originmesh_area_sqrt` |
| Ideal local frames | `local_coordinate_inverse()` / `source_p**` |
| Tutte `Mp_0` | `Tutte()` / `position_of_mesh` |
| Progressive `t` and `Mr` | `Update_source_same_t()` / `Intp_T_Min`, `update_p**` |
| Outer algorithm | `BPE()` / `run_bpe()` |
| Early progressive step | `SLIM()` |
| Later and final step | `CM()` |
| Restore ideal reference | `recover_to_src()` |
| Flip-safe cap | `max_step()` |
| Armijo step | `backtracking_line_search()` |
| Temporary energy | `Energy()` |
| True source energy | `Energysource()` |

`position_of_mesh` stores all `u` values followed by all `v` values. The local
face order in SLIM/CM is `[u0,u1,u2,v0,v1,v2]`. Despite its historical name,
`PardisoSolver` currently uses Eigen's sparse solver.

## Three-view snapshot path

`ProgressiveWorker` owns `Parafun` and runs on a `QThread`. It emits a copied
`ProgressiveSnapshot` after Tutte and after each progressive/final callback:

- `source`: connected 3D `M`.
- `reference`: disconnected, per-face planar `Mr` reconstructed from
  `update_p**` and aligned near corresponding `Mp` faces for visualization.
- `parameterized`: connected planar `Mp`.

Qt delivers snapshots to `ProgressiveParameterizationWidget::applySnapshot()`
on the GUI thread. Each viewport rebuilds OpenMesh, recomputes normals and
indices, makes its OpenGL context current, and calls
`updateBuffersFromOpenMesh()`. Do not remove this explicit GPU upload: snapshots
usually arrive after `initializeGL()`, and `update()` alone only schedules a
paint using stale buffers.

Load the source file directly into the first viewport before starting the
worker so `M` appears while Tutte initialization is still running.

Preserve each viewport's rotation, zoom, and framing when applying later
snapshots. Initialize the camera only when that viewport receives its first
non-empty mesh after a clear; otherwise iteration callbacks undo user input.

The surface pass uses positive `GL_POLYGON_OFFSET_FILL`, then the overlay draws
indexed `GL_LINES` with `GL_LEQUAL` and depth writes disabled. Do not rely on
`GL_POLYGON_OFFSET_LINE` for this path: it does not bias independent line
primitives, so equal-depth edges can disappear behind freshly drawn faces.

## Threading and controls

- Never mutate widgets or OpenGL buffers in `ProgressiveWorker::run()`.
- Use the atomic stop callback for long solver work.
- `continuous_`, `stepBudget_`, the mutex, and wait condition implement Run,
  Pause, and one-snapshot Step behavior.
- Stop and wait for the worker before replacing it or destroying the widget.
- Rendering mode, face visibility, wireframe visibility, reset, and center
  controls apply consistently to all three viewports and must survive later
  snapshot uploads.

## Build

Set `VCPKG_ROOT` to a bootstrapped vcpkg checkout. The normal VS build is:

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-debug
cmake --build --preset vs2022-release
```

The manifest installs Qt5 Base, CGAL, Eigen3, GLM, and OpenMesh. FetchContent
downloads ANN 1.1.2 and OpenVolumeMesh 3.4.1 during the first configure.

When a header-only tab change appears stale in a custom Ninja build, confirm
that `tab_manager.cpp` was recompiled. The Visual Studio preset is the primary
supported path and avoids relying on localized `/showIncludes` parsing.

## Verification

For solver, threading, project, or shared rendering changes:

1. Build x64 successfully; build both Debug and Release for solver/CMake work.
2. Run `models/progressive_sample.obj` or another open triangular patch.
3. Confirm `M`, `Mr`, and `Mp` are nonblank and correctly framed.
4. Confirm each emitted iteration refreshes `Mr` and `Mp`.
5. Exercise Run, Pause, Step, Reset, face visibility, wireframe visibility,
   rendering mode, reset views, and center views as relevant.
6. Confirm output vertices and energies are finite, output `z` is zero, and no
   parameterized face has zero or reversed signed area.

Inputs must be non-empty, triangular, manifold, open, and have a boundary.
Reject invalid topology with a clear error rather than allowing Tutte or local
coordinate inversion to fail deep inside the solver.

Generated OBJ/TXT diagnostic outputs belong to runtime verification. Keep them
untracked unless the user explicitly requests sample artifacts in the repo.
