---
name: dcharts-study
description: Explain, trace, experiment with, debug, extend, or verify the D-Charts quasi-developable mesh segmentation algorithm and Qt tab in this objViewer repository. Use for its cone proxy, F/C/P growth cost, Lloyd iterations, seam construction, colored snapshots, or D-Charts tests; do not use for generic data charts, unrelated UV solvers, or upstream MeshLab features.
---

# D-Charts Study

Work with the repository's D-Charts surface-segmentation implementation and its
Qt/OpenGL visualization. Answer in the user's language. Inspect the current
source before describing behavior or editing it because this project is used as
an experimental learning workspace.

## Route the task

- For paper concepts, formulas, derivations, parameter meaning, or comparisons
  with the original algorithm, read
  [references/algorithm.md](references/algorithm.md).
- For source tracing, Qt controls, worker/snapshot behavior, OpenGL rendering,
  tests, builds, or implementation changes, read
  [references/implementation.md](references/implementation.md).
- For a new Windows workstation, also read
  [BUILD_WINDOWS.md](../../BUILD_WINDOWS.md).

## Preserve these invariants

- A chart is a connected set of triangle faces represented by a cone proxy
  `(N, c)`, where `||N|| = 1` and `c = cos(theta)`.
- Face labels use `-1` only for temporarily unassigned faces. A complete result
  must label every face and use compact chart indices in `[0, chartCount)`.
- Each chart seed must be a face belonging to that chart. Recompute proxies and
  seeds after relabeling, hole filling, boundary relaxation, or merging.
- Region growth accepts a face only while its fitting error is below `Fmax`.
  Hole filling and developable merging are later stages and can make the final
  maximum fitting error exceed `Fmax`; do not interpret `Fmax` as a guaranteed
  final global bound.
- `DChartsAlgorithm` performs segmentation and seam extraction. It does not
  duplicate seam vertices, physically cut the mesh, or compute flattened UVs.
- A seam is stored as pairs of original vertex indices. Keep the edge array
  even-sized and indices valid for the copied mesh.
- Worker code may compute CPU snapshots in a background thread, but widgets and
  OpenGL buffers must be changed only on the GUI thread.
- Treat paper fidelity and engineering substitutions explicitly. In particular,
  the current boundary-straightening pass is a connectivity-preserving local
  relaxation, not the paper's exact fuzzy-region shortest-path construction.

## Repository expectations

- Keep segmentation logic in `dcharts/dchartsalgorithm.*`, worker and viewport
  integration in `glwidget/dchartswidget.*`, and controls in
  `tabs/dcharts_tab.h`.
- Preserve observable snapshots for seeds, each Lloyd round, convergence, hole
  filling, boundary straightening, developable merging, and completion.
- Preserve Run, Pause, Step, Reset, seam, seed, and wireframe behavior when
  changing the algorithm or UI.
- Reject unreadable, empty, degenerate, or non-manifold input with a useful
  error. Triangulate supported polygon meshes before segmentation.
- Use `models/cow.obj` for the established regression. The current defaults are
  expected to assign all 5804 faces and produce a multi-chart result.

For algorithm, CMake, threading, or rendering changes, run both the core smoke
test and UI smoke test, then exercise the real `Parameter > D-Charts` tab. For
explanations, distinguish what the paper specifies, what follows mathematically,
and what this implementation chooses.
