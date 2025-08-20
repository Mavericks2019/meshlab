#include "modelglwidget.h"
#include <cmath>
#include <algorithm>

#define EPSILON 1E-4F

ModelGLWidget::ModelGLWidget(QWidget *parent) : BaseGLWidget(parent)
{
}

void ModelGLWidget::setRenderMode(RenderMode mode) {
    currentRenderMode = mode;
    if (modelLoaded) {
        calculateCurvatures();
        makeCurrent();
        initializeShaders();
        doneCurrent();
    }
    update();
}

void ModelGLWidget::initializeShaders() {
    BaseGLWidget::initializeShaders();
    
    curvatureProgram.removeAllShaders();
    curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/curvature.vert");
    curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/curvature.frag");
    curvatureProgram.link();

    if (modelLoaded) {
        updateBuffersFromOpenMesh();
    }
}

// modelglwidget.cpp
void ModelGLWidget::updateBuffersFromOpenMesh() {
    // 首先调用基类的实现
    BaseGLWidget::updateBuffersFromOpenMesh();
    
    if (openMesh.n_vertices() == 0) return;
    
    vao.bind();
    vbo.bind();
    
    // 计算缓冲区偏移量（与基类保持一致）
    int vertexSize = openMesh.n_vertices() * 3 * sizeof(float);
    int normalSize = openMesh.n_vertices() * 3 * sizeof(float);
    
    // 设置 curvatureProgram 的属性
    curvatureProgram.bind();
    int posLoc = curvatureProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        curvatureProgram.enableAttributeArray(posLoc);
        curvatureProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    int normalLoc = curvatureProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        curvatureProgram.enableAttributeArray(normalLoc);
        curvatureProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    int curvatureLoc = curvatureProgram.attributeLocation("aCurvature");
    if (curvatureLoc != -1) {
        curvatureProgram.enableAttributeArray(curvatureLoc);
        curvatureProgram.setAttributeBuffer(curvatureLoc, GL_FLOAT, vertexSize + normalSize, 1, sizeof(float));
    }

    vao.release();
}

void ModelGLWidget::loadOBJ(const QString &path) {
    BaseGLWidget::loadOBJ(path); // 调用基类加载
    if (modelLoaded) {
        calculateCurvatures(); // 计算曲率
        makeCurrent();
        initializeShaders(); // 重新初始化着色器
        doneCurrent();
    }
}

void ModelGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || openMesh.n_vertices() == 0) {
        return;
    }

    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    // 定义三个光源的位置和颜色
    QVector3D lightPositions[3] = {
        QVector3D(10.0f, 10.0f, -10.0f),
        QVector3D(-10.0f, 10.0f, -10.0f),
        QVector3D(0.0f, 0.0f, 10.0f)
    };
    QVector3D lightColors[3] = {
        QVector3D(1.0f, 1.0f, 1.0f),
        QVector3D(1.0f, 1.0f, 1.0f),
        QVector3D(1.0f, 1.0f, 1.0f)
    };

    if (hideFaces) {
        drawWireframe(model, view, projection);
    } else {
        switch (currentRenderMode) {
        case GaussianCurvature:
        case MeanCurvature:
        case MaxCurvature:
            drawCurvature(model, view, projection, normalMatrix);
            break;
        case FlatShading:
            // Flat Shading渲染
            flatProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            flatProgram.setUniformValue("model", model);
            flatProgram.setUniformValue("view", view);
            flatProgram.setUniformValue("projection", projection);
            flatProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置三个光源的位置和颜色
            for (int i = 0; i < 3; i++) {
                flatProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                flatProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            flatProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
            flatProgram.setUniformValue("objectColor", surfaceColor);
            flatProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            flatProgram.release();
            break;
        default:
            // 调用基类的BlinnPhong渲染
            blinnPhongProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            blinnPhongProgram.setUniformValue("model", model);
            blinnPhongProgram.setUniformValue("view", view);
            blinnPhongProgram.setUniformValue("projection", projection);
            blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置三个光源的位置和颜色
            for (int i = 0; i < 3; i++) {
                blinnPhongProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                blinnPhongProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            blinnPhongProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
            blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            blinnPhongProgram.release();
            break;
        }

        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
    }
    
    if (showAxis) {
        drawXYZAxis(view, projection);
    }
    
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

void ModelGLWidget::drawCurvature(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    curvatureProgram.bind();
    vao.bind();
    faceEbo.bind();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    curvatureProgram.setUniformValue("model", model);
    curvatureProgram.setUniformValue("view", view);
    curvatureProgram.setUniformValue("projection", projection);
    curvatureProgram.setUniformValue("normalMatrix", normalMatrix);
    curvatureProgram.setUniformValue("curvatureType", static_cast<int>(currentRenderMode));
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    curvatureProgram.release();
}

void ModelGLWidget::calculateCurvatures() {
    if (openMesh.n_vertices() == 0) return;
    
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            openMesh.data(vh).curvature = 0.0f;
            continue;
        }
        
        float angleDefect = 2 * M_PI;
        float area = 0.0f;
        
        for (auto vf_it = openMesh.vf_begin(vh); vf_it != openMesh.vf_end(vh); ++vf_it) {
            auto heh = openMesh.halfedge_handle(*vf_it);
            while (openMesh.to_vertex_handle(heh) != vh) {
                heh = openMesh.next_halfedge_handle(heh);
            }
            
            auto v1 = openMesh.point(openMesh.from_vertex_handle(heh));
            auto v2 = openMesh.point(openMesh.to_vertex_handle(heh));
            auto heh_next = openMesh.next_halfedge_handle(heh);
            auto v3 = openMesh.point(openMesh.to_vertex_handle(heh_next));
            
            auto vec1 = (v1 - v2).normalize();
            auto vec2 = (v3 - v2).normalize();
            float angle = acos(std::max(-1.0f, std::min(1.0f, dot(vec1, vec2))));
            angleDefect -= angle;
            
            auto cross = (v1 - v2) % (v3 - v2);
            area += cross.length() / 6.0f;
        }
        
        float gaussianCurvature = 0.0f;
        if (area > EPSILON) {
            gaussianCurvature = angleDefect / area;
        }
        
        float A_mixed = 0.f;
        for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
            auto adjV = *vv_it;
            auto heh = openMesh.find_halfedge(vh, adjV);
            if (!heh.is_valid()) continue;
            
            Mesh::VertexHandle np;
            if (!openMesh.is_boundary(heh)) {
                auto next_heh = openMesh.next_halfedge_handle(heh);
                np = openMesh.to_vertex_handle(next_heh);
            } else {
                auto opp_heh = openMesh.opposite_halfedge_handle(heh);
                if (!openMesh.is_boundary(opp_heh)) {
                    auto next_opp_heh = openMesh.next_halfedge_handle(opp_heh);
                    np = openMesh.to_vertex_handle(next_opp_heh);
                } else {
                    continue;
                }
            }

            auto p_v = openMesh.point(vh);
            auto p_adjV = openMesh.point(adjV);
            auto p_np = openMesh.point(np);
            
            auto vec_adjV = p_adjV - p_v;
            auto vec_np = p_np - p_v;
            
            bool nonObtuse = 
                (vec_adjV | vec_np) >= 0.0f &&
                (p_v - p_adjV | p_np - p_adjV) >= 0.0f &&
                (p_v - p_np | p_adjV - p_np) >= 0.0f;
            
            if (nonObtuse) {
                float area = ((p_adjV - p_v) % (p_np - p_v)).length() / 2.0f;
                if (area > EPSILON) {
                    float cotB = dot(vec_np, vec_adjV) / cross(vec_np, vec_adjV).length();
                    float cotA = dot(vec_adjV, vec_np) / cross(vec_adjV, vec_np).length();
                    
                    float dist2_adjV = vec_adjV.sqrnorm();
                    float dist2_np = vec_np.sqrnorm();
                    
                    A_mixed += (dist2_adjV * cotB + dist2_np * cotA) / 8.0f;
                }
            } else {
                float area = ((p_adjV - p_v) % (p_np - p_v)).length() / 2.0f;
                if (area > EPSILON) {
                    if ((vec_adjV | vec_np) < 0.0f) {
                        A_mixed += area / 2.0f;
                    } else {
                        A_mixed += area / 4.0f;
                    }
                }
            }
        }
        
        float meanCurvature = 0.0f;
        if (A_mixed > EPSILON) {
            meanCurvature = 0.5f * sqrt(A_mixed);
        }
        switch (currentRenderMode) {
        case GaussianCurvature:
            openMesh.data(vh).curvature = gaussianCurvature;
            break;
        case MeanCurvature:
            openMesh.data(vh).curvature = meanCurvature;
            break;
        case MaxCurvature:
            openMesh.data(vh).curvature = gaussianCurvature + meanCurvature;
            break;
        default:
            openMesh.data(vh).curvature = 0.0f;
            break;
        }

    }
    
    // Normalize curvatures
    float minCurvature = FLT_MAX;
    float maxCurvature = -FLT_MAX;
    
    for (auto vh : openMesh.vertices()) {
        if (!openMesh.is_boundary(vh)) {
            minCurvature = std::min(minCurvature, openMesh.data(vh).curvature);
            maxCurvature = std::max(maxCurvature, openMesh.data(vh).curvature);
        }
    }
    
    float range = maxCurvature - minCurvature;
    if (range > 0) {
        for (auto vh : openMesh.vertices()) {
            if (!openMesh.is_boundary(vh)) {
                openMesh.data(vh).curvature = 
                    (openMesh.data(vh).curvature - minCurvature) / range;
            }
        }
    }
}