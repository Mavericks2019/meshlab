// simplesquarewidget.cpp
#include "simplesquarewidget.h"
#include <QDebug>
#include <Eigen/Sparse>
#include <set>
#include <cmath>

SimpleSquareWidget::SimpleSquareWidget(QWidget *parent) : BaseGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    meshVbo(QOpenGLBuffer::VertexBuffer),
    meshEbo(QOpenGLBuffer::IndexBuffer),
    squareSize(1.0f)  // 保持为1.0，在resizeGL中动态调整
{
    setFocusPolicy(Qt::StrongFocus);
    
    // 设置正方形颜色为白色
    squareColor = QColor(255, 255, 255, 255);
    // 设置网格颜色为白色（与正方形颜色一致）
    meshColor = QColor(255, 255, 255, 255);
    
    // 设置抗锯齿格式
    QSurfaceFormat format;
    format.setSamples(4);  // 4倍多重采样
    setFormat(format);
}

SimpleSquareWidget::~SimpleSquareWidget() {
    makeCurrent();
    squareVao.destroy();
    squareVbo.destroy();
    squareEbo.destroy();
    meshVao.destroy();
    meshVbo.destroy();
    meshEbo.destroy();
    doneCurrent();
}

void SimpleSquareWidget::setSquareColor(const QColor& color) {
    squareColor = color;
    meshColor = color; // 同时设置网格颜色
    update();
}

void SimpleSquareWidget::setMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& faces) {
    qDebug() << "Setting mesh data - Vertices:" << vertices.size() << "Faces:" << faces.size();
    
    meshVertices = vertices;
    meshFaces = faces;
    meshLoaded = !vertices.empty() && !faces.empty();
    
    qDebug() << "Mesh loaded:" << meshLoaded;
    
    if (meshLoaded) {
        makeCurrent();
        setupMesh();
        doneCurrent();
    }
    
    update();
}

void SimpleSquareWidget::clearMeshData() {
    qDebug() << "Clearing mesh data";
    meshVertices.clear();
    meshFaces.clear();
    meshLoaded = false;
    update();
}

void SimpleSquareWidget::initializeGL() {
    BaseGLWidget::initializeGL();
    
    // 设置背景颜色
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 启用多重采样抗锯齿
    glEnable(GL_MULTISAMPLE);

    // Initialize square
    squareVao.create();
    squareVbo.create();
    squareEbo.create();
    
    // Initialize mesh
    meshVao.create();
    meshVbo.create();
    meshEbo.create();
    
    setupSquare();
    
    if (meshLoaded) {
        setupMesh();
    }
}

void SimpleSquareWidget::setupSquare() {
    // 先移除旧的着色器
    squareProgram.removeAllShaders();
    
    // 使用动态的正方形尺寸，在paintGL中通过投影矩阵控制大小
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  // 左下角
         1.0f, -1.0f, 0.0f,  // 右下角
         1.0f,  1.0f, 0.0f,  // 右上角
        -1.0f,  1.0f, 0.0f   // 左上角
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    squareVao.bind();
    squareVbo.bind();
    squareVbo.allocate(vertices, sizeof(vertices));
    
    squareEbo.bind();
    squareEbo.allocate(indices, sizeof(indices));
    
    squareProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n");
    
    squareProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 squareColor;\n"
        "void main() {\n"
        "   FragColor = squareColor;\n"
        "}\n");
    squareProgram.link();
    
    squareProgram.bind();
    int posLoc = squareProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        squareProgram.enableAttributeArray(posLoc);
        squareProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    squareVao.release();
    squareProgram.release();
}

void SimpleSquareWidget::setupMesh() {
    if (meshVertices.empty() || meshFaces.empty()) {
        qDebug() << "Mesh data is empty, skipping setup";
        return;
    }
    
    qDebug() << "Setting up mesh with" << meshVertices.size() << "vertices and" << meshFaces.size() << "faces";
    
    // 先移除旧的着色器
    meshProgram.removeAllShaders();
    
    meshVao.bind();
    meshVbo.bind();
    meshVbo.allocate(meshVertices.data(), meshVertices.size() * sizeof(float));
    
    meshEbo.bind();
    meshEbo.allocate(meshFaces.data(), meshFaces.size() * sizeof(unsigned int));
    
    meshProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n");
    
    meshProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 meshColor;\n"
        "void main() {\n"
        "   FragColor = meshColor;\n"
        "}\n");
    
    if (!meshProgram.link()) {
        qDebug() << "Mesh program link failed:" << meshProgram.log();
        return;
    }
    
    meshProgram.bind();
    int posLoc = meshProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        meshProgram.enableAttributeArray(posLoc);
        meshProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    meshVao.release();
    meshProgram.release();
    
    qDebug() << "Mesh setup completed successfully";
}

void SimpleSquareWidget::resizeGL(int w, int h) {
    BaseGLWidget::resizeGL(w, h);
    
    // 创建保持宽高比的投影矩阵
    projection.setToIdentity();
    
    if (w > h) {
        // 宽屏 - 正方形适应高度
        float aspect = static_cast<float>(w) / h;
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        // 竖屏 - 正方形适应宽度
        float aspect = static_cast<float>(h) / w;
        projection.ortho(-1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
    }
}

void SimpleSquareWidget::paintGL() {
    // 先调用基类的清除缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (meshLoaded && !meshVertices.empty() && !meshFaces.empty()) {
        qDebug() << "Drawing parameterized mesh - Vertices:" << meshVertices.size() << "Faces:" << meshFaces.size();
        
        // Draw parameterized mesh with fill - 使用白色填充
        meshProgram.bind();
        meshVao.bind();
        meshEbo.bind();
        
        meshProgram.setUniformValue("projection", projection);
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(meshColor.redF(), meshColor.greenF(), 
                                             meshColor.blueF(), meshColor.alphaF())); // 使用meshColor
        
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        
        // Draw wireframe - 修复线框显示问题
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0); // 确保线框显示在填充之上
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f); // 增加线宽以便更清晰可见
        
        // 使用黑色线框
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(0.0f, 0.0f, 0.0f, 1.0f)); // 黑色线框
        
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        
        glDisable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        meshEbo.release();
        meshVao.release();
        meshProgram.release();
        
        qDebug() << "Mesh drawing completed";
    } else {
        qDebug() << "Drawing default square";
        // Draw square (fallback)
        squareProgram.bind();
        squareVao.bind();
        squareEbo.bind();
        
        squareProgram.setUniformValue("projection", projection);
        squareProgram.setUniformValue("squareColor", 
                                     QVector4D(squareColor.redF(), squareColor.greenF(), 
                                               squareColor.blueF(), squareColor.alphaF()));
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        squareEbo.release();
        squareVao.release();
        squareProgram.release();
    }
}

// 迁移的参数化权重计算方法
std::map<int, float> SimpleSquareWidget::computeWeightsForVertex(Mesh::VertexHandle vh, ParameterizationMethod method) {
    std::map<int, float> weights;
    int vertexId = vh.idx();
    
    // 获取邻居顶点并按顺序排列
    std::vector<Mesh::VertexHandle> neighbors;
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        neighbors.push_back(*vv_it);
    }
    
    int degree = neighbors.size();
    
    switch (method) {
    case UniformTutte:
        // Tutte均匀参数化：λ = 1/degree
        for (auto neighbor : neighbors) {
            weights[neighbor.idx()] = 1.0f / degree;
        }
        break;
        
    case WeightedTutte:
    {
        float totalWeight = 0.0f;
        auto centerPos = openMesh.point(vh);
        
        // 使用论文中的权重：w = 1/dist^q，q=1
        float q = 0.5f;
        std::vector<float> rawWeights;
        
        for (auto neighbor : neighbors) {
            auto neighborPos = openMesh.point(neighbor);
            float dist = (neighborPos - centerPos).norm();
            
            // 避免除零
            float eps = 1e-6f;
            float w = 1.0f / pow(dist + eps, q);
            rawWeights.push_back(w);
            totalWeight += w;
        }
        
        // 归一化
        for (size_t i = 0; i < neighbors.size(); i++) {
            weights[neighbors[i].idx()] = rawWeights[i] / totalWeight;
        }
    }
    break;
        
    case FloaterShapePreserving:
        // 实现论文第6节的形状保持参数化
    {
        if (degree < 3) {
            // 度小于3的情况，退化为均匀权重
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = 1.0f / degree;
            }
            break;
        }
        
        // 1. 获取邻居并按逆时针顺序排序（重要！）
        // OpenMesh不保证顺序，我们需要按角度排序
        auto center3D = openMesh.point(vh);
        
        // 创建一个带角度信息的邻居列表
        struct NeighborInfo {
            Mesh::VertexHandle vh;
            float angle;  // 在切平面中的角度
            float distance;
        };
        
        std::vector<NeighborInfo> neighborInfos;
        
        // 估计切平面法向量（取邻域面法向平均）
        Eigen::Vector3f normal(0, 0, 0);
        for (auto fh : openMesh.vf_range(vh)) {
            auto fn = openMesh.normal(fh);
            normal += Eigen::Vector3f(fn[0], fn[1], fn[2]);
        }
        if (normal.norm() > 0) normal.normalize();
        
        // 创建一个参考向量（任意方向）
        Eigen::Vector3f refVec(1, 0, 0);
        // 如果refVec与normal平行，调整一下
        if (std::abs(refVec.dot(normal)) > 0.99f) {
            refVec = Eigen::Vector3f(0, 1, 0);
        }
        
        // 创建切平面的基向量
        Eigen::Vector3f u = refVec - normal.dot(refVec) * normal;
        u.normalize();
        Eigen::Vector3f v = normal.cross(u);
        
        // 将邻居投影到切平面并计算角度
        for (auto neighbor : neighbors) {
            auto neighborPos = openMesh.point(neighbor);
            Eigen::Vector3f vec3D(neighborPos[0] - center3D[0],
                                neighborPos[1] - center3D[1],
                                neighborPos[2] - center3D[2]);
            
            // 投影到切平面
            float x = vec3D.dot(u);
            float y = vec3D.dot(v);
            
            float angle = atan2(y, x);  // [-π, π]
            if (angle < 0) angle += 2 * M_PI;  // [0, 2π)
            
            NeighborInfo info;
            info.vh = neighbor;
            info.angle = angle;
            info.distance = vec3D.norm();
            neighborInfos.push_back(info);
        }
        
        // 按角度排序
        std::sort(neighborInfos.begin(), neighborInfos.end(),
                [](const NeighborInfo& a, const NeighborInfo& b) {
                    return a.angle < b.angle;
                });
        
        // 2. 计算3D中的角度和总角度（论文公式13）
        std::vector<float> angles3D(degree);
        float totalAngle3D = 0.0f;
        
        for (int i = 0; i < degree; i++) {
            int j = (i + 1) % degree;
            
            // 计算向量
            auto pos_i = openMesh.point(neighborInfos[i].vh);
            auto pos_j = openMesh.point(neighborInfos[j].vh);
            
            Eigen::Vector3f vec_i(pos_i[0] - center3D[0],
                                pos_i[1] - center3D[1],
                                pos_i[2] - center3D[2]);
            Eigen::Vector3f vec_j(pos_j[0] - center3D[0],
                                pos_j[1] - center3D[1],
                                pos_j[2] - center3D[2]);
            
            // 计算角度
            float cos_angle = vec_i.dot(vec_j) / (vec_i.norm() * vec_j.norm() + 1e-6f);
            cos_angle = std::max(-1.0f, std::min(1.0f, cos_angle));
            float angle = acos(cos_angle);
            
            angles3D[i] = angle;
            totalAngle3D += angle;
        }
        
        // 3. 创建2D局部映射（论文公式13）
        std::vector<Eigen::Vector2f> neighbor2DPoints(degree);
        
        // 第一个邻居在x轴正方向上
        neighbor2DPoints[0] = Eigen::Vector2f(neighborInfos[0].distance, 0.0f);
        
        // 计算累积角度
        float accumulatedAngle = 0.0f;
        for (int i = 1; i < degree; i++) {
            // 角度比例缩放：2π * (3D角度 / 总3D角度)
            accumulatedAngle += 2.0f * M_PI * angles3D[i-1] / totalAngle3D;
            
            neighbor2DPoints[i] = Eigen::Vector2f(
                neighborInfos[i].distance * cos(accumulatedAngle),
                neighborInfos[i].distance * sin(accumulatedAngle)
            );
        }
        
        // 4. 计算权重（改进的射线法）
        Eigen::Vector2f center2D(0.0f, 0.0f);
        
        // 检查是否为凸多边形且中心在内部
        bool isConvex = true;
        for (int i = 0; i < degree; i++) {
            Eigen::Vector2f p1 = neighbor2DPoints[i];
            Eigen::Vector2f p2 = neighbor2DPoints[(i+1)%degree];
            Eigen::Vector2f p3 = neighbor2DPoints[(i+2)%degree];
            
            float cross = (p2 - p1).x() * (p3 - p2).y() - (p2 - p1).y() * (p3 - p2).x();
            if (cross < 0) {
                isConvex = false;
                break;
            }
        }
        
        if (!isConvex || degree == 3) {
            // 如果非凸或只有3个邻居，使用均值坐标作为后备方案
            // 均值坐标通常能产生正权重且形状保持较好
            
            std::vector<float> meanValueWeights(degree, 0.0f);
            float totalWeight = 0.0f;
            
            for (int i = 0; i < degree; i++) {
                int prev = (i - 1 + degree) % degree;
                int next = (i + 1) % degree;
                
                Eigen::Vector2f v_prev = neighbor2DPoints[prev] - center2D;
                Eigen::Vector2f v_curr = neighbor2DPoints[i] - center2D;
                Eigen::Vector2f v_next = neighbor2DPoints[next] - center2D;
                
                // 计算两个半角的正切值
                float tan_alpha_2 = tan(0.5f * acos(v_curr.dot(v_next) / 
                    (v_curr.norm() * v_next.norm() + 1e-6f)));
                float tan_beta_2 = tan(0.5f * acos(v_curr.dot(v_prev) / 
                    (v_curr.norm() * v_prev.norm() + 1e-6f)));
                
                float w = (tan_alpha_2 + tan_beta_2) / (v_curr.norm() + 1e-6f);
                meanValueWeights[i] = w;
                totalWeight += w;
            }
            
            // 归一化
            for (int i = 0; i < degree; i++) {
                weights[neighborInfos[i].vh.idx()] = meanValueWeights[i] / totalWeight;
            }
            
        } else {
            // 凸多边形且degree > 3，使用射线法
            std::vector<std::vector<float>> mu(degree, std::vector<float>(degree, 0.0f));
            
            for (int l = 0; l < degree; l++) {
                // 射线方向
                Eigen::Vector2f rayDir = neighbor2DPoints[l].normalized();
                
                // 找到射线与多边形另一侧的交点
                // 我们需要找到第一个使叉积改变符号的边
                int r = -1;
                float bestT = 1e9f;  // 参数t，越小表示交点越近
                
                for (int k = 0; k < degree; k++) {
                    int k_next = (k + 1) % degree;
                    
                    // 跳过相邻边和包含l的边
                    if (k == l || k_next == l || (k - l + degree) % degree == degree - 1) {
                        continue;
                    }
                    
                    Eigen::Vector2f p_k = neighbor2DPoints[k];
                    Eigen::Vector2f p_k_next = neighbor2DPoints[k_next];
                    
                    // 计算射线与线段的交点
                    // 参数方程: ray: p = t * rayDir, segment: p = p_k + s * (p_k_next - p_k)
                    // 求解 t, s
                    
                    Eigen::Vector2f segDir = p_k_next - p_k;
                    
                    // 解线性方程组
                    float det = -rayDir.x() * segDir.y() + rayDir.y() * segDir.x();
                    
                    if (std::abs(det) > 1e-6f) {
                        float t = (-p_k.x() * segDir.y() + p_k.y() * segDir.x()) / det;
                        float s = (-rayDir.x() * p_k.y() + rayDir.y() * p_k.x()) / det;
                        
                        // 检查交点在射线上(t>0)且在线段上(0<=s<=1)
                        if (t > 1e-6f && s >= 0.0f && s <= 1.0f && t < bestT) {
                            bestT = t;
                            r = k;
                        }
                    }
                }
                
                if (r == -1) {
                    // 没找到交点，使用对面的顶点
                    r = (l + degree/2) % degree;
                    bestT = (neighbor2DPoints[r] - center2D).norm();
                }
                
                // 现在我们有交点 Q = bestT * rayDir
                Eigen::Vector2f Q = bestT * rayDir;
                
                // 计算重心坐标
                int r_next = (r + 1) % degree;
                Eigen::Vector2f p_l = neighbor2DPoints[l];
                Eigen::Vector2f p_r = neighbor2DPoints[r];
                Eigen::Vector2f p_r_next = neighbor2DPoints[r_next];
                
                // 如果Q接近p_r，使用p_r作为交点
                if ((Q - p_r).norm() < 1e-3f) {
                    // Q在顶点p_r上
                    // 中心点表示为p_l和p_r的凸组合
                    float alpha = 0.5f;  // 简化处理
                    float beta = 0.5f;
                    
                    mu[l][l] = alpha;
                    mu[l][r] = beta;
                } else {
                    // Q在边(p_r, p_r_next)上
                    // 将Q表示为p_r和p_r_next的凸组合
                    float s = (Q - p_r).norm() / (p_r_next - p_r).norm();
                    s = std::max(0.0f, std::min(1.0f, s));
                    
                    // 现在将中心点表示为p_l, p_r, p_r_next的凸组合
                    // 简化：假设中心点在线段(p_l, Q)上
                    float t = center2D.norm() / Q.norm();
                    
                    // 重心坐标
                    float alpha = 1.0f - t;
                    float beta = t * (1.0f - s);
                    float gamma = t * s;
                    
                    mu[l][l] = alpha;
                    mu[l][r] = beta;
                    mu[l][r_next] = gamma;
                }
            }
            
            // 计算最终的 λ（公式15）
            for (int k = 0; k < degree; k++) {
                float sum = 0.0f;
                for (int l = 0; l < degree; l++) {
                    sum += mu[l][k];
                }
                weights[neighborInfos[k].vh.idx()] = sum / degree;
            }
        }
        
        // 确保所有权重为正且和为1
        float weightSum = 0.0f;
        for (auto& kv : weights) {
            kv.second = std::max(kv.second, 0.0f);
            weightSum += kv.second;
        }
        
        if (weightSum > 0) {
            for (auto& kv : weights) {
                kv.second /= weightSum;
            }
        } else {
            // 如果所有权重为0，使用均匀权重
            for (int i = 0; i < degree; i++) {
                weights[neighborInfos[i].vh.idx()] = 1.0f / degree;
            }
        }
    }
    break;
        
    case OriginalMethod:
    default:
        // 原来的余切权重方法
    {
        float totalWeight = 0.0f;
        
        // 遍历所有出边的一半边
        for (auto heh : openMesh.voh_range(vh)) {
            if (!openMesh.is_boundary(heh)) {
                Mesh::VertexHandle vj = openMesh.to_vertex_handle(heh);
                float weight = computeCotangentWeight(heh);
                
                if (weight > 0) {
                    weights[vj.idx()] = weight;
                    totalWeight += weight;
                }
            }
        }
        
        // 归一化权重
        if (totalWeight > 0) {
            for (auto& kv : weights) {
                kv.second /= totalWeight;
            }
        }
    }
    break;
    }
    
    return weights;
}

float SimpleSquareWidget::computeCotangentWeight(Mesh::HalfedgeHandle heh) {
    // 获取半边对应的两个三角形并计算余切权重
    if (!openMesh.is_boundary(heh)) {
        // 主半边
        Mesh::HalfedgeHandle heh_opp = openMesh.opposite_halfedge_handle(heh);
        
        // 获取顶点
        Mesh::VertexHandle v0 = openMesh.from_vertex_handle(heh);
        Mesh::VertexHandle v1 = openMesh.to_vertex_handle(heh);
        
        // 第一个三角形：v0-v1-v2
        Mesh::HalfedgeHandle heh_next = openMesh.next_halfedge_handle(heh);
        Mesh::VertexHandle v2 = openMesh.to_vertex_handle(heh_next);
        
        // 第二个三角形：v1-v0-v3
        Mesh::HalfedgeHandle heh_opp_next = openMesh.next_halfedge_handle(heh_opp);
        Mesh::VertexHandle v3 = openMesh.to_vertex_handle(heh_opp_next);
        
        // 计算两个角度的余切值
        auto p0 = openMesh.point(v0);
        auto p1 = openMesh.point(v1);
        auto p2 = openMesh.point(v2);
        auto p3 = openMesh.point(v3);
        
        // 向量计算
        auto e01 = p1 - p0;
        auto e02 = p2 - p0;
        auto e10 = p0 - p1;
        auto e13 = p3 - p1;
        
        // 计算角度并避免数值问题
        float cos_alpha = (e01 | e02) / (sqrt((e01 | e01) * (e02 | e02)) + 1e-10);
        float cos_beta = (e10 | e13) / (sqrt((e10 | e10) * (e13 | e13)) + 1e-10);
        
        cos_alpha = std::max(-1.0f, std::min(1.0f, cos_alpha));
        cos_beta = std::max(-1.0f, std::min(1.0f, cos_beta));
        
        float sin_alpha = sqrt(1.0f - cos_alpha * cos_alpha);
        float sin_beta = sqrt(1.0f - cos_beta * cos_beta);
        
        // 余切权重
        float cot_alpha = cos_alpha / (sin_alpha + 1e-10);
        float cot_beta = cos_beta / (sin_beta + 1e-10);
        
        return (cot_alpha + cot_beta) / 2.0f;
    }
    
    return 0.0f;
}

void SimpleSquareWidget::solveParameterizationInternal(ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }
    
    // 构建线性方程组
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
    
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        
        if (isBoundary[i]) {
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            b_u[i] = openMesh.point(vh)[0];
            b_v[i] = openMesh.point(vh)[1];
        } else {
            // 内部顶点：根据方法计算权重
            auto weights = computeWeightsForVertex(vh, method);
            
            float totalWeight = 0.0f;
            for (const auto& [j, w] : weights) {
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            triplets.push_back(Triplet(i, i, -totalWeight));
            b_u[i] = 0.0f;
            b_v[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    // 使用SparseLU求解器
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        qWarning() << "Matrix factorization failed!";
        return;
    }
    
    // 求解坐标
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(i), newPos);
    }
}

// 参数化相关方法实现
void SimpleSquareWidget::mapBoundaryToCircle() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 查找起始边界点
    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    // 获取有序边界顶点
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    // 查找下一个边界点
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    // 遍历边界
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

    // 计算总弧长
    float arc_len = 0.0f;
    for(int i = 1; i < boundary.size(); ++i) {
        arc_len += (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
    }
    arc_len += (openMesh.point(boundary[boundary.size()-1]) - openMesh.point(boundary[0])).norm();
    
    // 计算每段弧长对应的角度增量
    std::vector<float> delta;
    for(int i = 1; i < boundary.size(); ++i) {
        float seg_len = (openMesh.point(boundary[i]) - openMesh.point(boundary[i-1])).norm();
        delta.push_back(2.0f * M_PI * (seg_len / arc_len));
    }
    
    // 映射到单位圆
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

    // 查找起始边界点
    Mesh::VertexHandle start_v;
    for(auto vh : openMesh.vertices()) {
        if(openMesh.is_boundary(vh)) {
            start_v = vh;
            break;
        }
    }
    
    // 获取有序边界顶点
    std::vector<Mesh::VertexHandle> boundary;
    Mesh::VertexHandle pre, now;
    boundary.push_back(start_v);
    now = start_v;
    
    // 查找下一个边界点
    for(auto vv_it = openMesh.vv_begin(now); vv_it != openMesh.vv_end(now); ++vv_it) {
        if(openMesh.is_boundary(*vv_it)) {
            pre = now;
            now = *vv_it;
            break;
        }
    }
    
    // 遍历边界
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
    const float length = 1.0f; // 正方形边长
    
    // 计算四条边上的点数（尽可能平均分配）
    int side1 = n / 4;
    int side2 = n / 4;
    int side3 = n / 4;
    int side4 = n - 3 * (n / 4);
    
    // 设置四个角点
    openMesh.set_point(boundary[0], Mesh::Point(0.0f, 0.0f, 0.0f));
    openMesh.set_point(boundary[side1], Mesh::Point(0.0f, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2], Mesh::Point(length, length, 0.0f));
    openMesh.set_point(boundary[side1 + side2 + side3], Mesh::Point(length, 0.0f, 0.0f));
    
    // 左边 (y: 0 → length)
    float delta = length / side1;
    for (int i = 1; i < side1; ++i) {
        float y = i * delta;
        openMesh.set_point(boundary[i], Mesh::Point(0.0f, y, 0.0f));
    }
    
    // 上边 (x: 0 → length)
    delta = length / side2;
    for (int i = 1; i < side2; ++i) {
        int idx = side1 + i;
        float x = i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, length, 0.0f));
    }
    
    // 右边 (y: length → 0)
    delta = length / side3;
    for (int i = 1; i < side3; ++i) {
        int idx = side1 + side2 + i;
        float y = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(length, y, 0.0f));
    }
    
    // 下边 (x: length → 0)
    delta = length / side4;
    for (int i = 1; i < side4; ++i) {
        int idx = side1 + side2 + side3 + i;
        float x = length - i * delta;
        openMesh.set_point(boundary[idx], Mesh::Point(x, 0.0f, 0.0f));
    }
}

void SimpleSquareWidget::normalizeMesh() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 计算边界框
    Mesh::Point min(1e9, 1e9, 0), max(-1e9, -1e9, 0);
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        min[0] = std::min(min[0], p[0]);
        min[1] = std::min(min[1], p[1]);
        max[0] = std::max(max[0], p[0]);
        max[1] = std::max(max[1], p[1]);
    }
    
    // 计算中心点和范围
    Mesh::Point center((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, 0);
    float range_x = max[0] - min[0];
    float range_y = max[1] - min[1];
    
    // 获取视图尺寸
    int viewWidth = width();
    int viewHeight = height();
    float aspectRatio = static_cast<float>(viewWidth) / viewHeight;
    
    // 计算缩放因子 - 使用最小边
    float scaleFactor;
    if (aspectRatio > 1.0f) {
        // 宽屏 - 以高度为基准
        scaleFactor = 2.0f / (range_y > 0 ? range_y : 1.0f);
    } else {
        // 竖屏 - 以宽度为基准
        scaleFactor = 2.0f / (range_x > 0 ? range_x : 1.0f);
    }
    
    // 归一化所有顶点
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        p -= center; // 平移到中心
        p[0] *= scaleFactor;
        p[1] *= scaleFactor;
        p[2] = 0.0f;
        openMesh.set_point(vh, p);
    }
}

// 原来的参数化方法（使用余切权重）
void SimpleSquareWidget::solveParameterizationOriginal() {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }

    // 准备数据：计算每个顶点的余切权重
    std::vector<std::map<int, float>> weights(openMesh.n_vertices());
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        for (auto heh : openMesh.voh_range(vh)) {
            if (!openMesh.is_boundary(heh)) {
                auto vj = openMesh.to_vertex_handle(heh);
                int j = vj.idx();
                
                // 计算两个相邻三角形的角度
                auto from = openMesh.from_vertex_handle(heh);
                auto to = openMesh.to_vertex_handle(heh);
                auto next = openMesh.next_halfedge_handle(heh);
                auto opp_next = openMesh.next_halfedge_handle(openMesh.opposite_halfedge_handle(heh));
                
                auto p1 = openMesh.point(from);
                auto p2 = openMesh.point(to);
                auto p3 = openMesh.point(openMesh.to_vertex_handle(next));
                auto p4 = openMesh.point(openMesh.to_vertex_handle(opp_next));
                
                // 计算两个角度
                Eigen::Vector3f v1 = {p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]};
                Eigen::Vector3f v2 = {p3[0]-p2[0], p3[1]-p2[1], p3[2]-p2[2]};
                Eigen::Vector3f v3 = {p4[0]-p2[0], p4[1]-p2[1], p4[2]-p2[2]};
                
                float angle1 = acos(v1.dot(v2) / (v1.norm() * v2.norm()));
                float angle2 = acos(v1.dot(v3) / (v1.norm() * v3.norm()));
                
                // 计算余切权重
                float w1 = 1.0f / tan(angle1);
                float w2 = 1.0f / tan(angle2);
                float w = (w1 + w2) / 2.0f;  // 平均权重
                
                weights[i][j] = w;
            }
        }
    }

    // 构建线性方程组
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
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            b_u[i] = openMesh.point(Mesh::VertexHandle(i))[0];
            b_v[i] = openMesh.point(Mesh::VertexHandle(i))[1];
        } else {
            // 内部顶点：使用余切权重
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
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    // 使用SparseLU求解器
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        std::cerr << "Matrix factorization failed!" << std::endl;
        return;
    }
    
    // 求解坐标
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(i), newPos);
    }
}

// 新增的通用参数化方法
void SimpleSquareWidget::solveParameterization() {
    // 根据当前选择的方法调用不同的实现
    if (currentParamMethod == OriginalMethod) {
        solveParameterizationOriginal();  // 调用原来的方法
    } else {
        // 调用迁移过来的新方法
        solveParameterizationInternal(currentParamMethod);
    }
}

void SimpleSquareWidget::performParameterization(BoundaryType boundaryType, ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    // 保存原始网格
    Mesh original = openMesh;
    
    // 根据边界类型映射边界
    if (boundaryType == Circle) {
        mapBoundaryToCircle();
    } else {
        mapBoundaryToRectangle();
    }
    
    // 设置当前参数化方法
    currentParamMethod = method;
    
    // 执行参数化
    solveParameterization();
    
    // 归一化网格
    normalizeMesh();
    
    // 保存参数化结果 - 确保正确提取顶点数据
    paramVertices.clear();
    paramFaces = faces;  // 面索引保持不变
    
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        paramVertices.push_back(p[0]);
        paramVertices.push_back(p[1]);
        paramVertices.push_back(0.0f); // z坐标为0
    }
    
    // 恢复原始网格
    openMesh = original;
    
    parameterized = true;
    
    qDebug() << "Parameterization completed using method:" << method 
             << "Vertices:" << paramVertices.size() / 3 << "Faces:" << paramFaces.size() / 3;
}