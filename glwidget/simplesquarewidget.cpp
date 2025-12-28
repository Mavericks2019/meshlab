// simplesquarewidget.cpp
#include "simplesquarewidget.h"
#include <QDebug>
#include <set>
#include <QFile>
#include <fstream>
#include <iostream>
#include <iomanip>

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
// 修改 checkForFlipsWithOperator 函数，将调试信息输出到文件
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
    
    // 打开调试文件
    std::ofstream detFile("all_triangles_det.txt");
    std::ofstream gradFile("all_gradients_debug.txt");
    std::ofstream gradUFile("grad_u_2d_all.txt");
    std::ofstream gradVFile("grad_v_2d_all.txt");
    std::ofstream grad2dFile("grad_2d_flipped_triangles.txt");
    
    if (!detFile.is_open()) {
        qDebug() << "Failed to open all_triangles_det.txt";
    }
    
    if (!gradFile.is_open()) {
        qDebug() << "Failed to open all_gradients_debug.txt";
    }
    
    if (!gradUFile.is_open()) {
        qDebug() << "Failed to open grad_u_2d_all.txt";
    }
    
    if (!gradVFile.is_open()) {
        qDebug() << "Failed to open grad_v_2d_all.txt";
    }
    
    if (!grad2dFile.is_open()) {
        qDebug() << "Failed to open grad_2d_flipped_triangles.txt";
    }
    
    // 写入 grad_2d 翻转三角形的信息
    if (grad2dFile.is_open()) {
        grad2dFile << "grad_2d Matrix Information for Flipped Triangles" << std::endl;
        grad2dFile << "======================================================" << std::endl;
        grad2dFile << "Note: Each triangle has 2 rows in grad_2d (for ∂/∂x and ∂/∂y)" << std::endl;
        grad2dFile << "Matrix size: " << op.grad_2d.rows() << " x " << op.grad_2d.cols() << std::endl;
        grad2dFile << "Total non-zero elements: " << op.grad_2d.nonZeros() << std::endl;
        grad2dFile << std::endl;
    }
    
    // 写入 grad_u_2d 所有值
    if (gradUFile.is_open()) {
        gradUFile << "grad_u_2d - All Values (Size: " << grad_u_2d.size() << ")" << std::endl;
        gradUFile << "==============================================" << std::endl;
        gradUFile << "Index\tValue\tTriangle\tComponent" << std::endl;
        
        for (int i = 0; i < grad_u_2d.size(); i++) {
            int triangle_idx = i / 2;
            std::string component = (i % 2 == 0) ? "du/dx" : "du/dy";
            gradUFile << i << "\t" 
                     << std::scientific << std::setprecision(15) << grad_u_2d(i) << "\t"
                     << triangle_idx << "\t" << component << std::endl;
        }
        
        gradUFile << "\n==============================================" << std::endl;
        gradUFile << "Statistics for grad_u_2d:" << std::endl;
        gradUFile << "Min value: " << grad_u_2d.minCoeff() << std::endl;
        gradUFile << "Max value: " << grad_u_2d.maxCoeff() << std::endl;
        gradUFile << "Mean value: " << grad_u_2d.mean() << std::endl;
        gradUFile << "Norm: " << grad_u_2d.norm() << std::endl;
        gradUFile.close();
    }
    
    // 写入 grad_v_2d 所有值
    if (gradVFile.is_open()) {
        gradVFile << "grad_v_2d - All Values (Size: " << grad_v_2d.size() << ")" << std::endl;
        gradVFile << "==============================================" << std::endl;
        gradVFile << "Index\tValue\tTriangle\tComponent" << std::endl;
        
        for (int i = 0; i < grad_v_2d.size(); i++) {
            int triangle_idx = i / 2;
            std::string component = (i % 2 == 0) ? "dv/dx" : "dv/dy";
            gradVFile << i << "\t" 
                     << std::scientific << std::setprecision(15) << grad_v_2d(i) << "\t"
                     << triangle_idx << "\t" << component << std::endl;
        }
        
        gradVFile << "\n==============================================" << std::endl;
        gradVFile << "Statistics for grad_v_2d:" << std::endl;
        gradVFile << "Min value: " << grad_v_2d.minCoeff() << std::endl;
        gradVFile << "Max value: " << grad_v_2d.maxCoeff() << std::endl;
        gradVFile << "Mean value: " << grad_v_2d.mean() << std::endl;
        gradVFile << "Norm: " << grad_v_2d.norm() << std::endl;
        gradVFile.close();
    }
    
    if (detFile.is_open()) {
        detFile << "All Triangles - Jacobian Determinants" << std::endl;
        detFile << "==============================================" << std::endl;
        detFile << "Triangle Index\tJacobian Determinant\tStatus" << std::endl;
    }
    
    if (gradFile.is_open()) {
        gradFile << "All Triangles - Gradient Values" << std::endl;
        gradFile << "==========================================" << std::endl;
        gradFile << "Triangle\tdu/dx\tdu/dy\tdv/dx\tdv/dy" << std::endl;
    }
    
    // 计算每个三角形的Jacobian矩阵行列式
    int flipCount = 0;
    int degenerateCount = 0;
    int validCount = 0;
    
    for (int t = 0; t < nf; t++) {
        // 提取三角形t的梯度
        double du_dx = grad_u_2d(2 * t);      // ∂u/∂x
        double du_dy = grad_u_2d(2 * t + 1);  // ∂u/∂y
        double dv_dx = grad_v_2d(2 * t);      // ∂v/∂x
        double dv_dy = grad_v_2d(2 * t + 1);  // ∂v/∂y
        
        // 计算Jacobian行列式
        double det = du_dy * dv_dx - du_dx * dv_dy;
        
        // 写入第一个文件：三角形索引、行列式和状态
        if (detFile.is_open()) {
            std::string status;
            if (fabs(det) < 1e-12) {
                status = "DEGENERATE";
                degenerateCount++;
            } else if (det < 0.0) {
                status = "FLIPPED";
                flipCount++;
                flippedTriangles.push_back(t);
                
                // 输出翻转三角形的 grad_2d 信息
                if (grad2dFile.is_open()) {
                    outputGrad2dInfoForTriangle(op.grad_2d, t, faces, uv, grad2dFile);
                }
            } else {
                status = "VALID";
                validCount++;
            }
            
            detFile << t << "\t" 
                   << std::scientific << std::setprecision(15) << det << "\t"
                   << status << std::endl;
        }
        
        // 写入第二个文件：梯度值
        if (gradFile.is_open()) {
            gradFile << t << "\t" 
                    << std::scientific << std::setprecision(15) << du_dx << "\t"
                    << std::scientific << std::setprecision(15) << du_dy << "\t"
                    << std::scientific << std::setprecision(15) << dv_dx << "\t"
                    << std::scientific << std::setprecision(15) << dv_dy << std::endl;
        }
    }
    
    // 添加统计信息到文件
    if (detFile.is_open()) {
        detFile << "\n==============================================" << std::endl;
        detFile << "Summary:" << std::endl;
        detFile << "Total triangles: " << nf << std::endl;
        detFile << "Valid triangles: " << validCount << " (" << (validCount * 100.0 / nf) << "%)" << std::endl;
        detFile << "Flipped triangles: " << flipCount << " (" << (flipCount * 100.0 / nf) << "%)" << std::endl;
        detFile << "Degenerate triangles: " << degenerateCount << " (" << (degenerateCount * 100.0 / nf) << "%)" << std::endl;
        detFile.close();
    }
    
    if (gradFile.is_open()) {
        gradFile << "\n==========================================" << std::endl;
        gradFile << "Summary:" << std::endl;
        gradFile << "Total triangles analyzed: " << nf << std::endl;
        gradFile.close();
    }
    
    // 添加翻转三角形统计信息到 grad2dFile
    if (grad2dFile.is_open()) {
        grad2dFile << "\n======================================================" << std::endl;
        grad2dFile << "Summary of Flipped Triangles:" << std::endl;
        grad2dFile << "Total flipped triangles: " << flipCount << " out of " << nf << std::endl;
        
        if (flipCount > 0) {
            grad2dFile << "Flipped triangle indices: ";
            for (int i = 0; i < std::min(static_cast<int>(flippedTriangles.size()), 20); i++) {
                grad2dFile << flippedTriangles[i];
                if (i < std::min(static_cast<int>(flippedTriangles.size()), 20) - 1) {
                    grad2dFile << ", ";
                }
            }
            if (flippedTriangles.size() > 20) {
                grad2dFile << "... (and " << (flippedTriangles.size() - 20) << " more)";
            }
            grad2dFile << std::endl;
        }
        grad2dFile.close();
    }
    
    qDebug() << "Using gradient operator: found" << flipCount << "flipped triangles out of" << nf;
    qDebug() << "Valid triangles: " << validCount << ", Degenerate triangles: " << degenerateCount;
    qDebug() << "Debug info written to:";
    qDebug() << "  - all_triangles_det.txt";
    qDebug() << "  - all_gradients_debug.txt";
    qDebug() << "  - grad_u_2d_all.txt";
    qDebug() << "  - grad_v_2d_all.txt";
    qDebug() << "  - grad_2d_flipped_triangles.txt";
    
    // 可选：输出翻转三角形的详细信息到单独文件
    if (!flippedTriangles.empty()) {
        std::ofstream flipFile("flipped_triangles_detail.txt");
        if (flipFile.is_open()) {
            flipFile << "Detailed Information for Flipped Triangles" << std::endl;
            flipFile << "==============================================" << std::endl;
            flipFile << "Index\tFace Vertices\tJacobian Determinant\tdu/dx\tdu/dy\tdv/dx\tdv/dy" << std::endl;
            
            for (int t : flippedTriangles) {
                // 获取三角形的顶点索引
                unsigned int v1 = faces[t * 3];
                unsigned int v2 = faces[t * 3 + 1];
                unsigned int v3 = faces[t * 3 + 2];
                
                // 重新计算梯度值和行列式
                double du_dx = grad_u_2d(2 * t);
                double du_dy = grad_u_2d(2 * t + 1);
                double dv_dx = grad_v_2d(2 * t);
                double dv_dy = grad_v_2d(2 * t + 1);
                double det = du_dy * dv_dx - du_dx * dv_dy;
                
                flipFile << t << "\t" << v1 << "-" << v2 << "-" << v3 << "\t"
                        << std::scientific << std::setprecision(15) << det << "\t"
                        << std::scientific << std::setprecision(15) << du_dx << "\t"
                        << std::scientific << std::setprecision(15) << du_dy << "\t"
                        << std::scientific << std::setprecision(15) << dv_dx << "\t"
                        << std::scientific << std::setprecision(15) << dv_dy << std::endl;
            }
            flipFile.close();
            qDebug() << "Detailed flipped triangle info written to flipped_triangles_detail.txt";
        }
    }
    
    return flipCount;
}

// 修改函数：检查翻转（添加算子方法选择）
int SimpleSquareWidget::checkForFlips() const {
    flippedTriangles.clear();
    
    if (paramVertices.empty() || paramFaces.empty()) {
        return 0;
    }
    
    // 检查original是否有数据
    if (original.n_vertices() == 0) {
        qDebug() << "Warning: Original mesh is empty, using meshVertices as fallback";
        return 0; // 或者使用原来的方法
    }
    
    // 方法选择开关：true使用算子方法，false使用原方法
    bool useOperatorMethod = true;
    
    if (useOperatorMethod) {
        // 使用算子方法
        outputDebugFiles();
        
        // 从original中提取顶点数据
        std::vector<float> originalVertices;
        for (auto vh : original.vertices()) {
            auto p = original.point(vh);
            originalVertices.push_back(p[0]);
            originalVertices.push_back(p[1]);
            originalVertices.push_back(p[2]);
        }
        
        return checkForFlipsWithOperator(originalVertices, meshFaces, paramVertices);
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
            
            // 从original中获取原始三维顶点
            Mesh::VertexHandle vh1(idx1);
            Mesh::VertexHandle vh2(idx2);
            Mesh::VertexHandle vh3(idx3);
            
            auto p1 = original.point(vh1);
            auto p2 = original.point(vh2);
            auto p3 = original.point(vh3);
            
            float x1 = p1[0];
            float y1 = p1[1];
            float z1 = p1[2];
            float x2 = p2[0];
            float y2 = p2[1];
            float z2 = p2[2];
            float x3 = p3[0];
            float y3 = p3[1];
            float z3 = p3[2];
            
            // 方法1：比较参数域有向面积和三维投影有向面积
            float area2D = (u2 - u1) * (v3 - v1) - (u3 - u1) * (v2 - v1);
            float area3D_xy = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
            std::cout << area2D << "test" << area3D_xy << std::endl;
            
            // 如果符号不同，表示翻转
            if (area2D * area3D_xy < 0.0f) {
                flipCount++;
                flippedTriangles.push_back(i);
            }
        }
        
        qDebug() << "Using area method: found" << flipCount << "flipped triangles out of" << numFaces;
        outputDebugFiles();
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
    debug2 << "=======================================" << std::endl;
    
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

// 修改 outputGrad2dInfoForTriangle 函数，确保输出所有列的信息
// 修改 outputGrad2dInfoForTriangle 函数，添加详细的计算过程
void SimpleSquareWidget::outputGrad2dInfoForTriangle(
    const Eigen::SparseMatrix<double>& grad_2d, 
    int triangle_idx,
    const std::vector<unsigned int>& faces,
    const std::vector<float>& uv,
    std::ofstream& outFile) const {
    
    int nf = faces.size() / 3;
    if (triangle_idx >= nf) {
        return;
    }
    
    // 获取三角形的三个顶点
    unsigned int v1_idx = faces[triangle_idx * 3];
    unsigned int v2_idx = faces[triangle_idx * 3 + 1];
    unsigned int v3_idx = faces[triangle_idx * 3 + 2];
    
    // 获取这些顶点的u,v坐标
    float u1 = uv[v1_idx * 3];
    float v1 = uv[v1_idx * 3 + 1];
    float u2 = uv[v2_idx * 3];
    float v2 = uv[v2_idx * 3 + 1];
    float u3 = uv[v3_idx * 3];
    float v3 = uv[v3_idx * 3 + 1];
    
    outFile << std::endl;
    outFile << "Triangle " << triangle_idx << " (FLIPPED)" << std::endl;
    outFile << "======================================================" << std::endl;
    outFile << "Face vertices: " << v1_idx << ", " << v2_idx << ", " << v3_idx << std::endl;
    outFile << "UV coordinates: (" << u1 << ", " << v1 << "), (" << u2 << ", " << v2 << "), (" << u3 << ", " << v3 << ")" << std::endl;
    outFile << std::endl;
    
    // grad_2d 矩阵的行索引
    int row_x = 2 * triangle_idx;      // ∂/∂x 行
    int row_y = 2 * triangle_idx + 1;  // ∂/∂y 行
    
    // 收集参与 du/dx 计算的所有元素
    std::vector<std::tuple<int, double, float>> du_dx_elements; // (列索引, grad_2d值, u坐标)
    
    outFile << "Row " << row_x << " (∂/∂x):" << std::endl;
    outFile << "Column\tValue\tVertex\tu-coord\tContribution to du/dx" << std::endl;
    
    // 遍历 ∂/∂x 行的所有列（包括零元素）
    for (int col = 0; col < grad_2d.cols(); ++col) {
        // 检查这个位置是否有非零元素
        double value = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_x && it.col() == col) {
                value = it.value();
                break;
            }
        }
        
        if (std::abs(value) > 1e-15) {  // 只输出非零或接近非零的值
            float u_coord = uv[col * 3];
            double contribution = value * u_coord;
            du_dx_elements.push_back(std::make_tuple(col, value, u_coord));
            
            outFile << col << "\t" 
                   << std::scientific << std::setprecision(15) << value << "\t"
                   << "v" << col << "\t"
                   << std::fixed << std::setprecision(6) << u_coord << "\t"
                   << std::scientific << std::setprecision(15) << contribution << std::endl;
        }
    }
    
    // 收集参与 du/dy 计算的所有元素
    std::vector<std::tuple<int, double, float>> du_dy_elements; // (列索引, grad_2d值, u坐标)
    
    outFile << std::endl;
    outFile << "Row " << row_y << " (∂/∂y):" << std::endl;
    outFile << "Column\tValue\tVertex\tu-coord\tContribution to du/dy" << std::endl;
    
    // 遍历 ∂/∂y 行的所有列（包括零元素）
    for (int col = 0; col < grad_2d.cols(); ++col) {
        // 检查这个位置是否有非零元素
        double value = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_y && it.col() == col) {
                value = it.value();
                break;
            }
        }
        
        if (std::abs(value) > 1e-15) {  // 只输出非零或接近非零的值
            float u_coord = uv[col * 3];
            double contribution = value * u_coord;
            du_dy_elements.push_back(std::make_tuple(col, value, u_coord));
            
            outFile << col << "\t" 
                   << std::scientific << std::setprecision(15) << value << "\t"
                   << "v" << col << "\t"
                   << std::fixed << std::setprecision(6) << u_coord << "\t"
                   << std::scientific << std::setprecision(15) << contribution << std::endl;
        }
    }
    
    // 计算并显示这个三角形的 du/dx 和 du/dy
    double du_dx = 0.0;
    double du_dy = 0.0;
    
    for (int col = 0; col < grad_2d.cols(); ++col) {
        // 检查 ∂/∂x 行的值
        double value_x = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_x && it.col() == col) {
                value_x = it.value();
                break;
            }
        }
        du_dx += value_x * uv[col * 3];
        
        // 检查 ∂/∂y 行的值
        double value_y = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_y && it.col() == col) {
                value_y = it.value();
                break;
            }
        }
        du_dy += value_y * uv[col * 3];
    }
    
    // 计算 dv/dx 和 dv/dy
    double dv_dx = 0.0;
    double dv_dy = 0.0;
    
    for (int col = 0; col < grad_2d.cols(); ++col) {
        // 检查 ∂/∂x 行的值
        double value_x = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_x && it.col() == col) {
                value_x = it.value();
                break;
            }
        }
        dv_dx += value_x * uv[col * 3 + 1];
        
        // 检查 ∂/∂y 行的值
        double value_y = 0.0;
        for (Eigen::SparseMatrix<double>::InnerIterator it(grad_2d, col); it; ++it) {
            if (it.row() == row_y && it.col() == col) {
                value_y = it.value();
                break;
            }
        }
        dv_dy += value_y * uv[col * 3 + 1];
    }
    
    outFile << std::endl;
    outFile << "======================================================" << std::endl;
    outFile << "DETAILED CALCULATION PROCESS" << std::endl;
    outFile << "======================================================" << std::endl;
    
    // 详细展示 du/dx 的计算过程
    outFile << std::endl;
    outFile << "1. Calculation of du/dx:" << std::endl;
    outFile << "   du/dx = Σ (grad_2d[" << row_x << ", j] * u[j])" << std::endl;
    outFile << "         = ";
    
    double sum_du_dx = 0.0;
    for (size_t i = 0; i < du_dx_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dx_elements[i]);
        double grad_value = std::get<1>(du_dx_elements[i]);
        float u_value = std::get<2>(du_dx_elements[i]);
        double contribution = grad_value * u_value;
        sum_du_dx += contribution;
        
        outFile << "(" << std::scientific << std::setprecision(15) << grad_value 
                << " * " << std::fixed << std::setprecision(6) << u_value << ")";
        
        if (i < du_dx_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = ";
    
    for (size_t i = 0; i < du_dx_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dx_elements[i]);
        double grad_value = std::get<1>(du_dx_elements[i]);
        float u_value = std::get<2>(du_dx_elements[i]);
        double contribution = grad_value * u_value;
        
        outFile << std::scientific << std::setprecision(15) << contribution;
        
        if (i < du_dx_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = " << std::scientific << std::setprecision(15) << sum_du_dx << std::endl;
    
    // 详细展示 du/dy 的计算过程
    outFile << std::endl;
    outFile << "2. Calculation of du/dy:" << std::endl;
    outFile << "   du/dy = Σ (grad_2d[" << row_y << ", j] * u[j])" << std::endl;
    outFile << "         = ";
    
    double sum_du_dy = 0.0;
    for (size_t i = 0; i < du_dy_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dy_elements[i]);
        double grad_value = std::get<1>(du_dy_elements[i]);
        float u_value = std::get<2>(du_dy_elements[i]);
        double contribution = grad_value * u_value;
        sum_du_dy += contribution;
        
        outFile << "(" << std::scientific << std::setprecision(15) << grad_value 
                << " * " << std::fixed << std::setprecision(6) << u_value << ")";
        
        if (i < du_dy_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = ";
    
    for (size_t i = 0; i < du_dy_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dy_elements[i]);
        double grad_value = std::get<1>(du_dy_elements[i]);
        float u_value = std::get<2>(du_dy_elements[i]);
        double contribution = grad_value * u_value;
        
        outFile << std::scientific << std::setprecision(15) << contribution;
        
        if (i < du_dy_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = " << std::scientific << std::setprecision(15) << sum_du_dy << std::endl;
    
    // 计算 dv/dx 和 dv/dy
    outFile << std::endl;
    outFile << "3. Calculation of dv/dx:" << std::endl;
    outFile << "   dv/dx = Σ (grad_2d[" << row_x << ", j] * v[j])" << std::endl;
    
    // 收集参与 dv/dx 计算的所有元素
    outFile << "         = ";
    double sum_dv_dx = 0.0;
    
    for (size_t i = 0; i < du_dx_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dx_elements[i]);
        double grad_value = std::get<1>(du_dx_elements[i]);
        float v_value = uv[col_idx * 3 + 1];
        double contribution = grad_value * v_value;
        sum_dv_dx += contribution;
        
        outFile << "(" << std::scientific << std::setprecision(15) << grad_value 
                << " * " << std::fixed << std::setprecision(6) << v_value << ")";
        
        if (i < du_dx_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = " << std::scientific << std::setprecision(15) << sum_dv_dx << std::endl;
    
    outFile << std::endl;
    outFile << "4. Calculation of dv/dy:" << std::endl;
    outFile << "   dv/dy = Σ (grad_2d[" << row_y << ", j] * v[j])" << std::endl;
    outFile << "         = ";
    
    double sum_dv_dy = 0.0;
    for (size_t i = 0; i < du_dy_elements.size(); ++i) {
        int col_idx = std::get<0>(du_dy_elements[i]);
        double grad_value = std::get<1>(du_dy_elements[i]);
        float v_value = uv[col_idx * 3 + 1];
        double contribution = grad_value * v_value;
        sum_dv_dy += contribution;
        
        outFile << "(" << std::scientific << std::setprecision(15) << grad_value 
                << " * " << std::fixed << std::setprecision(6) << v_value << ")";
        
        if (i < du_dy_elements.size() - 1) {
            outFile << " + ";
        }
    }
    
    outFile << std::endl;
    outFile << "         = " << std::scientific << std::setprecision(15) << sum_dv_dy << std::endl;
    
    // 计算Jacobian行列式
    double det = sum_du_dy * sum_dv_dx - sum_du_dx * sum_dv_dy;
    
    outFile << std::endl;
    outFile << "5. Calculation of Jacobian determinant:" << std::endl;
    outFile << "   det = (du/dy)*(dv/dx) - (du/dx)*(dv/dy)" << std::endl;
    outFile << "       = (" << std::scientific << std::setprecision(15) << sum_du_dy 
            << ") * (" << std::scientific << std::setprecision(15) << sum_dv_dx 
            << ") - (" << std::scientific << std::setprecision(15) << sum_du_dx 
            << ") * (" << std::scientific << std::setprecision(15) << sum_dv_dy << ")" << std::endl;
    outFile << "       = " << std::scientific << std::setprecision(15) << (sum_du_dy * sum_dv_dx) 
            << " - " << std::scientific << std::setprecision(15) << (sum_du_dx * sum_dv_dy) << std::endl;
    outFile << "       = " << std::scientific << std::setprecision(15) << det << std::endl;
    
    // 检查计算结果的一致性
    outFile << std::endl;
    outFile << "======================================================" << std::endl;
    outFile << "CALCULATION VERIFICATION" << std::endl;
    outFile << "======================================================" << std::endl;
    outFile << "Direct calculation:" << std::endl;
    outFile << "  du/dx = " << std::scientific << std::setprecision(15) << du_dx << std::endl;
    outFile << "  du/dy = " << std::scientific << std::setprecision(15) << du_dy << std::endl;
    outFile << "  dv/dx = " << std::scientific << std::setprecision(15) << dv_dx << std::endl;
    outFile << "  dv/dy = " << std::scientific << std::setprecision(15) << dv_dy << std::endl;
    outFile << "  det = " << std::scientific << std::setprecision(15) << (du_dy * dv_dx - du_dx * dv_dy) << std::endl;
    
    outFile << std::endl;
    outFile << "Verification results:" << std::endl;
    outFile << "  du/dx matches: " << (std::abs(du_dx - sum_du_dx) < 1e-12 ? "YES" : "NO") << std::endl;
    outFile << "  du/dy matches: " << (std::abs(du_dy - sum_du_dy) < 1e-12 ? "YES" : "NO") << std::endl;
    outFile << "  dv/dx matches: " << (std::abs(dv_dx - sum_dv_dx) < 1e-12 ? "YES" : "NO") << std::endl;
    outFile << "  dv/dy matches: " << (std::abs(dv_dy - sum_dv_dy) < 1e-12 ? "YES" : "NO") << std::endl;
    
    outFile << "======================================================" << std::endl;
    outFile << std::endl;
}