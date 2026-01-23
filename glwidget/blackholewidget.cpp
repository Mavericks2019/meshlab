// blackholewidget.cpp
#include "blackholewidget.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <QFile>
#include <QDebug>
#include <QOpenGLFunctions>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace glm;
using namespace std;

// Constants
double c = 299792458.0;
double G = 6.67430e-11;

// 独立于类的RK4函数实现
void geodesicRHS(const BlackHoleWidget::Ray& ray, double rhs[4], double rs) {
    double r    = ray.r;
    double dr   = ray.dr;
    double dphi = ray.dphi;
    double E    = ray.E;

    double f = 1.0 - rs/r;

    rhs[0] = dr;
    rhs[1] = dphi;

    double dt_dλ = E / f;
    rhs[2] = 
        - (rs/(2*r*r)) * f * (dt_dλ*dt_dλ)
        + (rs/(2*r*r*f)) * (dr*dr)
        + (r - rs) * (dphi*dphi);

    rhs[3] = -2.0 * dr * dphi / r;
}

void addState(const double a[4], const double b[4], double factor, double out[4]) {
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i] * factor;
}

void rk4Step(BlackHoleWidget::Ray& ray, double dλ, double rs) {
    double y0[4] = { ray.r, ray.phi, ray.dr, ray.dphi };
    double k1[4], k2[4], k3[4], k4[4], temp[4];

    geodesicRHS(ray, k1, rs);
    addState(y0, k1, dλ/2.0, temp);
    BlackHoleWidget::Ray r2 = ray; 
    r2.r = temp[0]; r2.phi = temp[1]; r2.dr = temp[2]; r2.dphi = temp[3];
    geodesicRHS(r2, k2, rs);

    addState(y0, k2, dλ/2.0, temp);
    BlackHoleWidget::Ray r3 = ray; 
    r3.r = temp[0]; r3.phi = temp[1]; r3.dr = temp[2]; r3.dphi = temp[3];
    geodesicRHS(r3, k3, rs);

    addState(y0, k3, dλ, temp);
    BlackHoleWidget::Ray r4 = ray; 
    r4.r = temp[0]; r4.phi = temp[1]; r4.dr = temp[2]; r4.dphi = temp[3];
    geodesicRHS(r4, k4, rs);

    ray.r    += (dλ/6.0)*(k1[0] + 2*k2[0] + 2*k3[0] + k4[0]);
    ray.phi  += (dλ/6.0)*(k1[1] + 2*k2[1] + 2*k3[1] + k4[1]);
    ray.dr   += (dλ/6.0)*(k1[2] + 2*k2[2] + 2*k3[2] + k4[2]);
    ray.dphi += (dλ/6.0)*(k1[3] + 2*k2[3] + 2*k3[3] + k4[3]);
}

// BlackHole implementation
BlackHoleWidget::BlackHole::BlackHole(vec3 pos, double m) : position(pos), mass(m) {
    r_s = 2.0 * G * mass / (c*c);
}

void BlackHoleWidget::BlackHole::draw() {
    // 启用多重采样抗锯齿
    glEnable(GL_MULTISAMPLE);
    
    // 启用多边形平滑
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    
    // 绘制黑洞
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    for(int i = 0; i <= 64; i++) { // 增加顶点数以提高平滑度
        float angle = 2.0f * M_PI * i / 64;
        float x = r_s * cos(angle);
        float y = r_s * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // 禁用多边形平滑（避免影响其他绘制）
    glDisable(GL_POLYGON_SMOOTH);
}

// Ray implementation
BlackHoleWidget::Ray::Ray(vec2 pos, vec2 dir, double blackHoleMass) : 
    x(pos.x), y(pos.y), 
    r(sqrt(pos.x * pos.x + pos.y * pos.y)), 
    phi(atan2(pos.y, pos.x)), 
    dr(dir.x), dphi(dir.y) {
    
    this->r = sqrt(x*x + y*y);
    this->phi = atan2(y, x);
    
    dr = dir.x * cos(phi) + dir.y * sin(phi);
    dphi = ( -dir.x * sin(phi) + dir.y * cos(phi) ) / r;
    
    L = r*r * dphi;
    double rs = 2.0 * G * blackHoleMass / (c*c);
    double f = 1.0 - rs/r;  
    double dt_dλ = sqrt( (dr*dr)/(f*f) + (r*r*dphi*dphi)/f );
    E = f * dt_dλ;
    
    trail.push_back({x, y});
}

void BlackHoleWidget::Ray::draw(const std::vector<Ray>& rays, const QVector3D& rayColor, const QVector3D& trailColor) {
    // 启用多重采样抗锯齿
    glEnable(GL_MULTISAMPLE);
    
    // 启用线平滑
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // 启用点平滑
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    // 绘制光线点
    glPointSize(3.0f); // 稍微增大点大小以获得更好的抗锯齿效果
    glColor3f(rayColor.x(), rayColor.y(), rayColor.z());
    glBegin(GL_POINTS);
    for (const auto& ray : rays) {
        glVertex2f(ray.x, ray.y);
    }
    glEnd();

    // 检查混合是否已启用
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    if (!blendEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // 绘制每条光线的轨迹
    glLineWidth(1.5f); // 稍微增加线宽以获得更好的抗锯齿效果
    for (const auto& ray : rays) {
        size_t N = ray.trail.size();
        if (N < 2) continue;

        glBegin(GL_LINE_STRIP);
        for (size_t i = 0; i < N; ++i) {
            // 渐变色：从透明到轨迹颜色
            float alpha = float(i) / float(N - 1);
            glColor4f(trailColor.x(), trailColor.y(), trailColor.z(), std::max(alpha, 0.05f));
            glVertex2f(ray.trail[i].x, ray.trail[i].y);
        }
        glEnd();
    }

    // 禁用平滑和混合（如果需要）
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
}

void BlackHoleWidget::Ray::step(double dλ, double rs) {
    if(r <= rs) return; // 如果进入事件视界则停止
    
    // 调用RK4步进算法
    rk4Step(*this, dλ, rs);
    
    // 更新笛卡尔坐标
    x = r * cos(phi);
    y = r * sin(phi);
    
    // 记录轨迹
    trail.push_back({ float(x), float(y) });
}

// BlackHoleWidget implementation
BlackHoleWidget::BlackHoleWidget(QWidget* parent) : 
    QOpenGLWidget(parent), 
    SagA(glm::vec3(0.0f, 0.0f, 0.0f), 8.54e36) {
    
    setMouseTracking(true);
    
    // 设置更兼容的格式，启用多重采样抗锯齿
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 1); // 使用更低的OpenGL版本以提高兼容性
    format.setProfile(QSurfaceFormat::CompatibilityProfile); // 兼容性模式
    format.setOption(QSurfaceFormat::DeprecatedFunctions, true); // 允许使用已弃用的函数
    
    // 启用多重采样抗锯齿
    format.setSamples(4); // 4倍多重采样
    
    setFormat(format);
    
    // 初始化当前宽度和高度
    currentWidth = baseWidth;
    currentHeight = baseHeight;
    aspectRatio = (float)WIDTH / (float)HEIGHT;
    
    // 保存初始状态
    initialBaseWidth = baseWidth;
    initialBaseHeight = baseHeight;
    initialOffsetX = offsetX;
    initialOffsetY = offsetY;
    
    // 初始化模拟
    initializeSimulation();
    
    // 设置模拟定时器
    simulationTimer = new QTimer(this);
    connect(simulationTimer, &QTimer::timeout, this, QOverload<>::of(&BlackHoleWidget::update));
    simulationTimer->start(16); // ~60 FPS
}

BlackHoleWidget::~BlackHoleWidget() {
    if (simulationTimer) {
        simulationTimer->stop();
        delete simulationTimer;
    }
}

void BlackHoleWidget::initializeSimulation() {
    // 添加测试光线
    clearRays();
    addRay(vec2(-1e11, 3.27606302719999999e10), vec2(c, 0.0f));
    addRay(vec2(-1e11, 1e11), vec2(c, 0.0f));
    addRay(vec2(-1e11, 5e10), vec2(c, 0.0f));
    addRay(vec2(-1e11, 0.0f), vec2(c, 0.0f));
    addRay(vec2(-1e11, -5e10), vec2(c, 0.0f));
    addRay(vec2(-1e11, -1e11), vec2(c, 0.0f));
}

void BlackHoleWidget::addRay(const glm::vec2& position, const glm::vec2& direction) {
    rays.push_back(Ray(position, direction, SagA.mass));
}

void BlackHoleWidget::clearRays() {
    rays.clear();
}

void BlackHoleWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void BlackHoleWidget::setRayColor(const QColor& color) {
    rayColor = QVector3D(color.redF(), color.greenF(), color.blueF());
    update();
}

void BlackHoleWidget::setTrailColor(const QColor& color) {
    trailColor = QVector3D(color.redF(), color.greenF(), color.blueF());
    update();
}

void BlackHoleWidget::setSimulationSpeed(double speed) {
    simulationSpeed = speed;
    if (simulationTimer) {
        simulationTimer->setInterval(16 / speed);
    }
}

void BlackHoleWidget::setBlackHoleMass(double mass) {
    SagA = BlackHole(glm::vec3(0.0f, 0.0f, 0.0f), mass);
    // 重新初始化光线
    initializeSimulation();
    update();
}

void BlackHoleWidget::resetView() {
    baseWidth = initialBaseWidth;
    baseHeight = initialBaseHeight;
    offsetX = initialOffsetX;
    offsetY = initialOffsetY;
    updateProjectionMatrix();
    update();
}

void BlackHoleWidget::initializeGL() {
    // 设置基本的OpenGL状态
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glDisable(GL_DEPTH_TEST); // 2D不需要深度测试
    
    // 启用多重采样（如果可用）
    glEnable(GL_MULTISAMPLE);
    
    qDebug() << "OpenGL initialized successfully with anti-aliasing";
}

void BlackHoleWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    WIDTH = w;
    HEIGHT = h;
    
    // 更新宽高比
    aspectRatio = (float)WIDTH / (float)HEIGHT;
    
    // 更新投影矩阵
    updateProjectionMatrix();
}

void BlackHoleWidget::updateProjectionMatrix() {
    // 根据宽高比调整投影矩阵，保持圆形显示
    if (aspectRatio >= 1.0f) {
        // 宽度大于高度，保持高度不变，调整宽度
        currentWidth = baseHeight * aspectRatio;
        currentHeight = baseHeight;
    } else {
        // 高度大于宽度，保持宽度不变，调整高度
        currentWidth = baseWidth;
        currentHeight = baseWidth / aspectRatio;
    }
}

void BlackHoleWidget::paintGL() {
    // 清屏
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 设置投影矩阵
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // 使用调整后的宽度和高度，保持正确的宽高比
    double left   = -currentWidth + offsetX;
    double right  =  currentWidth + offsetX;
    double bottom = -currentHeight + offsetY;
    double top    =  currentHeight + offsetY;
    glOrtho(left, right, bottom, top, -1.0, 1.0);
    
    // 设置模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 绘制黑洞
    SagA.draw();
    
    // 更新并绘制所有光线
    for (auto& ray : rays) {
        ray.step(simulationSpeed, SagA.r_s);
        ray.draw(rays, rayColor, trailColor);
    }
}

void BlackHoleWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        middleMousePressed = true;
        lastMouseX = event->x();
        lastMouseY = event->y();
        event->accept();
    }
}

void BlackHoleWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        middleMousePressed = false;
        event->accept();
    }
}

void BlackHoleWidget::mouseMoveEvent(QMouseEvent* event) {
    if (middleMousePressed) {
        double deltaX = event->x() - lastMouseX;
        double deltaY = event->y() - lastMouseY;
        
        // 使用调整后的宽度和高度来计算世界坐标增量
        double worldDeltaX = deltaX * (2.0 * currentWidth / WIDTH);
        double worldDeltaY = -deltaY * (2.0 * currentHeight / HEIGHT);
        
        offsetX -= worldDeltaX;
        offsetY -= worldDeltaY;
        
        lastMouseX = event->x();
        lastMouseY = event->y();
        event->accept();
    }
}

void BlackHoleWidget::wheelEvent(QWheelEvent* event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        double zoomFactor = 1.1;
        if (numDegrees.y() > 0) {
            // 缩放时保持宽高比
            baseWidth /= zoomFactor;
            baseHeight /= zoomFactor;
        } else {
            baseWidth *= zoomFactor;
            baseHeight *= zoomFactor;
        }
        
        // 更新投影矩阵
        updateProjectionMatrix();
        event->accept();
    }
}