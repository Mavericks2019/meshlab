# Algorithm Reference

Read this reference for algorithm explanations, derivations, energy changes, or
solver modifications.

## State and topology

The input is an open triangulated disk-topology mesh:

```text
M = {V, F}
```

The output uses the same connectivity in the plane:

```text
Mp = {Vp, F}
```

Each source face is placed isometrically in its own local 2D frame. A current
progressive reference `Mr` is a collection of independent triangles; vertices
shared by `M` and `Mp` are not shared in the display representation of `Mr`.

## Symmetric Dirichlet energy

For one reference face and its current parameterized face, form edge matrices:

```text
Qr = [r1 - r0, r2 - r0]
Qp = [p1 - p0, p2 - p0]
J  = Qp * inverse(Qr)
```

The paper's distortion is:

```text
D(J) = 1/4 * (||J||_F^2 + ||inverse(J)||_F^2), det(J) > 0
```

It is infinite for a non-positive determinant. The local code drops `1/4`, so
the minimum is `4`. Do not compare code energy or `bound_distortion_K` directly
with paper-scale values without dividing by four.

## Initialization

Run Tutte once:

1. Place boundary vertices on a convex circle.
2. Solve uniform one-ring averaging for interior vertices.
3. Scale the result consistently with the source area.

The result is a connected, foldover-free initial map `Mp_0`. Tutte is not part
of the progressive loop.

## Progressive reference

For each face, compute the map from its ideal local triangle in fixed `M` to the
current triangle in `Mp` and take its SVD:

```text
J = U diag(sigma, tau) V^T
```

Interpolate the stretch exponentially:

```text
J(t) = U diag(sigma^t, tau^t) V^T, 0 <= t <= 1
```

For every face, determine how far it can progress without exceeding the
distortion bound. Use the minimum permitted value over all faces as the common
`t`. Construct each reference triangle using `inverse(J(t))` and the current
parameterized face. In code, `Mr` is represented by `update_p00`, `update_p01`,
`update_p10`, and `update_p11`, which store per-face inverse edge matrices.

## One update for a frozen reference

Store connected `Mp` coordinates as:

```text
x = [u_0 ... u_(V-1), v_0 ... v_(V-1)]^T
```

Assemble local energy gradients and positive/majorizing Hessian approximations
from all faces, then solve:

```text
H d = -g
```

`d` provides a distinct 2D displacement per vertex. Compute a single global
step `alpha`:

1. Substitute `p(alpha) = p + alpha d` into every signed face area.
2. Find the earliest positive root at which any face becomes degenerate.
3. Start below that global maximum step.
4. Reduce the step until Armijo sufficient decrease holds.

Apply exactly one update for the frozen reference:

```text
Mp_(n+1) = Mp_n + alpha_n d_n
```

Then recompute `Mr_(n+1)` from fixed `M` and `Mp_(n+1)`.

## Full phase sequence

```text
M --Tutte once--> Mp_0

repeat progressive phase:
  Mr_n = progressive_reference(M, Mp_n)
  d_n = one SLIM or CM direction(Mr_n, Mp_n)
  alpha_n = flip-safe Armijo step
  Mp_(n+1) = Mp_n + alpha_n d_n

restore Mr = M
repeat final CM against E(M, Mp) until convergence
```

The progressive phase does not completely minimize each temporary objective.
Do not claim global optimality or monotonic decrease of the true source energy
at every progressive iteration.
