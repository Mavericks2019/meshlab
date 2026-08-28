---
name: progressive-parameterization
description: Explain, debug, extend, or verify the Progressive Parameterization solver and M/Mr/Mp Qt viewer in this objViewer repository. Use for its Tutte, progressive-reference, SLIM/CM, snapshot, three-viewport, or Windows build work; do not use for unrelated upstream MeshLab features or generic mesh viewers.
---

# Progressive Parameterization

Work with the repository's progressive surface-parameterization algorithm and
its Qt/OpenGL tab. Answer in the user's language and inspect the current source
before editing because the worktree can contain user changes and generated
solver outputs.

## Route the task

- For mathematical explanations, energy/Jacobian questions, solver changes, or
  the meaning of `M`, `Mr`, `Mp`, `d`, and `alpha`, read
  [references/algorithm.md](references/algorithm.md).
- For source mapping, Qt controls, worker-thread behavior, OpenGL refresh bugs,
  building, or regression checks, read
  [references/implementation.md](references/implementation.md).
- For a new Windows workstation, also read
  [BUILD_WINDOWS.md](../../BUILD_WINDOWS.md).

## Preserve these invariants

- `M` is the fixed connected input 3D triangle mesh.
- Tutte initialization runs once and creates connected planar `Mp_0`.
- `Mp` is the connected planar optimization variable.
- `Mr` is rebuilt from fixed `M` and current `Mp`. It is a set of independent
  per-face 2D reference triangles, not another connected optimized mesh.
- Freeze each newly computed `Mr`, perform exactly one progressive SLIM or CM
  update of `Mp`, then recompute `Mr` from the new `Mp`.
- The per-vertex direction `d` has `2V` entries. One scalar `alpha`, limited by
  all faces, updates the entire mesh: `Mp_new = Mp + alpha * d`.
- The flip-safe maximum step is followed by Armijo backtracking.
- After the progressive phase, restore the ideal references (`Mr = M`) and run
  final CM iterations against the true source energy until convergence.
- The current implementation omits the paper's factor `1/4`; its per-face
  optimum is `4`, not `1`. Keep thresholds and reported energy on that scale.

## Repository expectations

- Keep solver work in `progressive/reference/`, viewport/worker integration in
  `glwidget/progressiveparameterizationwidget.*`, and controls in
  `tabs/progressive_parameterization_tab.h`.
- Keep OpenGL operations on the GUI thread. Worker callbacks emit copied
  snapshots; widgets upload those snapshots to GPU buffers.
- Preserve three observable views: original `M`, current progressive reference
  `Mr`, and current parameterization `Mp`. Refresh `Mr` and `Mp` after every
  emitted solver iteration.
- Use an open, manifold, triangulated mesh with a boundary for validation.
  Closed meshes require cutting before this solver can run.
- Do not add generated `*_mp.obj`, `*_mr.obj`, `*_comp_para_result.obj`, or
  `*_comp_energy_result.txt` files to source commits unless explicitly asked.

Verify changes in proportion to their scope. Solver, CMake, threading, or shared
rendering changes require a full x64 build and an end-to-end sample run. Narrow
control-panel changes still require compilation and visible UI-state checks.
