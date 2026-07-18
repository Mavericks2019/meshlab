// simplesquarewidget_parameterization.cpp
#include "simplesquarewidget.h"
#include <QDebug>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <set>
#include <cmath>

// 辅助函数：使用acos计算角度
float SimpleSquareWidget::computeAngle(const Mesh::Point& a, const Mesh::Point& b) const {
    float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    float len1 = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    float len2 = sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
    
    float cos_angle = dot / (len1 * len2 + 1e-10f);
    cos_angle = std::max(-1.0f, std::min(1.0f, cos_angle));
    return acos(cos_angle);
}

// 辅助函数：使用atan2计算角度（与MVector3::vectorAngle一致）
float SimpleSquareWidget::computeVectorAngle(const Mesh::Point& a, const Mesh::Point& b) const {
    float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    
    // 计算叉积的模长
    float cross_x = a[1]*b[2] - b[1]*a[2];
    float cross_y = a[2]*b[0] - b[2]*a[0];
    float cross_z = a[0]*b[1] - b[0]*a[1];
    float sin_angle = sqrt(cross_x*cross_x + cross_y*cross_y + cross_z*cross_z);
    
    return atan2(sin_angle, dot);
}

std::map<int, float> SimpleSquareWidget::computeWeightsForVertex(Mesh::VertexHandle vh, ParameterizationMethod method) {
    std::map<int, float> weights;
    std::vector<Mesh::VertexHandle> neighbors;
    
    for (auto vv_it = original.vv_begin(vh); vv_it != original.vv_end(vh); ++vv_it) {
        neighbors.push_back(*vv_it);
    }
    
    int degree = neighbors.size();
    if (degree == 0) return weights;
    
    switch (method) {
    case UniformTutte:
        for (auto neighbor : neighbors) {
            weights[neighbor.idx()] = 1.0f;
        }
        {
            float sum = degree;
            for (auto& [idx, w] : weights) {
                w /= sum;
            }
        }
        break;
        
    case WeightedTutte:
    {
        auto centerPos = original.point(vh);
        float totalWeight = 0.0f;
        
        for (auto neighbor : neighbors) {
            auto neighborPos = original.point(neighbor);
            float dist = (neighborPos - centerPos).norm();
            float weight = 1.0f / pow(dist + 1e-6f, 2.0f);
            weights[neighbor.idx()] = weight;
            totalWeight += weight;
        }
        
        if (totalWeight > 1e-10) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        } else {
            float uniformWeight = 1.0f / degree;
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = uniformWeight;
            }
        }
    }
    break;
        
    case OriginalMethod:
    default:
        float totalWeight = 0.0f;
        for (auto heh : original.voh_range(vh)) {
            if (!original.is_boundary(heh)) {
                Mesh::VertexHandle vj = original.to_vertex_handle(heh);
                float weight = computeCotangentWeight(heh);
                if (weight > 0) {
                    weights[vj.idx()] = weight;
                    totalWeight += weight;
                }
            }
        }
        
        if (totalWeight > 1e-10) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        } else {
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = 1.0f / neighbors.size();
            }
        }
        break;
    }
    
    float finalSum = 0.0f;
    for (const auto& [idx, w] : weights) {
        if (w < 0) {
            qWarning() << "Warning: Negative weight found at vertex" << vh.idx() 
                       << "for neighbor" << idx << ":" << w;
        }
        finalSum += w;
    }
    
    if (fabs(finalSum - 1.0f) > 0.01f && finalSum > 1e-10) {
        qDebug() << "Weights sum is" << finalSum << "for vertex" << vh.idx() 
                 << ", renormalizing...";
        for (auto& [idx, w] : weights) {
            w /= finalSum;
        }
    }
    
    return weights;
}

float SimpleSquareWidget::computeCotangentWeight(Mesh::HalfedgeHandle heh) {
    if (!openMesh.is_boundary(heh)) {
        Mesh::HalfedgeHandle heh_opp = openMesh.opposite_halfedge_handle(heh);
        Mesh::VertexHandle v0 = openMesh.from_vertex_handle(heh);
        Mesh::VertexHandle v1 = openMesh.to_vertex_handle(heh);
        Mesh::HalfedgeHandle heh_next = openMesh.next_halfedge_handle(heh);
        Mesh::HalfedgeHandle heh_opp_next = openMesh.next_halfedge_handle(heh_opp);
        Mesh::VertexHandle v2 = openMesh.to_vertex_handle(heh_next);
        Mesh::VertexHandle v3 = openMesh.to_vertex_handle(heh_opp_next);
        
        auto p0 = openMesh.point(v0);
        auto p1 = openMesh.point(v1);
        auto p2 = openMesh.point(v2);
        auto p3 = openMesh.point(v3);
        
        auto e01 = p1 - p0;
        auto e02 = p2 - p0;
        auto e10 = p0 - p1;
        auto e13 = p3 - p1;
        
        float cos_alpha = (e01 | e02) / (sqrt((e01 | e01) * (e02 | e02)) + 1e-10);
        float cos_beta = (e10 | e13) / (sqrt((e10 | e10) * (e13 | e13)) + 1e-10);
        
        cos_alpha = std::max(-1.0f, std::min(1.0f, cos_alpha));
        cos_beta = std::max(-1.0f, std::min(1.0f, cos_beta));
        
        float sin_alpha = sqrt(1.0f - cos_alpha * cos_alpha);
        float sin_beta = sqrt(1.0f - cos_beta * cos_beta);
        
        float cot_alpha = cos_alpha / (sin_alpha + 1e-10);
        float cot_beta = cos_beta / (sin_beta + 1e-10);
        
        return (cot_alpha + cot_beta) / 2.0f;
    }
    
    return 0.0f;
}

std::tuple<float, float, float> SimpleSquareWidget::computeBarycentric(float px, float py,
                                                                      float ax, float ay,
                                                                      float bx, float by,
                                                                      float cx, float cy) const {
    float detT = (by - cy)*(ax - cx) + (cx - bx)*(ay - cy);
    float alpha = ((by - cy)*(px - cx) + (cx - bx)*(py - cy)) / detT;
    float beta = ((cy - ay)*(px - cx) + (ax - cx)*(py - cy)) / detT;
    float gamma = 1.0f - alpha - beta;
    return std::make_tuple(alpha, beta, gamma);
}

void SimpleSquareWidget::solveParameterizationInternal(ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }
    
    std::vector<int> internalIndices;
    std::vector<int> boundaryIndices;
    
    for (int i = 0; i < openMesh.n_vertices(); i++) {
        if (isBoundary[i]) {
            boundaryIndices.push_back(i);
        } else {
            internalIndices.push_back(i);
        }
    }
    
    int n_internal = internalIndices.size();
    int n_total = openMesh.n_vertices();
    
    std::vector<int> vertexToInternalIndex(n_total, -1);
    for (int i = 0; i < n_internal; i++) {
        vertexToInternalIndex[internalIndices[i]] = i;
    }
    
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    SpMat A(n_internal, n_internal);
    VectorXf b_u(n_internal), b_v(n_internal);
    VectorXf x(n_internal), y(n_internal);
    
    b_u.setZero();
    b_v.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n_internal * 10);
    
    for (int i = 0; i < n_internal; i++) {
        int vertexIdx = internalIndices[i];
        Mesh::VertexHandle vh(vertexIdx);
        
        auto weights = computeWeightsForVertex(vh, method);
        
        triplets.push_back(Triplet(i, i, 1.0f));
        
        float boundarySumU = 0.0f;
        float boundarySumV = 0.0f;
        
        for (const auto& [neighborIdx, w] : weights) {
            if (!isBoundary[neighborIdx]) {
                int j = vertexToInternalIndex[neighborIdx];
                triplets.push_back(Triplet(i, j, -w));
            } else {
                auto boundaryPos = openMesh.point(Mesh::VertexHandle(neighborIdx));
                boundarySumU += w * boundaryPos[0];
                boundarySumV += w * boundaryPos[1];
            }
        }
        
        b_u(i) = boundarySumU;
        b_v(i) = boundarySumV;
    }
    
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        qWarning() << "Matrix factorization failed!";
        
        if (A.rows() < 10) {
            qDebug() << "Matrix A:";
            Eigen::MatrixXf denseA = A.toDense();
            for (int i = 0; i < denseA.rows(); i++) {
                QString rowStr;
                for (int j = 0; j < denseA.cols(); j++) {
                    rowStr += QString::number(denseA(i, j), 'f', 3) + " ";
                }
                qDebug() << rowStr;
            }
        }
        
        return;
    }
    
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    for (int i = 0; i < n_internal; i++) {
        int vertexIdx = internalIndices[i];
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(vertexIdx), newPos);
    }
    
    qDebug() << "Parameterization solved successfully";
    qDebug() << "Internal vertices count:" << n_internal;
    
    VectorXf residual_u = A * x - b_u;
    VectorXf residual_v = A * y - b_v;
    qDebug() << "Residual norm u:" << residual_u.norm();
    qDebug() << "Residual norm v:" << residual_v.norm();
    
    float min_u = x.minCoeff();
    float max_u = x.maxCoeff();
    float min_v = y.minCoeff();
    float max_v = y.maxCoeff();
    qDebug() << "u range: [" << min_u << ", " << max_u << "]";
    qDebug() << "v range: [" << min_v << ", " << max_v << "]";
}

void SimpleSquareWidget::solveParameterizationFloater() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    qDebug() << "Starting Floater shape-preserving parameterization...";
    int n_vertices = openMesh.n_vertices();
    
    // 保存原始网格坐标（用于计算权重）
    std::vector<Mesh::Point> originalPositions;
    originalPositions.reserve(n_vertices);
    for (auto vh : original.vertices()) {
        originalPositions.push_back(original.point(vh));
    }
    
    // 确定边界顶点和内部顶点
    std::vector<bool> isBoundary(n_vertices, false);
    std::vector<int> boundaryIndices;
    std::vector<int> internalIndices;
    
    for (auto vh : openMesh.vertices()) {
        int idx = vh.idx();
        isBoundary[idx] = openMesh.is_boundary(vh);
        if (isBoundary[idx]) {
            boundaryIndices.push_back(idx);
        } else {
            internalIndices.push_back(idx);
        }
    }
    
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    SpMat A(n_vertices, n_vertices);
    VectorXf Bu(n_vertices), Bv(n_vertices);
    VectorXf Xu(n_vertices), Xv(n_vertices);
    
    Bu.setZero();
    Bv.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n_vertices * 10);
    
    qDebug() << "Building linear system...";
    
    // 第一阶段：使用原始坐标计算权重，同时设置边界条件
    for (auto vh : openMesh.vertices()) {
        int idx = vh.idx();
        
        if (isBoundary[idx]) {
            // 边界顶点：直接固定位置（使用当前openMesh中已经映射的坐标）
            // 这里我们设置方程为 x_i = fixed_value
            triplets.push_back(Triplet(idx, idx, 1.0f));
            
            // 使用当前openMesh中的边界位置（已经在performParameterization中映射）
            auto pos = openMesh.point(vh);
            Bu(idx) = pos[0];
            Bv(idx) = pos[1];
            
        } else {
            // 内部顶点：使用原始坐标计算权重
            std::vector<Mesh::VertexHandle> neighbors;
            for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                neighbors.push_back(*vv_it);
            }
            
            int n_neighbors = neighbors.size();
            
            // 使用原始坐标计算角度
            std::vector<float> angles;
            float total_angle = 0.0f;
            
            // 计算最后一个邻居和第一个邻居的角度
            auto centerPos = originalPositions[idx];
            auto vec1 = originalPositions[neighbors.back().idx()] - centerPos;
            auto vec2 = originalPositions[neighbors.front().idx()] - centerPos;
            float angle = computeAngle(vec1, vec2);
            angles.push_back(angle);
            total_angle += angle;
            
            // 计算其他连续邻居的角度
            for (size_t i = 1; i < neighbors.size(); i++) {
                vec1 = originalPositions[neighbors[i-1].idx()] - centerPos;
                vec2 = originalPositions[neighbors[i].idx()] - centerPos;
                angle = computeAngle(vec1, vec2);
                angles.push_back(angle);
                
                total_angle += angle;
            }
            
            // 角度重新参数化
            std::vector<float> phi;
            for (float angle_val : angles) {
                phi.push_back(2.0f * M_PI * angle_val / total_angle);
            }
            
            // 构建局部二维坐标系（使用原始坐标的距离）
            std::vector<std::pair<float, float>> local_positions;
            
            // 第一个邻居
            Mesh::VertexHandle first_neighbor = neighbors[0];
            auto first_pos = originalPositions[first_neighbor.idx()];
            float first_dist = (first_pos - centerPos).norm();
            local_positions.push_back({first_dist, 0.0f});
            
            float current_angle = 0.0f;
            for (size_t i = 1; i < phi.size(); i++) {
                current_angle += phi[i];
                Mesh::VertexHandle neighbor = neighbors[i];
                auto neighbor_pos = originalPositions[neighbor.idx()];
                float dist = (neighbor_pos - centerPos).norm();
                
                float x = dist * cos(current_angle);
                float y = dist * sin(current_angle);
                local_positions.push_back({x, y});
            }
            
            // 计算均值坐标权重矩阵
            Eigen::MatrixXf Mu = Eigen::MatrixXf::Zero(n_neighbors, n_neighbors);
            
            for (int l = 0; l < n_neighbors; l++) {
                bool found_triangle = false;
                
                for (int k = 1; k < n_neighbors; k++) {
                    int k_minus_1 = k - 1;
                    
                    if (k_minus_1 == l || k == l) continue;
                    
                    auto A = local_positions[l];
                    auto B = local_positions[k_minus_1];
                    auto C = local_positions[k];
                    
                    Eigen::Vector2f AB(B.first - A.first, B.second - A.second);
                    Eigen::Vector2f AC(C.first - A.first, C.second - A.second);
                    Eigen::Vector2f AP(0.0f - A.first, 0.0f - A.second);
                    Eigen::Vector2f BP(0.0f - B.first, 0.0f - B.second);
                    Eigen::Vector2f CP(0.0f - C.first, 0.0f - C.second);
                    
                    float area_ABC = fabs(AB.x() * AC.y() - AB.y() * AC.x());
                    if (area_ABC < 1e-10f) continue;
                    
                    float area_PBC = fabs(BP.x() * CP.y() - BP.y() * CP.x());
                    float area_APC = fabs(AP.x() * CP.y() - AP.y() * CP.x());
                    float area_ABP = fabs(AB.x() * AP.y() - AB.y() * AP.x());
                    
                    float a = area_PBC / area_ABC;
                    float b = area_APC / area_ABC;
                    float c = area_ABP / area_ABC;
                    
                    const float eps = 1e-6f;
                    if (a >= -eps && b >= -eps && c >= -eps && 
                        fabs(a + b + c - 1.0f) < 0.01f) {
                        
                        Mu(l, l) = a;
                        Mu(k_minus_1, l) = b;
                        Mu(k, l) = c;
                        found_triangle = true;
                        break;
                    }
                }
                
                if (!found_triangle) {
                    float uniform_weight = 1.0f / n_neighbors;
                    for (int n = 0; n < n_neighbors; n++) {
                        Mu(n, l) = uniform_weight;
                    }
                }
            }
            
            Eigen::VectorXf row_sums = Mu.rowwise().sum();
            std::map<int, float> weights;
            
            bool has_invalid = false;
            for (int n = 0; n < n_neighbors; n++) {
                if (row_sums(n) < 1e-10f) {
                    has_invalid = true;
                    break;
                }
            }
            
            if (has_invalid) {
                float uniform_weight = 1.0f / n_neighbors;
                for (int n = 0; n < n_neighbors; n++) {
                    weights[neighbors[n].idx()] = uniform_weight;
                }
            } else {
                for (int n = 0; n < n_neighbors; n++) {
                    float weight = row_sums(n) / n_neighbors;
                    weights[neighbors[n].idx()] = weight;
                }
            }
            
            float sum_weight = 0.0f;
            for (const auto& [neighbor_idx, w] : weights) {
                triplets.push_back(Triplet(idx, neighbor_idx, w));
                sum_weight += w;
            }
            
            triplets.push_back(Triplet(idx, idx, -sum_weight));
            
            // 右端项为0（因为方程是：sum(w_ij * x_j) - x_i = 0）
            Bu(idx) = 0.0f;
            Bv(idx) = 0.0f;
        }
    }
    
    // 构建稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    qDebug() << "Linear system built. Matrix size:" << A.rows() << "x" << A.cols();
    qDebug() << "Non-zero elements:" << A.nonZeros();
    
    // 求解线性方程组
    SparseLU<SpMat> solver;
    solver.compute(A);
    
    if (solver.info() != Success) {
        qWarning() << "SparseLU decomposition failed! Error code:" << solver.info();
        
        qDebug() << "Trying BiCGSTAB solver...";
        BiCGSTAB<SpMat> backup_solver;
        backup_solver.compute(A);
        
        if (backup_solver.info() == Success) {
            Xu = backup_solver.solve(Bu);
            Xv = backup_solver.solve(Bv);
            qDebug() << "BiCGSTAB solver succeeded";
        } else {
            qWarning() << "All solvers failed! Using uniform parameterization as fallback";
            solveParameterizationInternal(UniformTutte);
            return;
        }
    } else {
        Xu = solver.solve(Bu);
        Xv = solver.solve(Bv);
        qDebug() << "SparseLU solver succeeded";
    }
    
    // 更新所有顶点的位置（包括边界）
    int internal_count = 0;
    for (auto vh : openMesh.vertices()) {
        int idx = vh.idx();
        Mesh::Point newPos(Xu(idx), Xv(idx), 0.0f);
        openMesh.set_point(vh, newPos);
        if (!isBoundary[idx]) {
            internal_count++;
        }
    }
    
    VectorXf residual_u = A * Xu - Bu;
    VectorXf residual_v = A * Xv - Bv;
    
    qDebug() << "Floater parameterization completed successfully";
    qDebug() << "Internal vertices updated:" << internal_count;
    qDebug() << "Residual norm u:" << residual_u.norm();
    qDebug() << "Residual norm v:" << residual_v.norm();
    
    float min_u = Xu.minCoeff();
    float max_u = Xu.maxCoeff();
    float min_v = Xv.minCoeff();
    float max_v = Xv.maxCoeff();
    qDebug() << "u coordinate range: [" << min_u << ", " << max_u << "]";
    qDebug() << "v coordinate range: [" << min_v << ", " << max_v << "]";
}

void SimpleSquareWidget::mapBoundaryToCircle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    while(now != start_v) {
        boundary.push_back(now);
        Mesh::VertexHandle next;
        for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
            if(openMesh.is_boundary(*vv_it) && *vv_it != pre) {
                next = *vv_it;
                break;
            }
        }
        pre = now;
        now = next;
    }

    float arc_len = 0.0f;
    for(int i = 1; i < boundary.size(); ++i) {
        arc_len += (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
    }
    arc_len += (openMesh.point(boundary[boundary.size()-1]) - openMesh.point(boundary[0])).norm();
    
    std::vector<float> delta;
    for(int i = 1; i < boundary.size(); ++i) {
        float seg_len = (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
        delta.push_back(2.0f * M_PI * (seg_len / arc_len));
    }
    
    float angle_now = 0.0f;
    for(size_t i = 0; i < boundary.size(); ++i) {
        float x = cos(angle_now);
        float y = sin(angle_now);
        openMesh.set_point(boundary[i], Mesh::Point(x, y, 0));
        if(i < boundary.size() - 1) {
            angle_now += delta[i];
        }
    }
}

void SimpleSquareWidget::mapBoundaryToRectangle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    while(now != start_v) {
        boundary.push_back(now);
        Mesh::VertexHandle next;
        for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
            if(openMesh.is_boundary(*vv_it) && *vv_it != pre) {
                next = *vv_it;
                break;
            }
        }
        pre = now;
        now = next;
    }

    const int n = boundary.size();
    const float length = 1.0f;
    
    int side1 = n / 4;
    int side2 = n / 4;
    int side3 = n / 4;
    int side4 = n - 3 * (n / 4);
    
    openMesh.set_point(boundary[0], Mesh::Point(0.0f, 0.0f, 0.0f));
    openMesh.set_point(boundary[side1], Mesh::Point(0.0f, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2], Mesh::Point(length, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2 + side3], Mesh::Point(length, 0.0f, 0.0f));
    
    float delta = length / side1;
    for (int i = 1; i < side1; ++i) {
        float y = i * delta;
        openMesh.set_point(boundary[i], Mesh::Point(0.0f, y, 0.0f));
    }
    
    delta = length / side2;
    for (int i = 1; i < side2; ++i) {
        int idx = side1 + i;
        float x = i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, length, 0.0f));
    }
    
    delta = length / side3;
    for (int i = 1; i < side3; ++i) {
        int idx = side1 + side2 + i;
        float y = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(length, y, 0.0f));
    }
    
    delta = length / side4;
    for (int i = 1; i < side4; ++i) {
        int idx = side1 + side2 + side3 + i;
        float x = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, 0.0f, 0.0f));
    }
}

void SimpleSquareWidget::normalizeMesh() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    Mesh::Point min(1e9, 1e9, 0), max(-1e9, -1e9, 0);
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        min[0] = std::min(min[0], p[0]);
        min[1] = std::min(min[1], p[1]);
        max[0] = std::max(max[0], p[0]);
        max[1] = std::max(max[1], p[1]);
    }
    
    Mesh::Point center((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, 0);
    float range_x = max[0] - min[0];
    float range_y = max[1] - min[1];
    
    int viewWidth = width();
    int viewHeight = height();
    float aspectRatio = static_cast<float>(viewWidth) / viewHeight;
    
    float scaleFactor;
    if (aspectRatio > 1.0f) {
        scaleFactor = 2.0f / (range_y > 0 ? range_y : 1.0f);
    } else {
        scaleFactor = 2.0f / (range_x > 0 ? range_x : 1.0f);
    }
    
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        p -= center;
        p[0] *= scaleFactor;
        p[1] *= scaleFactor;
        p[2] = 0.0f;
        openMesh.set_point(vh, p);
    }
}

void SimpleSquareWidget::solveParameterizationOriginal() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }

    std::vector<std::map<int, float>> weights(openMesh.n_vertices());
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        for (auto heh : openMesh.voh_range(vh)) {
            if (!openMesh.is_boundary(heh)) {
                auto vj = openMesh.to_vertex_handle(heh);
                int j = vj.idx();
                
                auto from = openMesh.from_vertex_handle(heh);
                auto to = openMesh.to_vertex_handle(heh);
                auto next = openMesh.next_halfedge_handle(heh);
                auto opp_next = openMesh.next_halfedge_handle(openMesh.opposite_halfedge_handle(heh));
                
                auto p1 = openMesh.point(from);
                auto p2 = openMesh.point(to);
                auto p3 = openMesh.point(openMesh.to_vertex_handle(next));
                auto p4 = openMesh.point(openMesh.to_vertex_handle(opp_next));
                
                Eigen::Vector3f v1(static_cast<float>(p1[0]-p2[0]), static_cast<float>(p1[1]-p2[1]), static_cast<float>(p1[2]-p2[2]));
                Eigen::Vector3f v2(static_cast<float>(p3[0]-p2[0]), static_cast<float>(p3[1]-p2[1]), static_cast<float>(p3[2]-p2[2]));
                Eigen::Vector3f v3(static_cast<float>(p4[0]-p2[0]), static_cast<float>(p4[1]-p2[1]), static_cast<float>(p4[2]-p2[2]));
                
                float angle1 = acos(v1.dot(v2) / (v1.norm() * v2.norm()));
                float angle2 = acos(v1.dot(v3) / (v1.norm() * v3.norm()));
                
                float w1 = 1.0f / tan(angle1);
                float w2 = 1.0f / tan(angle2);
                float w = (w1 + w2) / 2.0f;
                
                weights[i][j] = w;
            }
        }
    }

    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);
    VectorXf b_u(n), b_v(n);
    VectorXf x(n), y(n);
    
    b_u.setZero();
    b_v.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n * 10);
    
    for (int i = 0; i < n; i++) {
        if (isBoundary[i]) {
            triplets.push_back(Triplet(i, i, 1.0f));
            b_u[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            b_v[i] = openMesh.point(Mesh::VertexHandle(i))[1];
        } else {
            float totalWeight = 0.0f;
            for (const auto& [j, w] : weights[i]) {
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            triplets.push_back(Triplet(i, i, -totalWeight));
            b_u[i] = 0.0f;
            b_v[i] = 0.0f;
        }
    }
    
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        std::cerr << "Matrix factorization failed!" << std::endl;
        return;
    }
    
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    for (int i = 0; i < n; i++) {
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(i), newPos);
    }
}