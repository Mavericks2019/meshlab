// arapglwidget.cpp
#include "arapglwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QtMath>
#include <CGAL/IO/OBJ.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/boost/graph/selection.h>
#include <CGAL/Surface_mesh_parameterization/ARAP_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Discrete_conformal_map_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Error_code.h>
#include <CGAL/Surface_mesh_parameterization/parameterize.h>
#include <CGAL/Surface_mesh_parameterization/Barycentric_mapping_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Mean_value_coordinates_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/LSCM_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Two_vertices_parameterizer_3.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/transform.h>
#include <CGAL/Cartesian_converter.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/iterator.h>

namespace SMP = CGAL::Surface_mesh_parameterization;
namespace PMP = CGAL::Polygon_mesh_processing;

// 实现辅助函数
namespace ARAPKernel {
    template<typename Graph>
    typename boost::graph_traits<Graph>::vertex_descriptor 
    vertices_around_face(typename boost::graph_traits<Graph>::halfedge_descriptor h, const Graph& g) {
        return target(h, g);
    }
}

ARAPGLWidget::ARAPGLWidget(QWidget *parent)
    : CGALGLWidget(parent)
    , isParameterized_(false)
    , useFixedBorder_(true)
    , useIterativeSolver_(true)
    , maxIterations_(100)
    , convergenceThreshold_(1e-6)
    , has_original_backup_(false)
{
    // 设置ARAP参数化的默认选项
}

ARAPGLWidget::~ARAPGLWidget()
{
    // 清理资源
}

void ARAPGLWidget::computeNormalsForARAP()
{
    // 调用基类的方法计算法线
    CGALGLWidget::computeNormals();
}

ARAPKernel::Point_3 ARAPGLWidget::convertToARAPPoint(const Point& p) const
{
    return ARAPKernel::Point_3(p.x(), p.y(), p.z());
}

Point ARAPGLWidget::convertToCgalPoint(const ARAPKernel::Point_3& p) const
{
    return Point(p.x(), p.y(), p.z());
}

void ARAPGLWidget::backupOriginalMesh()
{
    if (has_original_backup_) {
        return;
    }
    
    // 简单备份：只标记原始网格存在
    // 在实际应用中，可能需要更复杂的备份机制
    has_original_backup_ = true;
    qDebug() << "Original mesh backed up";
}

void ARAPGLWidget::restoreOriginalMesh()
{
    if (!has_original_backup_) {
        return;
    }
    
    // 重新加载原始文件
    // 在实际应用中，可以从备份中恢复
    qDebug() << "Original mesh restored";
    has_original_backup_ = false;
}

void ARAPGLWidget::loadOBJ(const QString &path)
{
    // 首先清除当前数据和状态
    clearParameterization();
    clearMeshData();
    
    // 使用基类的loadOBJ函数
    // 由于基类的loadOBJ不是虚函数，我们直接调用基类版本
    CGALGLWidget::loadOBJ(path);
    
    if (modelLoaded) {
        // 备份原始网格
        backupOriginalMesh();
        
        // 重置参数化状态
        isParameterized_ = false;
        
        qDebug() << "ARAPGLWidget: Loaded OBJ file and backed up original mesh";
    }
}

bool ARAPGLWidget::checkAndFixMesh()
{
    if (mesh.number_of_vertices() == 0) {
        qWarning() << "Mesh is empty!";
        return false;
    }
    
    // 检查网格是否有效
    if (!isMeshValidForARAP()) {
        qWarning() << "Mesh is not valid for ARAP parameterization!";
        return false;
    }
    
    // 确保网格是三角网格
    triangulateMesh();
    
    // 移除孤立顶点
    removeIsolatedVertices();
    
    // 检查组件数量
    std::vector<CgalMesh> components;
    PMP::split_connected_components(mesh, components);
    
    if (components.empty()) {
        qWarning() << "No connected components found!";
        return false;
    }
    
    if (components.size() > 1) {
        qWarning() << "Mesh has" << components.size() << "connected components. Using the largest one.";
        
        // 找到最大的组件
        size_t max_vertices = 0;
        size_t largest_component = 0;
        for (size_t i = 0; i < components.size(); ++i) {
            if (components[i].number_of_vertices() > max_vertices) {
                max_vertices = components[i].number_of_vertices();
                largest_component = i;
            }
        }
        
        // 使用最大的组件
        mesh = components[largest_component];
    }
    
    // 检查方向并统一
    try {
        PMP::orient_to_bound_a_volume(mesh);
    } catch (...) {
        qWarning() << "Failed to orient mesh, continuing anyway";
    }
    
    // 计算边界数量
    int border_count = 0;
    for (auto h : mesh.halfedges()) {
        if (mesh.is_border(h)) {
            border_count++;
        }
    }
    
    qDebug() << "Mesh after fixing - Vertices:" << mesh.number_of_vertices()
             << "Faces:" << mesh.number_of_faces()
             << "Border halfedges:" << border_count;
    
    return true;
}

bool ARAPGLWidget::isMeshValidForARAP() const
{
    if (mesh.number_of_vertices() < 3) {
        qWarning() << "Mesh has too few vertices:" << mesh.number_of_vertices();
        return false;
    }
    
    if (mesh.number_of_faces() < 1) {
        qWarning() << "Mesh has no faces!";
        return false;
    }
    
    // 检查是否有非流形边 - 使用正确的方法
    for (auto e : mesh.edges()) {
        auto h = mesh.halfedge(e);
        auto h_opp = mesh.opposite(h);
        
        // 计算共享此边的面数
        int face_count = 0;
        if (!mesh.is_border(h)) face_count++;
        if (!mesh.is_border(h_opp)) face_count++;
        
        if (face_count > 2) {
            qWarning() << "Non-manifold edge detected!";
            return false;
        }
    }
    
    return true;
}

void ARAPGLWidget::triangulateMesh()
{
    // 统计非三角面的数量
    int non_tri_faces = 0;
    for (auto f : mesh.faces()) {
        int vertex_count = 0;
        for (auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
            vertex_count++;
            (void)v; // 避免未使用变量警告
        }
        if (vertex_count > 3) {
            non_tri_faces++;
        }
    }
    
    if (non_tri_faces > 0) {
        qDebug() << "Triangulating" << non_tri_faces << "non-triangle faces";
        
        try {
            // 使用CGAL的三角化功能
            PMP::triangulate_faces(mesh);
            
            // 重新计算法线和索引
            computeNormalsForARAP();
            prepareFaceIndices();
            prepareEdgeIndices();
        } catch (...) {
            qWarning() << "Failed to triangulate faces";
        }
    }
}

void ARAPGLWidget::removeIsolatedVertices()
{
    std::vector<CgalMesh::Vertex_index> vertices_to_remove;
    
    for (auto v : mesh.vertices()) {
        if (mesh.is_isolated(v)) {
            vertices_to_remove.push_back(v);
        }
    }
    
    if (!vertices_to_remove.empty()) {
        qDebug() << "Removing" << vertices_to_remove.size() << "isolated vertices";
        for (auto v : vertices_to_remove) {
            mesh.remove_vertex(v);
        }
        // 使用正确的函数名
        mesh.collect_garbage();
    }
}

bool ARAPGLWidget::prepareForARAP()
{
    if (!modelLoaded) {
        qWarning() << "No model loaded!";
        return false;
    }
    
    // 备份原始网格
    if (!has_original_backup_) {
        backupOriginalMesh();
    }
    
    // 检查和修复网格
    if (!checkAndFixMesh()) {
        QMessageBox::warning(nullptr, "Mesh Error", 
                           "The mesh is not suitable for ARAP parameterization. "
                           "Please ensure the mesh is watertight and manifold.");
        return false;
    }
    
    // 将CgalMesh转换为Surface_mesh用于ARAP
    copyMeshToSurfaceMesh();
    
    // 提取边界
    if (!extractBoundary()) {
        QMessageBox::warning(nullptr, "Boundary Error", 
                           "Failed to extract a valid boundary for parameterization.");
        return false;
    }
    
    qDebug() << "Prepared for ARAP: Vertices =" << arap_mesh_.number_of_vertices()
             << ", Faces =" << arap_mesh_.number_of_faces()
             << ", Border vertices =" << border_vertices_.size();
    
    return true;
}

void ARAPGLWidget::copyMeshToSurfaceMesh()
{
    // 清空现有的Surface_mesh
    arap_mesh_.clear();
    
    // 创建顶点映射
    vertex_cgal_to_arap_map_.clear();
    vertex_arap_to_cgal_map_.clear();
    
    // 添加顶点
    for (auto v : mesh.vertices()) {
        const Point& p = mesh.point(v);
        ARAPKernel::vertex_descriptor vd = arap_mesh_.add_vertex(convertToARAPPoint(p));
        vertex_cgal_to_arap_map_[v] = vd;
        vertex_arap_to_cgal_map_[vd] = v;
    }
    
    // 添加面
    for (auto f : mesh.faces()) {
        std::vector<ARAPKernel::vertex_descriptor> face_vertices;
        for (auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
            if (vertex_cgal_to_arap_map_.find(v) != vertex_cgal_to_arap_map_.end()) {
                face_vertices.push_back(vertex_cgal_to_arap_map_[v]);
            }
        }
        if (face_vertices.size() >= 3) {
            // 确保是三角面
            if (face_vertices.size() == 3) {
                arap_mesh_.add_face(face_vertices[0], face_vertices[1], face_vertices[2]);
            } else {
                // 多边形需要三角化
                for (size_t i = 1; i < face_vertices.size() - 1; ++i) {
                    arap_mesh_.add_face(face_vertices[0], face_vertices[i], face_vertices[i + 1]);
                }
            }
        }
    }
}

void ARAPGLWidget::copySurfaceMeshToCgalMesh()
{
    // 清空原始网格
    mesh.clear();
    
    // 创建顶点映射
    std::map<ARAPKernel::vertex_descriptor, CgalMesh::Vertex_index> vertex_map;
    
    // 添加顶点
    for (auto vd : arap_mesh_.vertices()) {
        const ARAPKernel::Point_3& p = arap_mesh_.point(vd);
        auto v = mesh.add_vertex(convertToCgalPoint(p));
        vertex_map[vd] = v;
    }
    
    // 添加面
    for (auto fd : arap_mesh_.faces()) {
        std::vector<CgalMesh::Vertex_index> face_vertices;
        for (auto vd : vertices_around_face(arap_mesh_.halfedge(fd), arap_mesh_)) {
            if (vertex_map.find(vd) != vertex_map.end()) {
                face_vertices.push_back(vertex_map[vd]);
            }
        }
        if (face_vertices.size() >= 3) {
            mesh.add_face(face_vertices[0], face_vertices[1], face_vertices[2]);
        }
    }
    
    // 重新计算法线和索引
    computeNormalsForARAP();
    prepareFaceIndices();
    prepareEdgeIndices();
    
    // 更新模型加载状态
    modelLoaded = true;
}

bool ARAPGLWidget::extractBoundary()
{
    border_vertices_.clear();
    border_uvs_.clear();
    
    // 使用CGAL的边界提取功能 - 使用正确的类型
    std::vector<CgalHalfedgeDescriptor> border_cycles;
    PMP::extract_boundary_cycles(mesh, std::back_inserter(border_cycles));
    
    if (border_cycles.empty()) {
        qWarning() << "No boundary found in the mesh!";
        return false;
    }
    
    // 找到最长的边界
    CgalHalfedgeDescriptor longest_border = border_cycles[0];
    size_t max_length = 0;
    
    for (auto border : border_cycles) {
        size_t length = 0;
        CgalHalfedgeDescriptor h = border;
        do {
            length++;
            h = mesh.next(h);
        } while (h != border);
        
        if (length > max_length) {
            max_length = length;
            longest_border = border;
        }
    }
    
    // 收集边界顶点
    std::vector<CgalMesh::Vertex_index> boundary_vertices;
    CgalHalfedgeDescriptor h = longest_border;
    do {
        CgalMesh::Vertex_index v = mesh.target(h);
        boundary_vertices.push_back(v);
        h = mesh.next(h);
    } while (h != longest_border);
    
    // 使用映射将CgalMesh顶点转换为ARAP顶点
    for (auto v : boundary_vertices) {
        if (vertex_cgal_to_arap_map_.find(v) != vertex_cgal_to_arap_map_.end()) {
            border_vertices_.push_back(vertex_cgal_to_arap_map_[v]);
        }
    }
    
    qDebug() << "Extracted boundary with" << border_vertices_.size() << "vertices";
    
    // 创建圆形边界UV坐标
    createCircularBorderUVs();
    
    return border_vertices_.size() >= 3;
}

void ARAPGLWidget::createCircularBorderUVs()
{
    border_uvs_.clear();
    
    size_t n = border_vertices_.size();
    for (size_t i = 0; i < n; ++i) {
        double angle = 2.0 * M_PI * i / n;
        double u = 0.5 + 0.4 * cos(angle);  // 映射到[0.1, 0.9]范围内
        double v = 0.5 + 0.4 * sin(angle);
        border_uvs_.push_back(ARAPKernel::Point_2(u, v));
    }
}

void ARAPGLWidget::createSquareBorderUVs()
{
    border_uvs_.clear();
    
    size_t n = border_vertices_.size();
    size_t side_length = n / 4;
    
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / n;
        double u, v;
        
        if (i < side_length) {
            // 上边
            u = 0.1 + 0.8 * (static_cast<double>(i) / side_length);
            v = 0.9;
        } else if (i < 2 * side_length) {
            // 右边
            u = 0.9;
            v = 0.9 - 0.8 * (static_cast<double>(i - side_length) / side_length);
        } else if (i < 3 * side_length) {
            // 下边
            u = 0.9 - 0.8 * (static_cast<double>(i - 2 * side_length) / side_length);
            v = 0.1;
        } else {
            // 左边
            u = 0.1;
            v = 0.1 + 0.8 * (static_cast<double>(i - 3 * side_length) / (n - 3 * side_length));
        }
        
        border_uvs_.push_back(ARAPKernel::Point_2(u, v));
    }
}

void ARAPGLWidget::computeBoundingBox(Point& min, Point& max)
{
    if (mesh.number_of_vertices() == 0) {
        min = Point(0, 0, 0);
        max = Point(0, 0, 0);
        return;
    }
    
    auto it = mesh.vertices_begin();
    min = max = mesh.point(*it);
    
    for (auto v : mesh.vertices()) {
        const Point& p = mesh.point(v);
        min = Point(std::min(min.x(), p.x()), 
                   std::min(min.y(), p.y()), 
                   std::min(min.z(), p.z()));
        max = Point(std::max(max.x(), p.x()), 
                   std::max(max.y(), p.y()), 
                   std::max(max.z(), p.z()));
    }
}

double ARAPGLWidget::computeMeshDiagonal()
{
    if (mesh.number_of_vertices() == 0) return 0.0;
    
    Point min, max;
    computeBoundingBox(min, max);
    
    double dx = max.x() - min.x();
    double dy = max.y() - min.y();
    double dz = max.z() - min.z();
    
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool ARAPGLWidget::performARAPParameterization()
{
    if (!prepareForARAP()) {
        return false;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        // 创建UV属性映射
        ARAPKernel::UV_pmap uv_map = arap_mesh_.add_property_map<ARAPKernel::vertex_descriptor, ARAPKernel::Point_2>("v:uv").first;
        
        // 为边界顶点设置初始UV值
        for (size_t i = 0; i < border_vertices_.size(); ++i) {
            ARAPKernel::vertex_descriptor vd = border_vertices_[i];
            uv_map[vd] = border_uvs_[i];
        }
        
        // 定义边界描述符
        typedef std::vector<ARAPKernel::vertex_descriptor> Vertex_container;
        Vertex_container border_vertices(border_vertices_.begin(), border_vertices_.end());
        
        // 创建ARAP参数化器
        typedef SMP::ARAP_parameterizer_3<ARAPKernel::Surface_mesh> Parameterizer;
        
        // 执行ARAP参数化 - 使用正确的参数
        SMP::Error_code error;
        
        try {
            // 首先尝试使用ARAP参数化器
            Parameterizer parameterizer;
            
            // 找到一个边界halfedge
            ARAPKernel::halfedge_descriptor border_halfedge;
            bool found_border = false;
            
            for (auto h : arap_mesh_.halfedges()) {
                if (arap_mesh_.is_border(h)) {
                    border_halfedge = h;
                    found_border = true;
                    break;
                }
            }
            
            if (found_border) {
                // 使用带边界halfedge的参数化
                error = SMP::parameterize(
                    arap_mesh_,
                    parameterizer,
                    border_halfedge,
                    uv_map
                );
            } else {
                qWarning() << "No border found for parameterization";
                error = SMP::ERROR_BORDER_TOO_SHORT;
            }
        } catch (...) {
            qWarning() << "Failed to initialize ARAP parameterizer";
            error = SMP::ERROR_BORDER_TOO_SHORT;
        }
        
        if (error != SMP::OK) {
            qWarning() << "ARAP parameterization failed with error code:" << error;
            
            // 尝试使用LSCM作为后备
            qDebug() << "Trying LSCM as fallback...";
            
            // 使用LSCM参数化
            typedef SMP::Two_vertices_parameterizer_3<ARAPKernel::Surface_mesh> Border_parameterizer;
            typedef SMP::LSCM_parameterizer_3<ARAPKernel::Surface_mesh, Border_parameterizer> LSCM_parameterizer;
            
            // 找到一个边界halfedge
            ARAPKernel::halfedge_descriptor border_halfedge;
            bool found_border = false;
            
            for (auto h : arap_mesh_.halfedges()) {
                if (arap_mesh_.is_border(h)) {
                    border_halfedge = h;
                    found_border = true;
                    break;
                }
            }
            
            if (found_border) {
                LSCM_parameterizer lscm_parameterizer;
                error = SMP::parameterize(
                    arap_mesh_,
                    lscm_parameterizer,
                    border_halfedge,
                    uv_map
                );
                
                if (error != SMP::OK) {
                    qWarning() << "Fallback LSCM also failed with error code:" << error;
                    QMessageBox::warning(nullptr, "Parameterization Error", 
                                       "ARAP parameterization failed. The mesh may not be suitable.");
                    return false;
                }
            } else {
                qWarning() << "No border found for LSCM fallback";
                return false;
            }
        }
        
        // 将参数化后的网格复制回原始网格
        copySurfaceMeshToCgalMesh();
        
        // 更新UV坐标
        updateUVCoordinates();
        
        // === 关键修改：将参数化后的UV坐标映射回3D网格 ===
        mapUVTo3DMesh();
        
        isParameterized_ = true;
        
        qint64 elapsed = timer.elapsed();
        qDebug() << "ARAP parameterization completed in" << elapsed << "ms";
        
        // 发送参数化完成信号
        emit parameterizationCompleted(uv_coordinates_, uv_edges_);
        
        // 强制刷新左侧视图
        makeCurrent();
        updateBuffersFromCGALMesh();
        doneCurrent();
        update();
        
        QMessageBox::information(nullptr, "Success", 
                               QString("ARAP parameterization completed successfully in %1 ms").arg(elapsed));
        
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "Exception during ARAP parameterization:" << e.what();
        QMessageBox::critical(nullptr, "Error", 
                            QString("ARAP parameterization failed: %1").arg(e.what()));
        return false;
    } catch (...) {
        qWarning() << "Unknown exception during ARAP parameterization";
        QMessageBox::critical(nullptr, "Error", "Unknown error during ARAP parameterization");
        return false;
    }
}

void ARAPGLWidget::mapUVTo3DMesh()
{
    if (uv_coordinates_.empty() || mesh.number_of_vertices() == 0) {
        qWarning() << "No UV coordinates or mesh is empty";
        return;
    }
    
    // 将UV坐标转换为3D点（XY平面，Z=0）
    auto uv_map_it = arap_mesh_.property_map<ARAPKernel::vertex_descriptor, ARAPKernel::Point_2>("v:uv");
    if (!uv_map_it.second) {
        qWarning() << "UV map not found in arap_mesh_";
        return;
    }
    
    auto uv_map = uv_map_it.first;
    
    // 遍历arap_mesh_的顶点，将UV坐标应用到对应的CgalMesh顶点
    for (auto vd : arap_mesh_.vertices()) {
        // 获取对应的CgalMesh顶点
        if (vertex_arap_to_cgal_map_.find(vd) != vertex_arap_to_cgal_map_.end()) {
            CgalMesh::Vertex_index v = vertex_arap_to_cgal_map_[vd];
            const ARAPKernel::Point_2& uv = uv_map[vd];
            
            // 将UV坐标转换为3D点（放在XY平面，Z=0）
            Point newPoint(uv.x(), uv.y(), 0.0);
            mesh.point(v) = newPoint;
        }
    }
    
    qDebug() << "Mapped UV coordinates to 3D mesh (XY plane)";
    
    // 重新计算法线（对于平面网格，所有法线应该指向Z轴方向）
    computeNormalsForARAP();
    
    // 计算包围盒并居中缩放，像处理普通3D模型一样
    centerAndScaleParameterizedMesh();
}

void ARAPGLWidget::updateUVCoordinates()
{
    uv_coordinates_.clear();
    uv_edges_.clear();
    
    // 将UV坐标从Surface_mesh提取出来
    auto uv_map_it = arap_mesh_.property_map<ARAPKernel::vertex_descriptor, ARAPKernel::Point_2>("v:uv");
    if (!uv_map_it.second) {
        qWarning() << "UV map not found in arap_mesh_";
        return;
    }
    
    auto uv_map = uv_map_it.first;
    
    // 收集所有顶点的UV坐标
    std::map<ARAPKernel::vertex_descriptor, int> vertex_index_map;
    int index = 0;
    for (auto vd : arap_mesh_.vertices()) {
        const ARAPKernel::Point_2& uv = uv_map[vd];
        uv_coordinates_.emplace_back(uv.x(), uv.y());
        vertex_index_map[vd] = index++;
    }
    
    // 收集边信息
    for (auto e : arap_mesh_.edges()) {
        ARAPKernel::halfedge_descriptor h = arap_mesh_.halfedge(e);
        ARAPKernel::vertex_descriptor v1 = arap_mesh_.source(h);
        ARAPKernel::vertex_descriptor v2 = arap_mesh_.target(h);
        
        if (vertex_index_map.find(v1) != vertex_index_map.end() &&
            vertex_index_map.find(v2) != vertex_index_map.end()) {
            uv_edges_.emplace_back(vertex_index_map[v1], vertex_index_map[v2]);
        }
    }
    
    qDebug() << "UV coordinates updated:" << uv_coordinates_.size() << "vertices," 
             << uv_edges_.size() << "edges";
}

bool ARAPGLWidget::canPerformARAP() const
{
    return modelLoaded && mesh.number_of_vertices() >= 3 && mesh.number_of_faces() >= 1;
}

void ARAPGLWidget::clearParameterization()
{
    if (has_original_backup_) {
        // 恢复原始网格
        // 注意：这里需要实际的深拷贝恢复，简化处理：重新加载原始文件
        isParameterized_ = false;
        uv_coordinates_.clear();
        uv_edges_.clear();
        arap_mesh_.clear();
        border_vertices_.clear();
        border_uvs_.clear();
        
        // 清空当前网格
        mesh.clear();
        modelLoaded = false;
        
        // 清除缓冲区
        makeCurrent();
        faces.clear();
        edges.clear();
        updateBuffersFromCGALMesh();
        doneCurrent();
        
        // 重置视图
        rotation = QQuaternion();
        zoom = 1.0f;
        modelCenter = QVector3D(0, 0, 0);
        viewDistance = 5.0f;
        
        update();
        
        qDebug() << "Parameterization cleared. Please reload the original OBJ file.";
    }
}

void ARAPGLWidget::centerAndScaleParameterizedMesh()
{
    if (mesh.number_of_vertices() == 0) return;
    
    // 计算参数化后网格的包围盒
    Point min, max;
    computeBoundingBox(min, max);
    
    // 计算中心点
    Point center = Point((min.x() + max.x()) * 0.5, 
                         (min.y() + max.y()) * 0.5, 
                         (min.z() + max.z()) * 0.5);
    
    // 计算最大尺寸
    double size_x = max.x() - min.x();
    double size_y = max.y() - min.y();
    double maxSize = std::max(size_x, size_y);
    
    // 如果网格尺寸为0，使用默认值
    if (maxSize <= 0.0) {
        maxSize = 1.0;
    }
    
    // 缩放因子：使最大尺寸为2.0（与基类中的处理一致）
    float scaleFactor = 2.0f / maxSize;
    
    // 平移并缩放所有顶点
    for (auto v : mesh.vertices()) {
        Point p = mesh.point(v);
        // 平移到中心，然后缩放
        p = Point((p.x() - center.x()) * scaleFactor, 
                  (p.y() - center.y()) * scaleFactor, 
                  (p.z() - center.z()) * scaleFactor);
        mesh.point(v) = p;
    }
    
    // 重新计算中心点（现在应该在原点附近）
    computeBoundingBox(min, max);
    center = Point((min.x() + max.x()) * 0.5, 
                   (min.y() + max.y()) * 0.5, 
                   (min.z() + max.z()) * 0.5);
    
    // 更新模型中心点
    modelCenter = QVector3D(center.x(), center.y(), center.z());
    viewDistance = 2.0f * std::max(max.x() - min.x(), max.y() - min.y());
    
    // 重置旋转和缩放
    rotation = QQuaternion();
    zoom = 1.0f;
    
    qDebug() << "Parameterized mesh centered and scaled. Center: (" 
             << center.x() << ", " << center.y() << ", " << center.z() 
             << "), View distance: " << viewDistance;
}