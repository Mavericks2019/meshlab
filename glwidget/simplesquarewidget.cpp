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

// 修改后的权重计算方法：返回未归一化的原始权重
std::map<int, float> SimpleSquareWidget::computeWeightsForVertex(Mesh::VertexHandle vh, ParameterizationMethod method) {
    std::map<int, float> weights;
    
    // 获取邻居顶点
    std::vector<Mesh::VertexHandle> neighbors;
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        neighbors.push_back(*vv_it);
    }
    
    int degree = neighbors.size();
    if (degree == 0) return weights;  // 没有邻居，返回空权重
    
    switch (method) {
    case UniformTutte:
        // Tutte均匀参数化：每个邻居权重为1
        for (auto neighbor : neighbors) {
            weights[neighbor.idx()] = 1.0f;
        }
        // 归一化
        {
            float sum = degree;
            for (auto& [idx, w] : weights) {
                w /= sum;
            }
        }
        break;
        
    case WeightedTutte:
    {
        auto centerPos = openMesh.point(vh);
        float totalWeight = 0.0f;
        
        // 使用弦长权重：w = 1/dist^q，这里q=0.2
        for (auto neighbor : neighbors) {
            auto neighborPos = openMesh.point(neighbor);
            float dist = (neighborPos - centerPos).norm();
            float weight = 1.0f / pow(dist + 1e-6f, 0.2f);  // 加小值避免除零
            weights[neighbor.idx()] = weight;
            totalWeight += weight;
        }
        
        // 归一化
        if (totalWeight > 1e-10) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        } else {
            // 如果总权重太小，使用均匀权重
            float uniformWeight = 1.0f / degree;
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = uniformWeight;
            }
        }
    }
    break;
        
    case FloaterShapePreserving:
    {
        // 保形参数化权重计算
        auto centerPos = openMesh.point(vh);
        
        // 步骤1: 计算局部映射
        std::vector<float> angles;
        std::vector<float> lengths;
        
        // 计算每个邻居的距离
        for (int i = 0; i < degree; i++) {
            auto neighbor = neighbors[i];
            auto neighborPos = openMesh.point(neighbor);
            float dist = (neighborPos - centerPos).norm();
            lengths.push_back(dist);
        }
        
        // 计算角度（需要邻居按顺序排列）
        for (int i = 0; i < degree; i++) {
            int next_i = (i + 1) % degree;
            auto neighbor1 = neighbors[i];
            auto neighbor2 = neighbors[next_i];
            
            auto pos1 = openMesh.point(neighbor1);
            auto pos2 = openMesh.point(neighbor2);
            
            // 计算向量
            auto vec1 = pos1 - centerPos;
            auto vec2 = pos2 - centerPos;
            
            // 计算夹角
            float dot = vec1[0]*vec2[0] + vec1[1]*vec2[1] + vec1[2]*vec2[2];
            float len1 = sqrt(vec1[0]*vec1[0] + vec1[1]*vec1[1] + vec1[2]*vec1[2]);
            float len2 = sqrt(vec2[0]*vec2[0] + vec2[1]*vec2[1] + vec2[2]*vec2[2]);
            float cosAngle = dot / (len1 * len2 + 1e-10);
            cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
            float angle = acos(cosAngle);
            angles.push_back(angle);
        }
        
        // 步骤2: 在平面上创建局部映射
        std::vector<Eigen::Vector2f> planePoints;
        float totalAngle = 0;
        for (float ang : angles) totalAngle += ang;
        
        // 避免除零
        if (totalAngle < 1e-10) {
            // 退化情况，使用均匀权重
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = 1.0f / degree;
            }
            break;
        }
        
        // 第一个点在x轴上
        float currentAngle = 0;
        planePoints.push_back(Eigen::Vector2f(lengths[0], 0));
        
        // 其他点按角度比例放置
        for (int i = 1; i < degree; i++) {
            currentAngle += angles[i-1] * 2 * M_PI / totalAngle;
            float x = lengths[i] * cos(currentAngle);
            float y = lengths[i] * sin(currentAngle);
            planePoints.push_back(Eigen::Vector2f(x, y));
        }
        
        // 辅助函数：计算二维叉积（返回标量）
        auto cross2D = [](const Eigen::Vector2f& a, const Eigen::Vector2f& b) {
            return a.x() * b.y() - a.y() * b.x();
        };
        
        // 辅助函数：计算三角形面积
        auto triangleArea = [&cross2D](const Eigen::Vector2f& a, 
                                       const Eigen::Vector2f& b, 
                                       const Eigen::Vector2f& c) {
            return fabs(cross2D(b - a, c - a)) * 0.5f;
        };
        
        // 定义原点
        Eigen::Vector2f origin(0, 0);
        
        // 步骤3: 计算权重
        if (degree == 3) {
            // 三角形情况：使用重心坐标
            Eigen::Vector2f p1 = planePoints[0];
            Eigen::Vector2f p2 = planePoints[1];
            Eigen::Vector2f p3 = planePoints[2];
            
            float area123 = triangleArea(p1, p2, p3);
            
            if (area123 > 1e-10) {
                float area023 = triangleArea(origin, p2, p3);
                float area013 = triangleArea(p1, origin, p3);
                float area012 = triangleArea(p1, p2, origin);
                
                weights[neighbors[0].idx()] = area023 / area123;
                weights[neighbors[1].idx()] = area013 / area123;
                weights[neighbors[2].idx()] = area012 / area123;
            } else {
                // 退化情况，使用均匀权重
                for (auto neighbor : neighbors) {
                    weights[neighbor.idx()] = 1.0f / degree;
                }
            }
        } else {
            // 大于3个邻居：使用Floater的方法
            // 初始化Mu矩阵
            Eigen::MatrixXf Mu = Eigen::MatrixXf::Zero(degree, degree);
            
            for (int l = 0; l < degree; l++) {
                Eigen::Vector2f pl = planePoints[l];
                
                // 找到与pl相对的边或顶点
                bool found = false;
                
                for (int k = 0; k < degree && !found; k++) {
                    int k_next = (k + 1) % degree;
                    
                    if (k == l || k_next == l) continue;
                    
                    Eigen::Vector2f pk = planePoints[k];
                    Eigen::Vector2f pk_next = planePoints[k_next];
                    
                    // 检查原点是否在三角形(pl, pk, pk_next)内
                    // 使用重心坐标方法
                    Eigen::Vector2f v0 = pk_next - pk;
                    Eigen::Vector2f v1 = pl - pk;
                    Eigen::Vector2f v2 = origin - pk;
                    
                    float dot00 = v0.dot(v0);
                    float dot01 = v0.dot(v1);
                    float dot02 = v0.dot(v2);
                    float dot11 = v1.dot(v1);
                    float dot12 = v1.dot(v2);
                    
                    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01 + 1e-10);
                    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
                    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
                    
                    if (u >= -1e-6 && v >= -1e-6 && (u + v) <= 1.0 + 1e-6) {
                        // 原点在三角形内
                        float area = triangleArea(pk, pk_next, pl);
                        
                        if (area > 1e-10) {
                            float area0 = triangleArea(origin, pk_next, pl);
                            float area1 = triangleArea(origin, pl, pk);
                            float area2 = triangleArea(origin, pk, pk_next);
                            
                            Mu(l, l) = area0 / area;
                            Mu(k, l) = area1 / area;
                            Mu(k_next, l) = area2 / area;
                        }
                        found = true;
                    }
                }
                
                // 如果没找到三角形，使用均匀权重
                if (!found) {
                    Mu(l, l) = 1.0f;
                }
            }
            
            // 计算平均权重
            for (int k = 0; k < degree; k++) {
                float sum = 0;
                for (int l = 0; l < degree; l++) {
                    sum += Mu(k, l);
                }
                weights[neighbors[k].idx()] = sum / degree;
            }
        }
        
        // 确保权重已归一化
        float sum = 0.0f;
        for (const auto& [idx, w] : weights) {
            sum += w;
        }
        if (sum > 1e-10) {
            for (auto& [idx, w] : weights) {
                w /= sum;
            }
        } else {
            // 如果和为0，使用均匀权重
            float uniformWeight = 1.0f / degree;
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = uniformWeight;
            }
        }
    }
    break;
        
    case OriginalMethod:
    default:
        // 原来的余切权重方法
        float totalWeight = 0.0f;
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
        
        // 归一化余切权重
        if (totalWeight > 1e-10) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        } else {
            // 如果总权重太小，使用均匀权重
            for (auto neighbor : neighbors) {
                weights[neighbor.idx()] = 1.0f / neighbors.size();
            }
        }
        break;
    }
    
    // 最后检查：确保所有权重为正且和为1
    float finalSum = 0.0f;
    for (const auto& [idx, w] : weights) {
        if (w < 0) {
            qWarning() << "Warning: Negative weight found at vertex" << vh.idx() 
                       << "for neighbor" << idx << ":" << w;
        }
        finalSum += w;
    }
    
    // 如果和不接近1，重新归一化
    if (fabs(finalSum - 1.0f) > 0.01f && finalSum > 1e-10) {
        qDebug() << "Weights sum is" << finalSum << "for vertex" << vh.idx() 
                 << ", renormalizing...";
        for (auto& [idx, w] : weights) {
            w /= finalSum;
        }
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

// 修改后的参数化内部求解函数，采用参考代码的形式
void SimpleSquareWidget::solveParameterizationInternal(ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }
    
    // 分离内部顶点和边界顶点
    std::vector<int> internalIndices;
    std::vector<int> boundaryIndices;
    
    for (int i = 0; i < openMesh.n_vertices(); i++) {
        if (isBoundary[i]) {
            boundaryIndices.push_back(i);
        } else {
            internalIndices.push_back(i);
        }
    }
    
    int n_internal = internalIndices.size();
    int n_total = openMesh.n_vertices();
    
    // 构建从顶点索引到内部索引的映射
    std::vector<int> vertexToInternalIndex(n_total, -1);
    for (int i = 0; i < n_internal; i++) {
        vertexToInternalIndex[internalIndices[i]] = i;
    }
    
    // 构建线性方程组
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    // 使用归一化形式：u_i - Σ λ_ij * u_j = Σ_{边界} λ_ij * u_j
    SpMat A(n_internal, n_internal);
    VectorXf b_u(n_internal), b_v(n_internal);
    VectorXf x(n_internal), y(n_internal);
    
    b_u.setZero();
    b_v.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n_internal * 10);
    
    // 为每个内部顶点构建方程
    for (int i = 0; i < n_internal; i++) {
        int vertexIdx = internalIndices[i];
        Mesh::VertexHandle vh(vertexIdx);
        
        // 获取权重（已归一化）
        auto weights = computeWeightsForVertex(vh, method);
        
        // 对角线为1（归一化形式）
        triplets.push_back(Triplet(i, i, 1.0f));
        
        // 右侧向量
        float boundarySumU = 0.0f;
        float boundarySumV = 0.0f;
        
        // 遍历所有邻居
        for (const auto& [neighborIdx, w] : weights) {
            if (!isBoundary[neighborIdx]) {
                // 内部邻居：贡献到左侧矩阵，系数为 -λ
                int j = vertexToInternalIndex[neighborIdx];
                triplets.push_back(Triplet(i, j, -w));
            } else {
                // 边界邻居：贡献到右侧向量，系数为 λ * u_j
                auto boundaryPos = openMesh.point(Mesh::VertexHandle(neighborIdx));
                boundarySumU += w * boundaryPos[0];
                boundarySumV += w * boundaryPos[1];
            }
        }
        
        // 设置右侧向量
        b_u(i) = boundarySumU;
        b_v(i) = boundarySumV;
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
        
        // 添加调试信息
        qDebug() << "Matrix size:" << A.rows() << "x" << A.cols();
        qDebug() << "Non-zero entries:" << A.nonZeros();
        qDebug() << "Number of internal vertices:" << n_internal;
        qDebug() << "Solver info:" << solver.info();
        
        // 尝试打印矩阵的一部分来调试
        if (A.rows() < 10) {
            qDebug() << "Matrix A:";
            Eigen::MatrixXf denseA = A.toDense();
            for (int i = 0; i < denseA.rows(); i++) {
                QString rowStr;
                for (int j = 0; j < denseA.cols(); j++) {
                    rowStr += QString::number(denseA(i, j), 'f', 3) + " ";
                }
                qDebug() << rowStr;
            }
        }
        
        return;
    }
    
    // 求解坐标
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    // 更新内部顶点位置
    for (int i = 0; i < n_internal; i++) {
        int vertexIdx = internalIndices[i];
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(vertexIdx), newPos);
    }
    
    // 添加求解后的调试信息
    qDebug() << "Parameterization solved successfully";
    qDebug() << "Internal vertices count:" << n_internal;
    
    // 检查解的质量
    VectorXf residual_u = A * x - b_u;
    VectorXf residual_v = A * y - b_v;
    qDebug() << "Residual norm u:" << residual_u.norm();
    qDebug() << "Residual norm v:" << residual_v.norm();
    
    // 检查坐标范围
    float min_u = x.minCoeff();
    float max_u = x.maxCoeff();
    float min_v = y.minCoeff();
    float max_v = y.maxCoeff();
    qDebug() << "u range: [" << min_u << ", " << max_u << "]";
    qDebug() << "v range: [" << min_v << ", " << max_v << "]";
}

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