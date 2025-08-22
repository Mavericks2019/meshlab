#ifndef SHORTESTPATHGLWIDGET_H
#define SHORTESTPATHGLWIDGET_H

#include "baseglwidget.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <vector>

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
    void calculateShortestPath(); // 占位函数，后续实现
    void savePickingImage(const QString& filename); // 新增方法

protected:
    void initializePickingShaders();
    void renderForPicking();
    int pickVertexAtPosition(int x, int y);

private:
    QOpenGLShaderProgram pickingProgram;
    QOpenGLShaderProgram facePickingProgram; // 新增：面元拾取着色器
    QOpenGLFramebufferObject *pickingFBO;
    
    std::vector<unsigned int> selectedVertices;
    std::vector<unsigned int> pathVertices;
    
    QVector3D highlightColor = QVector3D(1.0f, 0.0f, 0.0f); // 高亮颜色（红色）
};

#endif // SHORTESTPATHGLWIDGET_H