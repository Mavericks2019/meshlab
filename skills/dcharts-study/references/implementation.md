# Repository Implementation

Read this reference for code tracing, debugging, UI work, compilation, tests, or
experiments with the repository implementation.

## Source map

| Concern | Location |
|---|---|
| Parameters, mesh copy, snapshots, public API | `dcharts/dchartsalgorithm.h` |
| Segmentation engine and seam construction | `dcharts/dchartsalgorithm.cpp` |
| Background worker and colored OpenGL viewport | `glwidget/dchartswidget.*` |
| Controls and sample selection | `tabs/dcharts_tab.h` |
| Tab lifecycle and menu registration | `tab_manager.*`, `menu_utils.cpp` |
| Build and test targets | `CMakeLists.txt` |
| Core algorithm smoke test | `tests/dcharts_smoke.cpp` |
| Rendered UI smoke test | `tests/dcharts_ui_smoke.cpp` |
| Established sample mesh | `models/cow.obj` |

## Algorithm symbol map

| Paper or operation | Implementation symbol |
|---|---|
| Input triangulation and adjacency | `Engine::load()` |
| Farthest-point initialization | `farthestSeeds()` |
| Initial local proxies | `initializeCharts()` |
| Area-weighted cone proxy | `fitProxy()` |
| `F(C,t)` | `fittingError()` |
| `F^alpha C^beta P^gamma` | `assignmentCost()` |
| One greedy growth round | `growCharts()` |
| Low-error central seed | `selectSeed()` |
| Refit and compact labels | `compactAndRecomputeCharts()` |
| Small/large hole handling | `fillSmallComponent()`, `spawnChartInComponent()`, `fillHoles()` |
| Engineering boundary substitute | `relaxBoundaries()` |
| Cylinder-boundary merge test | `cylindricalBoundaryError()`, `mergeCharts()` |
| Darts/gussets visualization | `buildPartialCuts()`, `addVertexPathToCut()` |
| Final chart and mesh seams | `seamEdges()` |
| Iteration/state delivery | `publish()` |

The internal mesh keeps original shared vertices and triangular face indices.
`FaceInfo` caches face normal, centroid, area, three vertices, edge lengths, and
neighbor faces. `GlobalEdge` connects the two incident faces and is also the
source for seam visualization. Non-manifold edges and degenerate triangles are
rejected during loading.

## Public state and snapshots

`DChartsParameters` contains all tunable values exposed by the UI plus internal
convergence and cleanup controls. `DChartsSnapshot` is a copied, thread-safe
view of one observable stage:

```text
mesh vertices and triangle indices
one chart label per face
seed face indices
seam vertex-index pairs
phase, iteration, chart count
assigned and unassigned counts
changed ratio, mean F, max F, completion flag
```

When changing the engine, keep snapshots internally consistent:

```text
faceCharts.size() == mesh.faces.size() / 3
assignedFaces + unassignedFaces == faceCharts.size()
all complete labels are in [0, chartCount)
seedFaces.size() == chartCount
seamEdges.size() is even
```

The callback returns `false` to stop computation. A stop request is a normal
cancellation path, not an algorithm failure.

## Threading and step behavior

`DChartsWorker` owns the algorithm run on a `QThread`. The worker emits copied
snapshots. Qt queues them to `DChartsWidget::applySnapshot()` on the GUI thread,
where text and OpenGL data are updated.

- Continuous mode sleeps briefly after non-final snapshots so stages remain
  visible.
- Pause sets `continuous_` false; the worker waits after the next snapshot.
- Step supplies one unit of `stepBudget_`, allowing exactly one later snapshot.
- Reset requests cancellation, wakes the condition variable, waits for the
  worker, clears the viewport, and restores the empty UI state.
- Stop and join the worker before replacing it or destroying the widget.

Do not call widget methods or upload buffers from `DChartsWorker::run()`.

## Rendering path

`DChartsViewport` duplicates vertices per face in its display-only OpenMesh.
This lets every face use one chart color without requiring shared-vertex color
blending. The source `DChartsMeshData` remains unchanged.

- A fixed multi-hue palette handles the first charts; higher labels use
  deterministic HSV colors.
- Unassigned faces are gray.
- Chart boundaries, input boundaries, and partial cuts are thick dark lines.
- Seed faces are represented by white points at face centroids.
- The optional triangle wireframe is independent of chart seams.

The shader and buffer upload must remain on the GUI thread with a current
OpenGL context. Initialize the camera only on the first non-empty snapshot so
later iterations do not undo user rotation or zoom.

## UI behavior

Open `Parameter > D-Charts` in the main application.

- `Load` chooses an OBJ/OFF/PLY mesh and starts paused after the first stage.
- `Sample` uses `models/cow.obj` and also starts paused.
- Play runs or resumes continuously; Pause stops after the current stage; Step
  advances one stage; Reset cancels and clears.
- Display toggles control chart seams, seed points, and triangle wireframe.
- The status panel reports phase, iteration, chart count, assignment counts,
  changed-face percentage, and fitting errors.

When changing controls, test both the initial no-mesh state and the completed
state. Long file names and status lines must remain readable in the scrollable
control panel.

## Build and verification

Use the configured x64 Visual Studio environment described in
`BUILD_WINDOWS.md`. Typical commands are:

```powershell
cmake -S . -B build-vs
cmake --build build-vs --config Release --target objViewer
cmake --build build-vs --config Release --target dcharts_smoke
cmake --build build-vs --config Release --target dcharts_ui_smoke
ctest --test-dir build-vs -C Release -R dcharts_cow_smoke --output-on-failure
```

Run the UI smoke test when rendering, snapshots, controls, or shaders change:

```powershell
.\build-vs\Release\dcharts_ui_smoke.exe `
  .\models\cow.obj `
  .\build-vs\dcharts-ui-smoke.png
```

The established default regression should end with all 5804 cow faces assigned
and more than one chart. Chart count can legitimately change when the algorithm
or parameters change, so assert structural invariants and inspect the rendered
image rather than treating exactly 12 charts as universal correctness.

For end-to-end verification:

1. Confirm the full Release application links.
2. Run the core CTest and check that completion has zero unassigned faces.
3. Run the UI smoke test and inspect that the canvas is nonblank, chart colors
   are distinct, and seams/seeds are visible.
4. Open the real menu tab, click Sample, verify the seed-stage pause, then use
   Step, Play, Pause, and Reset.
5. Try one invalid or non-manifold mesh when changing input validation.

## Learning and extension order

Trace one default run in this order:

```text
execute -> load -> initializeCharts -> growCharts
        -> compactAndRecomputeCharts -> fillHoles
        -> relaxBoundaries -> mergeCharts -> buildPartialCuts -> publish
```

Good focused extensions are:

1. Add per-phase timing and export label statistics without changing labels.
2. Visualize fitting error as a separate color mode.
3. Compare growth with one cost exponent set to zero at a time.
4. Replace local boundary relaxation with the paper's fuzzy-region path method.
5. Add a topology-cut output that duplicates seam vertices.
6. Feed each resulting disk chart to a UV parameterizer and measure distortion.

Keep each extension separately testable. Do not combine a new segmentation
objective, topology cutting, and UV optimization in one unmeasured change.
