#pragma once

// Minimal ANN 1.1.2-compatible facade over nanoflann for this project.

#include <cstddef>

using ANNcoord = double;
using ANNdist = double;
using ANNidx = int;

using ANNpoint = ANNcoord*;
using ANNpointArray = ANNpoint*;
using ANNidxArray = ANNidx*;
using ANNdistArray = ANNdist*;

ANNpoint annAllocPt(int dim, ANNcoord fill = 0.0);
ANNpointArray annAllocPts(int n, int dim);
void annDeallocPt(ANNpoint& point);
void annDeallocPts(ANNpointArray& points);
void annClose();

class ANNkd_tree {
public:
    ANNkd_tree(ANNpointArray points, int n, int dim);
    ~ANNkd_tree();

    void annkSearch(ANNpoint query, int k, ANNidxArray nnIdx, ANNdistArray dists,
                    double eps = 0.0) const;

private:
    struct Impl;
    Impl* impl_;
};
