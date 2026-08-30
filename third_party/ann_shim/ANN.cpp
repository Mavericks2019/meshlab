#include "ANN.h"

#include <nanoflann.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

struct PointCloud {
    std::vector<ANNcoord> data;
    int dim = 3;
    int size = 0;

    inline std::size_t kdtree_get_point_count() const
    {
        return static_cast<std::size_t>(size);
    }

    inline ANNcoord kdtree_get_pt(std::size_t index, std::size_t coordinate) const
    {
        return data[index * static_cast<std::size_t>(dim) + coordinate];
    }

    template <class BBox>
    bool kdtree_get_bbox(BBox&) const
    {
        return false;
    }
};

using KdTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<ANNcoord, PointCloud>,
    PointCloud,
    3>;

} // namespace

struct ANNkd_tree::Impl {
    PointCloud cloud;
    std::unique_ptr<KdTree> tree;
};

ANNpoint annAllocPt(int dim, ANNcoord fill)
{
    auto* point = new ANNcoord[static_cast<std::size_t>(dim)];
    std::fill(point, point + dim, fill);
    return point;
}

ANNpointArray annAllocPts(int n, int dim)
{
    auto** points = new ANNpoint[static_cast<std::size_t>(n)];
    for (int i = 0; i < n; ++i)
        points[i] = annAllocPt(dim);
    return points;
}

void annDeallocPt(ANNpoint& point)
{
    delete[] point;
    point = nullptr;
}

void annDeallocPts(ANNpointArray& points)
{
    delete[] points;
    points = nullptr;
}

void annClose()
{
}

ANNkd_tree::ANNkd_tree(ANNpointArray points, int n, int dim)
    : impl_(new Impl)
{
    if (dim != 3)
        throw std::invalid_argument("ann_shim only supports 3D points");
    impl_->cloud.dim = dim;
    impl_->cloud.size = n;
    impl_->cloud.data.resize(static_cast<std::size_t>(n) * static_cast<std::size_t>(dim));
    for (int i = 0; i < n; ++i)
        for (int d = 0; d < dim; ++d)
            impl_->cloud.data[static_cast<std::size_t>(i * dim + d)] = points[i][d];
    impl_->tree = std::make_unique<KdTree>(
        3, impl_->cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    impl_->tree->buildIndex();
}

ANNkd_tree::~ANNkd_tree()
{
    delete impl_;
    impl_ = nullptr;
}

void ANNkd_tree::annkSearch(ANNpoint query, int k, ANNidxArray nnIdx,
                            ANNdistArray dists, double) const
{
    std::vector<KdTree::IndexType> indices(static_cast<std::size_t>(k), 0);
    std::vector<ANNcoord> distanceSquared(static_cast<std::size_t>(k), 0.0);
    (void)impl_->tree->knnSearch(
        query, static_cast<KdTree::Size>(k), indices.data(), distanceSquared.data());
    for (int i = 0; i < k; ++i) {
        nnIdx[i] = static_cast<ANNidx>(indices[static_cast<std::size_t>(i)]);
        dists[i] = distanceSquared[static_cast<std::size_t>(i)];
    }
}
