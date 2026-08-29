# D-Charts Algorithm Reference

Read this reference for conceptual explanations, formula derivations, parameter
experiments, or changes intended to match the paper more closely.

## Source and objective

The source is Dan Julius, Vladislav Kraevoy, and Alla Sheffer,
"D-Charts: Quasi-Developable Mesh Segmentation," Eurographics Symposium on
Geometry Processing, 2005. The local copy used for this implementation is:

```text
C:/Users/10156/Downloads/EG05.pdf
```

The method partitions a triangle mesh into charts that are approximately
developable. A chart should have normals close to a circle on the Gaussian
sphere, remain compact, and have a simple boundary. The result is a segmentation
and a set of cut candidates, not a parameterization by itself.

## Cone proxy and fitting error

For a face `t` with unit normal `n_t`, chart `C` uses proxy:

```text
P_C = (N_C, theta_C),  ||N_C|| = 1
c_C = cos(theta_C)
```

The fitting error is:

```text
F(C, t) = (N_C dot n_t - c_C)^2
```

Planes, cones, and cylinders are covered by the same representation. A cylinder
has `theta = pi/2`, hence `c = 0`.

For chart faces with triangle areas `A_t`, fit the proxy by minimizing:

```text
minimize  sum_t A_t (N dot n_t - c)^2
subject to ||N|| = 1
```

For a fixed `N`, the optimal scalar is:

```text
c = N dot n_bar
n_bar = (sum_t A_t n_t) / (sum_t A_t)
```

Substitution gives `N^T Cov(n) N`. Therefore the minimizing `N` is the unit
eigenvector associated with the smallest eigenvalue of the area-weighted normal
covariance matrix. `fitProxy()` uses this closed form. It is mathematically
equivalent to the constrained quadratic objective even though the paper
describes a Newton solve.

## Growth cost

The paper combines fitting, compactness, and boundary terms:

```text
cost(C, t) = F(C,t)^alpha * Compact(C,t)^beta * Boundary(C,t)^gamma

Compact(C,t) = pi * D(S_C,t)^2 / A_C
Boundary(C,t) = l_outer / l_inner
```

`S_C` is the chart seed, `D` is distance over the face adjacency graph, `A_C`
is current chart area, `l_inner` is candidate-edge length touching the chart,
and `l_outer` is the remaining candidate boundary length. Smaller cost is
preferred, but a candidate is accepted only if `F(C,t) < Fmax`.

The paper's reported default exponents, also used here, are:

```text
alpha = 1.0
beta  = 0.7
gamma = 0.5
```

## Phase sequence

```text
triangulate and build face adjacency
choose farthest-point seed faces
initialize a local cone proxy around each seed

repeat Lloyd phase:
  grow all charts greedily with the F/C/P cost and Fmax
  remove empty charts
  refit every proxy
  choose a new representative seed for every chart
until fewer than 5% of faces change or maxIterations is reached

fill unassigned components
straighten chart boundaries
merge adjacent charts across sufficiently cylindrical boundaries
add partial cuts toward remaining high-error regions
emit final labels and seam edges
```

### Seeds

`farthestSeeds()` uses repeated dual-graph distances to spread initial seeds.
For a recomputed chart, `selectSeed()` first keeps up to ten faces with lowest
proxy error and then selects the candidate closest to the chart's area-weighted
centroid. This follows the paper's low-error, central representative idea.

### Lloyd growth

The proxy stays fixed during one growth round. After the round, labels define
new charts, proxies and seeds are recomputed, and the next round starts from
those seeds. Convergence is based on the fraction of faces whose labels changed,
not on fitting energy alone.

### Unassigned components

The `Fmax` condition can leave holes. Small components grow from neighboring
charts without the fitting threshold. Large components receive a new fitted
chart, then any remaining pieces are processed again. This ensures a complete
labeling while retaining the role of `Fmax` during the main Lloyd phase.

### Boundary straightening

The paper constructs a fuzzy band around each boundary and finds a shorter path
through it while respecting fitting constraints. The current implementation
does not reproduce that construction exactly. `relaxBoundaries()` moves
individual boundary faces only when the local boundary becomes shorter, the new
proxy satisfies `Fmax`, the source chart remains connected, and a seed is not
removed.

When improving paper fidelity, replace this phase with an explicit fuzzy-region
shortest-path or graph-cut formulation. Keep it isolated from proxy fitting and
Lloyd growth so results can be compared phase by phase.

### Developable merging

For each adjacent chart pair, collect faces near their common boundary and fit a
cylindrical proxy (`c = 0`). Merge the pair when its area-weighted residual is
below `mergeThreshold`. The paper suggests roughly `1e-5` for mechanical models
and `1e-2` for less exact organic models. The UI default is `0.01`.

### Partial cuts

After merging, connected high-error face regions are detected. For each region,
the implementation chooses its maximum-error face and adds a shortest primal
edge path from a chart boundary to that face. These paths are displayed as seam
edges. They do not alter topology; an actual cutting pipeline must duplicate
vertices and update face connectivity separately.

## Parameters and experiments

| Parameter | Effect when increased |
|---|---|
| `initialChartCount` | More starting basins; often more detail and runtime |
| `fittingThreshold` / `Fmax` | Looser main growth; fewer holes but less developable charts |
| `alpha` | More preference for normal-cone fit |
| `beta` | More preference for compact charts |
| `gamma` | More preference for simple local boundaries |
| `convergenceRatio` | Earlier Lloyd termination |
| `largeHoleAreaRatio` | Fewer unassigned components spawn new charts |
| `boundaryRelaxationPasses` | More opportunities for local seam shortening |
| `mergeThreshold` | More aggressive cylindrical merging |

Change one variable at a time. Record chart count, Lloyd iterations, assigned
faces, mean/max fitting error, and seam appearance at every phase. A useful
study sequence is: disable merging, disable partial cuts, vary `Fmax`, then vary
the three cost exponents. This separates region-growth behavior from cleanup.

## Interpretation cautions

- `F` is a squared normal-space residual and has no direct length unit.
- Low mean `F` does not imply good chart topology, compactness, or UV distortion.
- Final maximum `F` may exceed `Fmax` after hole filling or merging.
- Colored charts show labels, not distortion values.
- A good D-Charts segmentation still needs a cutting/topology step and a UV
  parameterizer before it becomes a texture atlas.
