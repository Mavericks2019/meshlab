// blackholewidget.h
#ifndef BLACKHOLEWIDGET_H
#define BLACKHOLEWIDGET_H

// 首先包含Qt OpenGL头文件
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QColor>

// 然后包含GLM
#include <glm/glm.hpp>
#include <vector>

// 前向声明
struct Ray;
void rk4Step(Ray& ray, double dlambda, double rs);

class BlackHoleWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit BlackHoleWidget(QWidget* parent = nullptr);
    ~BlackHoleWidget();

    // 公共方法用于控制
    void setBackgroundColor(const QColor& color);
    void setRayColor(const QColor& color);
    void setTrailColor(const QColor& color);
    void resetView();
    void addRay(const glm::vec2& position, const glm::vec2& direction);
    void clearRays();
    void setSimulationSpeed(double speed);
    void setBlackHoleMass(double mass);

    // 物理结构
    struct BlackHole {
        glm::vec3 position;
        double mass;
        double radius;
        double r_s;

        BlackHole(glm::vec3 pos, double m);
        void draw();
    };

    struct Ray {
        double x, y;
        double r, phi;
        double dr, dphi;
        std::vector<glm::vec2> trail;
        double E, L;

        Ray(glm::vec2 pos, glm::vec2 dir, double blackHoleMass);
        void draw(const std::vector<Ray>& rays, const QVector3D& rayColor, const QVector3D& trailColor);
        void step(double dlambda, double rs);
    };

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // 辅助函数
    void updateProjectionMatrix();
    void initializeSimulation();

    // 成员变量
    int WIDTH = 800;
    int HEIGHT = 600;
    float aspectRatio = 1.0f;
    float baseWidth = 100000000000.0f;
    float baseHeight = 75000000000.0f;
    float currentWidth, currentHeight;

    float offsetX = 0.0f, offsetY = 0.0f;
    bool middleMousePressed = false;
    double lastMouseX = 0.0, lastMouseY = 0;
    
    BlackHole SagA;
    std::vector<Ray> rays;
    
    // 颜色设置
    QColor bgColor = QColor(0, 0, 0);
    QVector3D rayColor = QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D trailColor = QVector3D(1.0f, 1.0f, 1.0f);
    
    // 模拟控制
    double simulationSpeed = 1.0f;
    QTimer* simulationTimer;
    
    // 初始状态
    float initialBaseWidth = 100000000000.0f;
    float initialBaseHeight = 75000000000.0f;
    float initialOffsetX = 0.0f;
    float initialOffsetY = 0.0f;
};

#endif // BLACKHOLEWIDGET_H