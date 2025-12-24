// simplesquarewidget.cpp
#include "simplesquarewidget.h"
#include <QDebug>
#include <set>
#include <QFile>
#include <fstream>

const float EPSILON = 1e-6f;

SimpleSquareWidget::SimpleSquareWidget(QWidget *parent) : BaseGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    meshVbo(QOpenGLBuffer::VertexBuffer),
    meshEbo(QOpenGLBuffer::IndexBuffer),
    squareSize(1.0f)
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
    meshColor = color;
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
    flippedTriangles.clear();
    update();
}

void SimpleSquareWidget::initializeGL() {
    BaseGLWidget::initializeGL();
    
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    squareVao.create();
    squareVbo.create();
    squareEbo.create();
    meshVao.create();
    meshVbo.create();
    meshEbo.create();
    
    setupSquare();
    
    if (meshLoaded) {
        setupMesh();
    }
}

void SimpleSquareWidget::setupSquare() {
    squareProgram.removeAllShaders();
    
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
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
    
    projection.setToIdentity();
    
    if (w > h) {
        float aspect = static_cast<float>(w) / h;
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        float aspect = static_cast<float>(h) / w;
        projection.ortho(-1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
    }
}

void SimpleSquareWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (meshLoaded && !meshVertices.empty() && !meshFaces.empty()) {
        qDebug() << "Drawing parameterized mesh - Vertices:" << meshVertices.size() << "Faces:" << meshFaces.size();
        
        meshProgram.bind();
        meshVao.bind();
        meshEbo.bind();
        
        meshProgram.setUniformValue("projection", projection);
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(meshColor.redF(), meshColor.greenF(), 
                                             meshColor.blueF(), meshColor.alphaF()));
        
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        
        glDisable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        meshEbo.release();
        meshVao.release();
        meshProgram.release();
        
        qDebug() << "Mesh drawing completed";
    } else {
        qDebug() << "Drawing default square";
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

// 新增函数：计算变形梯度算子
DeformationGradientOperator SimpleSquareWidget::computeDeformationGradientOperator(
    const std::vector<float>& vertices, 
    const std::vector<unsigned int>& faces) const {
    
    DeformationGradientOperator result;
    
    int nv = vertices.size() / 3;  // 顶点数量
    int nf = faces.size() / 3;     // 面数量
    
    if (nv == 0 || nf == 0) {
        qDebug() << "Error: Empty mesh data";
        return result;
    }
    
    // 将顶点转换为Eigen格式以便计算
    std::vector<Eigen::Vector3d> V(nv);
    for (int i = 0; i < nv; i++) {
        V[i] = Eigen::Vector3d(
            vertices[i * 3],
            vertices[i * 3 + 1],
            vertices[i * 3 + 2]
        );
    }
    
    // 初始化面积向量
    result.areas = Eigen::VectorXd::Zero(nf);
    
    // 为 grad_3d 预分配空间 (3*nf x nv 稀疏矩阵)
    std::vector<Eigen::Triplet<double>> triplets_grad3d;
    triplets_grad3d.reserve(9 * nf);
    
    // 为投影矩阵P预分配空间 (2*nf x 3*nf 稀疏矩阵)
    std::vector<Eigen::Triplet<double>> triplets_proj;
    triplets_proj.reserve(6 * nf);
    
    // 遍历所有三角形
    for (int t = 0; t < nf; t++) {
        unsigned int i = faces[t * 3];
        unsigned int j = faces[t * 3 + 1];
        unsigned int k = faces[t * 3 + 2];
        
        // 获取顶点坐标
        Eigen::Vector3d vi = V[i];
        Eigen::Vector3d vj = V[j];
        Eigen::Vector3d vk = V[k];
        
        // 计算边向量
        Eigen::Vector3d e1 = vj - vi;  // 边 vi->vj
        Eigen::Vector3d e2 = vk - vi;  // 边 vi->vk
        Eigen::Vector3d e3 = vk - vj;  // 边 vj->vk
        
        // 计算三角形法向量和面积
        Eigen::Vector3d n = cross(e1, e2);
        double area = norm(n) / 2.0;  // 三角形面积 (n的长度是2倍面积)
        result.areas(t) = area;
        
        // 归一化法向量
        Eigen::Vector3d n_unit = n / (2.0 * area + 1e-12);
        
        // 计算基函数梯度
        // 公式: ∇φ_i = (n × e_jk) / (2A)
        // 其中 e_jk 是与顶点 i 相对的边 (v_k - v_j)
        Eigen::Vector3d grad_phi_i = cross(n_unit, e3) / (2.0 * area + 1e-12);
        Eigen::Vector3d grad_phi_j = cross(n_unit, -e2) / (2.0 * area + 1e-12);  // e_ki = -(v_k - v_i)
        Eigen::Vector3d grad_phi_k = cross(n_unit, e1) / (2.0 * area + 1e-12);
        
        // 添加 grad_3d 的三元组 (每个三角形贡献3行，每行对应一个坐标分量)
        // 行索引: 3*t + 0, 3*t + 1, 3*t + 2 分别对应三角形的 x, y, z 分量
        // 列索引: 顶点索引 i, j, k
        
        // 顶点 i 的贡献
        triplets_grad3d.emplace_back(3*t,     i, grad_phi_i.x());  // x分量
        triplets_grad3d.emplace_back(3*t + 1, i, grad_phi_i.y());  // y分量
        triplets_grad3d.emplace_back(3*t + 2, i, grad_phi_i.z());  // z分量
        
        // 顶点 j 的贡献
        triplets_grad3d.emplace_back(3*t,     j, grad_phi_j.x());
        triplets_grad3d.emplace_back(3*t + 1, j, grad_phi_j.y());
        triplets_grad3d.emplace_back(3*t + 2, j, grad_phi_j.z());
        
        // 顶点 k 的贡献
        triplets_grad3d.emplace_back(3*t,     k, grad_phi_k.x());
        triplets_grad3d.emplace_back(3*t + 1, k, grad_phi_k.y());
        triplets_grad3d.emplace_back(3*t + 2, k, grad_phi_k.z());
        
        // 计算局部坐标系
        // 第一条边方向作为局部x轴
        Eigen::Vector3d lx = e1.normalized();
        
        // 在三角形平面内与lx垂直的方向作为局部y轴
        Eigen::Vector3d ly = cross(n_unit, lx).normalized();
        
        // 添加投影矩阵P的三元组
        // 投影矩阵P的大小为 (2*nf) x (3*nf)
        // 对于三角形t，投影矩阵的块 P_t = [lx^T; ly^T] ∈ R^{2x3}
        
        int row_base = 2 * t;  // 当前三角形在P中的起始行索引
        int col_base = 3 * t;  // 当前三角形在P中的起始列索引
        
        // 第一行: lx
        triplets_proj.emplace_back(row_base,     col_base,     lx.x());
        triplets_proj.emplace_back(row_base,     col_base + 1, lx.y());
        triplets_proj.emplace_back(row_base,     col_base + 2, lx.z());
        
        // 第二行: ly
        triplets_proj.emplace_back(row_base + 1, col_base,     ly.x());
        triplets_proj.emplace_back(row_base + 1, col_base + 1, ly.y());
        triplets_proj.emplace_back(row_base + 1, col_base + 2, ly.z());
    }
    
    // 构建稀疏矩阵
    result.grad_3d = Eigen::SparseMatrix<double>(3 * nf, nv);
    result.grad_3d.setFromTriplets(triplets_grad3d.begin(), triplets_grad3d.end());
    
    // 构建投影矩阵P
    Eigen::SparseMatrix<double> P(2 * nf, 3 * nf);
    P.setFromTriplets(triplets_proj.begin(), triplets_proj.end());
    
    // 计算grad_2d = P * grad_3d
    result.grad_2d = P * result.grad_3d;
    
    qDebug() << "Deformation gradient operator computed:";
    qDebug() << "  grad_3d: " << result.grad_3d.rows() << "x" << result.grad_3d.cols();
    qDebug() << "  grad_2d: " << result.grad_2d.rows() << "x" << result.grad_2d.cols();
    qDebug() << "  areas: " << result.areas.size();
    
    return result;
}

// 新增函数：构建变形梯度矩阵D
Eigen::SparseMatrix<double> SimpleSquareWidget::buildDeformationMatrix(
    const DeformationGradientOperator& op) const {
    
    int nf = op.areas.size();
    int nv = op.grad_2d.cols();
    
    // 提取grad_2d的奇数行(D1)和偶数行(D2)
    Eigen::SparseMatrix<double> D1(nf, nv);
    Eigen::SparseMatrix<double> D2(nf, nv);
    
    // 为D1和D2预分配空间
    std::vector<Eigen::Triplet<double>> triplets_D1, triplets_D2;
    
    // grad_2d的大小是(2*nf) x nv
    // 奇数行(0,2,4,...)对应∂/∂x分量 -> D1
    // 偶数行(1,3,5,...)对应∂/∂y分量 -> D2
    
    for (int k = 0; k < op.grad_2d.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(op.grad_2d, k); it; ++it) {
            int row = it.row();
            int col = it.col();
            double value = it.value();
            
            if (row % 2 == 0) {  // 偶数行索引(0,2,4,...)，对应∂/∂x
                triplets_D1.emplace_back(row / 2, col, value);
            } else {  // 奇数行索引(1,3,5,...)，对应∂/∂y
                triplets_D2.emplace_back(row / 2, col, value);
            }
        }
    }
    
    D1.setFromTriplets(triplets_D1.begin(), triplets_D1.end());
    D2.setFromTriplets(triplets_D2.begin(), triplets_D2.end());
    
    // 构建完整的变形梯度矩阵D
    // D = [D1  0 ; 0 D1; D2 0; 0 D2] ∈ R^{(4*nf) x (2*nv)}
    
    int D_rows = 4 * nf;
    int D_cols = 2 * nv;
    
    std::vector<Eigen::Triplet<double>> triplets_D;
    triplets_D.reserve(4 * D1.nonZeros());  // D1和D2各有两个副本
    
    // 第1块: D1作用于u
    for (int k = 0; k < D1.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(D1, k); it; ++it) {
            int row = it.row();
            int col = it.col();
            triplets_D.emplace_back(row, col, it.value());
        }
    }
    
    // 第2块: D1作用于v (偏移nv列)
    for (int k = 0; k < D1.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(D1, k); it; ++it) {
            int row = it.row();
            int col = it.col();
            triplets_D.emplace_back(row + nf, col + nv, it.value());
        }
    }
    
    // 第3块: D2作用于u (偏移2*nf行)
    for (int k = 0; k < D2.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(D2, k); it; ++it) {
            int row = it.row();
            int col = it.col();
            triplets_D.emplace_back(row + 2 * nf, col, it.value());
        }
    }
    
    // 第4块: D2作用于v (偏移2*nf行, nv列)
    for (int k = 0; k < D2.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(D2, k); it; ++it) {
            int row = it.row();
            int col = it.col();
            triplets_D.emplace_back(row + 2 * nf + nf, col + nv, it.value());
        }
    }
    
    Eigen::SparseMatrix<double> D(D_rows, D_cols);
    D.setFromTriplets(triplets_D.begin(), triplets_D.end());
    
    qDebug() << "Deformation matrix D built: " << D.rows() << "x" << D.cols();
    return D;
}

// 新增函数：使用算子方法检测翻转
int SimpleSquareWidget::checkForFlipsWithOperator(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& faces,
    const std::vector<float>& uv) const {
    
    flippedTriangles.clear();
    
    if (vertices.empty() || uv.empty() || faces.empty()) {
        return 0;
    }
    
    // 计算变形梯度算子
    DeformationGradientOperator op = computeDeformationGradientOperator(vertices, faces);
    
    int nv = vertices.size() / 3;
    int nf = faces.size() / 3;
    
    // 提取u和v坐标
    Eigen::VectorXd u_vec(nv);
    Eigen::VectorXd v_vec(nv);
    
    for (int i = 0; i < nv; i++) {
        u_vec(i) = uv[i * 3];      // u坐标
        v_vec(i) = uv[i * 3 + 1];  // v坐标
    }
    
    // 计算u和v的2D梯度
    Eigen::VectorXd grad_u_2d = op.grad_2d * u_vec;
    Eigen::VectorXd grad_v_2d = op.grad_2d * v_vec;
    
    // 计算每个三角形的Jacobian矩阵行列式
    int flipCount = 0;
    
    for (int t = 0; t < nf; t++) {
        // 提取三角形t的梯度
        double du_dx = grad_u_2d(2 * t);      // ∂u/∂x
        double du_dy = grad_u_2d(2 * t + 1);  // ∂u/∂y
        double dv_dx = grad_v_2d(2 * t);      // ∂v/∂x
        double dv_dy = grad_v_2d(2 * t + 1);  // ∂v/∂y
        
        // 计算Jacobian行列式
        double det = du_dx * dv_dy - du_dy * dv_dx;
        
        // 如果行列式为负，表示翻转
        if (det < 0.0) {
            flipCount++;
            flippedTriangles.push_back(t);
        }
    }
    
    qDebug() << "Using gradient operator: found" << flipCount << "flipped triangles out of" << nf;
    return flipCount;
}

// 修改函数：检查翻转（添加算子方法选择）
int SimpleSquareWidget::checkForFlips() const {
    flippedTriangles.clear();
    
    if (paramVertices.empty() || meshVertices.empty() || paramFaces.empty()) {
        return 0;
    }
    
    // 方法选择开关：true使用算子方法，false使用原方法
    bool useOperatorMethod = false;
    
    if (useOperatorMethod) {
        // 使用算子方法
        return checkForFlipsWithOperator(meshVertices, meshFaces, paramVertices);
    } else {
        // 使用原方法（有向面积方法）
        int numFaces = paramFaces.size() / 3;
        int flipCount = 0;
        
        const float EPSILON = 1e-6f;
        
        for (int i = 0; i < numFaces; i++) {
            unsigned int idx1 = paramFaces[i * 3];
            unsigned int idx2 = paramFaces[i * 3 + 1];
            unsigned int idx3 = paramFaces[i * 3 + 2];
            
            // 获取参数域顶点（二维）
            float u1 = paramVertices[idx1 * 3];
            float v1 = paramVertices[idx1 * 3 + 1];
            float u2 = paramVertices[idx2 * 3];
            float v2 = paramVertices[idx2 * 3 + 1];
            float u3 = paramVertices[idx3 * 3];
            float v3 = paramVertices[idx3 * 3 + 1];
            
            // 获取原始三维顶点
            float x1 = meshVertices[idx1 * 3];
            float y1 = meshVertices[idx1 * 3 + 1];
            float z1 = meshVertices[idx1 * 3 + 2];
            float x2 = meshVertices[idx2 * 3];
            float y2 = meshVertices[idx2 * 3 + 1];
            float z2 = meshVertices[idx2 * 3 + 2];
            float x3 = meshVertices[idx3 * 3];
            float y3 = meshVertices[idx3 * 3 + 1];
            float z3 = meshVertices[idx3 * 3 + 2];
            
            // 方法1：比较参数域有向面积和三维投影有向面积
            float area2D = (u2 - u1) * (v3 - v1) - (u3 - u1) * (v2 - v1);
            float area3D_xy = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
            
            // 如果符号不同，表示翻转
            if (area2D * area3D_xy < 0.0f) {
                flipCount++;
                flippedTriangles.push_back(i);
            }
        }
        
        qDebug() << "Using area method: found" << flipCount << "flipped triangles out of" << numFaces;
        return flipCount;
    }
}

void SimpleSquareWidget::solveParameterization() {
    if (currentParamMethod == OriginalMethod) {
        solveParameterizationOriginal();
    } else if (currentParamMethod == UniformTutte || currentParamMethod == WeightedTutte) {
        solveParameterizationInternal(currentParamMethod);
    } else if (currentParamMethod == FloaterShapePreserving) {
        solveParameterizationFloater();
    }
}

void SimpleSquareWidget::performParameterization(BoundaryType boundaryType, ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;

    original = openMesh;
    
    if (boundaryType == Circle) {
        mapBoundaryToCircle();
    } else {
        mapBoundaryToRectangle();
    }
    
    currentParamMethod = method;
    solveParameterization();
    normalizeMesh();
    
    paramVertices.clear();
    paramFaces = faces;
    
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        paramVertices.push_back(p[0]);
        paramVertices.push_back(p[1]);
        paramVertices.push_back(0.0f);
    }
    
    openMesh = original;
    parameterized = true;
    
    // 清空之前的翻转检测结果
    flippedTriangles.clear();
    
    qDebug() << "Parameterization completed using method:" << method 
             << "Vertices:" << paramVertices.size() / 3 << "Faces:" << paramFaces.size() / 3;
}

void SimpleSquareWidget::outputDebugFiles() const {
    if (paramVertices.empty() || paramFaces.empty()) {
        qDebug() << "No parameterized data to output";
        return;
    }
    
    int numFaces = paramFaces.size() / 3;
    qDebug() << "Outputting debug files for" << numFaces << "faces";
    
    // 输出原始模型坐标到 debug1.txt
    std::ofstream debug1("debug1.txt");
    if (!debug1.is_open()) {
        qDebug() << "Failed to open debug1.txt";
        return;
    }
    
    debug1 << "Original Model Coordinates (3D)" << std::endl;
    debug1 << "========================================" << std::endl;
    
    // 从原始网格获取坐标
    if (original.n_vertices() > 0) {
        for (int i = 0; i < numFaces; i++) {
            unsigned int idx1 = paramFaces[i * 3];
            unsigned int idx2 = paramFaces[i * 3 + 1];
            unsigned int idx3 = paramFaces[i * 3 + 2];
            
            auto vh1 = original.vertex_handle(idx1);
            auto vh2 = original.vertex_handle(idx2);
            auto vh3 = original.vertex_handle(idx3);
            
            auto p1 = original.point(vh1);
            auto p2 = original.point(vh2);
            auto p3 = original.point(vh3);
            
            debug1 << "Face " << i << ":" << std::endl;
            debug1 << "  Vertex " << idx1 << ": (" << p1[0] << ", " << p1[1] << ", " << p1[2] << ")" << std::endl;
            debug1 << "  Vertex " << idx2 << ": (" << p2[0] << ", " << p2[1] << ", " << p2[2] << ")" << std::endl;
            debug1 << "  Vertex " << idx3 << ": (" << p3[0] << ", " << p3[1] << ", " << p3[2] << ")" << std::endl;
            
            // 计算原始三角形的面积（在XY平面投影）
            float area_original = 0.5f * fabs(
                (p2[0] - p1[0]) * (p3[1] - p1[1]) - 
                (p3[0] - p1[0]) * (p2[1] - p1[1])
            );
            debug1 << "  Projected Area (XY plane): " << area_original << std::endl;
            debug1 << std::endl;
        }
    } else {
        debug1 << "Original mesh not available" << std::endl;
    }
    
    debug1.close();
    qDebug() << "Original coordinates written to debug1.txt";
    
    // 输出参数化坐标到 debug2.txt
    std::ofstream debug2("debug2.txt");
    if (!debug2.is_open()) {
        qDebug() << "Failed to open debug2.txt";
        return;
    }
    
    debug2 << "Parameterized Coordinates (2D)" << std::endl;
    debug2 << "========================================" << std::endl;
    
    for (int i = 0; i < numFaces; i++) {
        unsigned int idx1 = paramFaces[i * 3];
        unsigned int idx2 = paramFaces[i * 3 + 1];
        unsigned int idx3 = paramFaces[i * 3 + 2];
        
        float x1 = paramVertices[idx1 * 3];
        float y1 = paramVertices[idx1 * 3 + 1];
        float x2 = paramVertices[idx2 * 3];
        float y2 = paramVertices[idx2 * 3 + 1];
        float x3 = paramVertices[idx3 * 3];
        float y3 = paramVertices[idx3 * 3 + 1];
        
        debug2 << "Face " << i << ":" << std::endl;
        debug2 << "  Vertex " << idx1 << ": (" << x1 << ", " << y1 << ", 0.0)" << std::endl;
        debug2 << "  Vertex " << idx2 << ": (" << x2 << ", " << y2 << ", 0.0)" << std::endl;
        debug2 << "  Vertex " << idx3 << ": (" << x3 << ", " << y3 << ", 0.0)" << std::endl;
        
        // 计算参数化后的三角形面积
        float area_param = 0.5f * fabs(
            (x2 - x1) * (y3 - y1) - 
            (x3 - x1) * (y2 - y1)
        );
        debug2 << "  Area: " << area_param << std::endl;
        debug2 << std::endl;
    }
    
    debug2.close();
    qDebug() << "Parameterized coordinates written to debug2.txt";
    
    // 输出计算结果到 debug3.txt
    std::ofstream debug3("debug3.txt");
    if (!debug3.is_open()) {
        qDebug() << "Failed to open debug3.txt";
        return;
    }
    
    debug3 << "Calculation Results (Jacobian Determinants)" << std::endl;
    debug3 << "========================================" << std::endl;
    
    int flipCount = 0;
    const float EPSILON = 1e-6f;
    
    for (int i = 0; i < numFaces; i++) {
        unsigned int idx1 = paramFaces[i * 3];
        unsigned int idx2 = paramFaces[i * 3 + 1];
        unsigned int idx3 = paramFaces[i * 3 + 2];
        
        // 参数化坐标
        float x1 = paramVertices[idx1 * 3];
        float y1 = paramVertices[idx1 * 3 + 1];
        float x2 = paramVertices[idx2 * 3];
        float y2 = paramVertices[idx2 * 3 + 1];
        float x3 = paramVertices[idx3 * 3];
        float y3 = paramVertices[idx3 * 3 + 1];
        
        // 计算Jacobian矩阵行列式（与checkForFlips一致）
        // det = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)
        float det = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
        
        // 计算面积（绝对值的0.5倍）
        float area = 0.5f * fabs(det);
        
        debug3 << "Face " << i << ":" << std::endl;
        debug3 << "  Vertex indices: " << idx1 << ", " << idx2 << ", " << idx3 << std::endl;
        debug3 << "  Jacobian determinant: " << det << std::endl;
        debug3 << "  Area: " << area << std::endl;
        debug3 << "  Status: ";
        
        if (fabs(det) < EPSILON) {
            debug3 << "DEGENERATE (|det| < " << EPSILON << ")" << std::endl;
        } else if (det < 0.0f) {
            debug3 << "FLIPPED (det < 0)" << std::endl;
            flipCount++;
        } else {
            debug3 << "VALID (det > 0)" << std::endl;
        }
        
        // 输出三角形顶点的详细坐标
        debug3 << "  Coordinates:" << std::endl;
        debug3 << "    v" << idx1 << ": (" << x1 << ", " << y1 << ")" << std::endl;
        debug3 << "    v" << idx2 << ": (" << x2 << ", " << y2 << ")" << std::endl;
        debug3 << "    v" << idx3 << ": (" << x3 << ", " << y3 << ")" << std::endl;
        
        debug3 << std::endl;
    }
    
    debug3 << "Summary:" << std::endl;
    debug3 << "  Total faces: " << numFaces << std::endl;
    debug3 << "  Flipped faces: " << flipCount << std::endl;
    debug3 << "  Valid faces: " << numFaces - flipCount << std::endl;
    debug3 << "  Flip percentage: " << (flipCount * 100.0f / numFaces) << "%" << std::endl;
    
    debug3.close();
    qDebug() << "Calculation results written to debug3.txt";
    qDebug() << "Flip count: " << flipCount << "/" << numFaces;
}