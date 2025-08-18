#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <QQuaternion>  // 添加四元数支持

struct MyTraits : public OpenMesh::DefaultTraits {
    VertexAttributes(OpenMesh::Attributes::Normal | 
                     OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Normal | 
                   OpenMesh::Attributes::Status);
    VertexTraits {
        float curvature;
    };
};
typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> Mesh;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum RenderMode { 
        BlinnPhong,
        GaussianCurvature,
        MeanCurvature,
        MaxCurvature
    };
    
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

public:
    void setRenderMode(RenderMode mode);
    void setBackgroundColor(const QColor& color);
    void setWireframeColor(const QVector4D& color);
    void setSurfaceColor(const QVector3D& color);
    void setSpecularEnabled(bool enabled);
    void setShowWireframeOverlay(bool show);
    void setHideFaces(bool hide);
    void resetView();
    void centerView();
    void loadOBJ(const QString &path);
    void clearMeshData();
    bool loadOBJToOpenMesh(const QString &path);
    void computeBoundingBox(Mesh::Point& min, Mesh::Point& max);
    void centerAndScaleMesh(const Mesh::Point& center, float maxSize);
    void prepareFaceIndices();
    void prepareEdgeIndices();
    void saveOriginalMesh();
    void calculateCurvatures();
    void setViewScale(float scale); // 新增：设置视角缩放因子

    QVector3D surfaceColor = QVector3D(1.0f, 1.0f, 0.0f);
    bool specularEnabled = true;
    QVector4D wireframeColor;
    QColor bgColor;
    RenderMode currentRenderMode;
    
    Mesh openMesh;
    Mesh originalMesh;
    bool hasOriginalMesh = false;
    
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
    
    QQuaternion rotation;   // 使用四元数替代欧拉角
    float zoom;
    
    bool showWireframeOverlay;
    bool hideFaces;
    bool modelLoaded;

    QVector3D modelCenter;   // 模型中心位置
    float viewDistance;      // 基础视图距离（未缩放）
    float viewScale = 1.5f;  // 新增：视角缩放因子
    QVector3D eyePosition;   // 相机位置

private:
    // 初始视图状态
    QQuaternion initialRotation;  // 初始旋转状态
    float rotationSensitivity = 2.0f; // 默认值，可根据需要调整
    float initialZoom;
    QVector3D initialModelCenter;
    float initialViewDistance;
    float initialViewScale = 2.0f; // 新增：初始视角缩放因子

    // XYZ坐标轴相关成员
    void drawXYZAxis(const QMatrix4x4& view, const QMatrix4x4& projection);
    QOpenGLShaderProgram axisProgram;
    QOpenGLBuffer axisVbo;
    QOpenGLBuffer axisEbo;
    bool showAxis; // 控制是否显示坐标轴

    // 轨迹球相关
    QVector3D projectToTrackball(const QPoint& screenPos);

protected:
    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram blinnPhongProgram;
    QOpenGLShaderProgram curvatureProgram;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QOpenGLBuffer faceEbo;
    
    bool isDragging;
    QPoint lastMousePos;

    void updateBuffersFromOpenMesh();
    void initializeShaders();
    void drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawCurvature(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix);
    void drawBlinnPhong(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix);
    void drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
};

#endif // GLWIDGET_H