// cgalglwidget.h
#ifndef CGALGLWIDGET_H
#define CGALGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <QQuaternion>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;
typedef CGAL::Surface_mesh<Point> CgalMesh;

class CGALGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CGALGLWidget(QWidget *parent = nullptr);
    virtual ~CGALGLWidget();

    enum RenderMode { 
        BlinnPhong
    };

    void setBackgroundColor(const QColor& color);
    void setWireframeColor(const QVector4D& color);
    void setSurfaceColor(const QVector3D& color);
    void setSpecularEnabled(bool enabled);
    void setShowWireframeOverlay(bool show);
    void setHideFaces(bool hide);
    void setShowAxis(bool show);
    void resetView();
    void centerView();
    void loadOBJ(const QString &path);
    void clearMeshData();
    void setViewScale(float scale);

    QVector3D surfaceColor = QVector3D(1.0f, 1.0f, 0.0f);
    bool specularEnabled = true;
    QVector4D wireframeColor;
    QColor bgColor;
    RenderMode currentRenderMode;
    
    CgalMesh mesh;
    bool hasOriginalMesh = false;
    
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
    
    QQuaternion rotation;
    float zoom;
    
    bool showWireframeOverlay;
    bool hideFaces;
    bool modelLoaded;

    QVector3D modelCenter;
    float viewDistance;
    float viewScale = 1.5f;
    QVector3D eyePosition;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    bool loadOBJToCGALMesh(const QString &path);
    void computeBoundingBox(Point& min, Point& max);
    void centerAndScaleMesh(const Point& center, float maxSize);
    void prepareFaceIndices();
    void prepareEdgeIndices();
    void saveOriginalMesh();
    void updateBuffersFromCGALMesh();
    void initializeShaders();
    void drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawXYZAxis(const QMatrix4x4& view, const QMatrix4x4& projection);
    QVector3D projectToTrackball(const QPoint& screenPos);

    // 初始视图状态
    QQuaternion initialRotation;
    float rotationSensitivity = 2.0f;
    float initialZoom;
    QVector3D initialModelCenter;
    float initialViewDistance;
    float initialViewScale = 2.0f;

    // XYZ坐标轴相关成员
    QOpenGLShaderProgram axisProgram;
    QOpenGLBuffer axisVbo;
    QOpenGLBuffer axisEbo;
    bool showAxis;

    // 轨迹球相关
    bool isDragging;
    QPoint lastMousePos;

    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram blinnPhongProgram;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QOpenGLBuffer faceEbo;
};

#endif // CGALGLWIDGET_H