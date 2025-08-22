// shortestpathglwidget.h
#ifndef SHORTESTPATHGLWIDGET_H
#define SHORTESTPATHGLWIDGET_H

#include "baseglwidget.h"
#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <vector>
#include <queue>
#include <functional>
#include <limits>

class ShortestPathGLWidget : public BaseGLWidget
{
    Q_OBJECT

public:
    explicit ShortestPathGLWidget(QWidget *parent = nullptr);
    ~ShortestPathGLWidget();

    void initializeGL() override;
    void paintGL() override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void clearSelectedPoints();
    void calculateShortestPath(); // 计算最短路径
    void savePickingImage(const QString& filename);
    
    // 新增：计算所有选中点之间的最短路径
    void calculateAllShortestPaths();

protected:
    void initializePickingShaders();
    void renderForPicking();
    int pickVertexAtPosition(int x, int y);

private:
    QOpenGLShaderProgram pickingProgram;
    QOpenGLShaderProgram facePickingProgram;
    QOpenGLFramebufferObject *pickingFBO;
    std::vector<unsigned int> pathEdgeIndices; // 存储路径边的顶点索引
    QOpenGLBuffer pathEdgeEbo; // 专门用于路径边的EBO
    
    std::vector<unsigned int> selectedVertices;
    std::vector<unsigned int> pathVertices;
    std::vector<unsigned int> pathEdges; // 新增：存储路径边的索引
    
    QVector3D highlightColor = QVector3D(1.0f, 0.0f, 0.0f); // 高亮颜色（红色）
    
    // 新增：手动实现的Dijkstra算法计算最短路径
    std::vector<unsigned int> dijkstraShortestPath(unsigned int start, unsigned int end);
    
    // 新增：绘制路径边
    void renderPathEdges();
};

#endif // SHORTESTPATHGLWIDGET_H