// arapglwidget.h
#ifndef ARAPGLWIDGET_H
#define ARAPGLWIDGET_H

#include "cgalglwidget.h"
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh_parameterization/ARAP_parameterizer_3.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/transform.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/boost/graph/selection.h>
#include <CGAL/Timer.h>
#include <CGAL/Origin.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

namespace SMP = CGAL::Surface_mesh_parameterization;
namespace PMP = CGAL::Polygon_mesh_processing;

// 使用与基类相同的Kernel定义
// 注意：这里不能重新定义Kernel，应该使用基类的定义
// 我们将使用基类中定义的typedef
// 这里只定义新类型
namespace ARAPKernel {
    // 定义自己的Kernel类型，避免与基类冲突
    typedef CGAL::Simple_cartesian<double> Kernel;
    typedef Kernel::Point_2 Point_2;
    typedef Kernel::Point_3 Point_3;
    typedef Kernel::Vector_2 Vector_2;
    typedef Kernel::Vector_3 Vector_3;
    typedef CGAL::Surface_mesh<Point_3> Surface_mesh;
    typedef boost::graph_traits<Surface_mesh>::vertex_descriptor vertex_descriptor;
    typedef boost::graph_traits<Surface_mesh>::halfedge_descriptor halfedge_descriptor;
    typedef boost::graph_traits<Surface_mesh>::face_descriptor face_descriptor;
    typedef Surface_mesh::Property_map<vertex_descriptor, Point_2> UV_pmap;
    
    // 添加辅助函数声明
    template<typename Graph>
    typename boost::graph_traits<Graph>::vertex_descriptor 
    vertices_around_face(typename boost::graph_traits<Graph>::halfedge_descriptor h, const Graph& g);
}

class ARAPGLWidget : public CGALGLWidget
{
    Q_OBJECT

public:
    explicit ARAPGLWidget(QWidget *parent = nullptr);
    virtual ~ARAPGLWidget();

    // 执行ARAP参数化
    bool performARAPParameterization();
    
    // 检查是否可以执行ARAP参数化
    bool canPerformARAP() const;
    
    // 获取参数化状态
    bool isParameterized() const { return isParameterized_; }
    
    // 清除参数化结果
    void clearParameterization();

    // 设置参数化选项
    void setUseFixedBorder(bool useFixed) { useFixedBorder_ = useFixed; }
    void setUseIterativeSolver(bool useIterative) { useIterativeSolver_ = useIterative; }
    void setMaxIterations(int iterations) { maxIterations_ = iterations; }
    void setConvergenceThreshold(double threshold) { convergenceThreshold_ = threshold; }

    // 添加computeNormals函数声明（非覆盖版本）
    void computeNormalsForARAP();
    
    // 重写loadOBJ函数以处理备份 - 设为public
    void loadOBJ(const QString &path);
    
    // 获取UV坐标和边信息
    const std::vector<QVector2D>& getUVCoordinates() const { return uv_coordinates_; }
    const std::vector<std::pair<int, int>>& getUVEdges() const { return uv_edges_; }
    
    // 获取参数化后的网格（用于传递给UV视图）
    const ARAPKernel::Surface_mesh& getParameterizedMesh() const { return arap_mesh_; }

signals:
    // 添加信号，通知参数化完成
    void parameterizationCompleted(const std::vector<QVector2D>& uvCoords,
                                   const std::vector<std::pair<int, int>>& uvEdges);

protected:
    // 准备ARAP参数化
    bool prepareForARAP();
    
    // 提取边界
    bool extractBoundary();
    
    // 将参数化结果转换为OpenMesh格式以显示在UV视图中
    void convertToOpenMeshUV();

private:
    // ARAP相关数据
    ARAPKernel::Surface_mesh arap_mesh_;
    std::vector<ARAPKernel::vertex_descriptor> border_vertices_;
    std::vector<ARAPKernel::Point_2> border_uvs_;
    bool isParameterized_;
    
    // ARAP参数化选项
    bool useFixedBorder_;
    bool useIterativeSolver_;
    int maxIterations_;
    double convergenceThreshold_;
    
    // 原始网格备份
    CgalMesh original_mesh_;
    bool has_original_backup_;
    
    // UV映射数据
    std::vector<QVector2D> uv_coordinates_;
    std::vector<std::pair<int, int>> uv_edges_;
    
    // 顶点映射
    std::map<CgalMesh::Vertex_index, ARAPKernel::vertex_descriptor> vertex_cgal_to_arap_map_;
    std::map<ARAPKernel::vertex_descriptor, CgalMesh::Vertex_index> vertex_arap_to_cgal_map_;
    
    // 边界选择相关
    double computeMeshDiagonal();
    void selectBoundaryVertices();
    void createCircularBorderUVs();
    void createSquareBorderUVs();
    
    // 网格检查和修复
    bool checkAndFixMesh();
    bool isMeshValidForARAP() const;
    void triangulateMesh();
    void removeIsolatedVertices();
    
    // 辅助函数
    void copyMeshToSurfaceMesh();
    void copySurfaceMeshToCgalMesh();
    void updateUVCoordinates();
    
    // 备份和恢复原始网格
    void backupOriginalMesh();
    void restoreOriginalMesh();
    
    // 解决函数重载问题：使用不同的函数名
    ARAPKernel::Point_3 convertToARAPPoint(const Point& p) const;  // 从CGAL::Simple_cartesian::Point_3转换
    Point convertToCgalPoint(const ARAPKernel::Point_3& p) const;  // 转换到CGAL::Simple_cartesian::Point_3
    
    // 添加辅助函数
    void computeBoundingBox(Point& min, Point& max);
    
    // 使用正确的半边描述符类型
    typedef CGAL::SM_Halfedge_index CgalHalfedgeDescriptor;
    void mapUVTo3DMesh();
    void centerAndScaleParameterizedMesh();
};

#endif // ARAPGLWIDGET_H