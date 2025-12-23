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

int SimpleSquareWidget::checkForFlips() const {
    flippedTriangles.clear();
    
    if (paramVertices.empty() || meshVertices.empty() || paramFaces.empty()) {
        return 0;
    }
    
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
            continue; // 找到翻转，继续下一个三角形
        }else{
            qDebug() << "Triangle" << i << "is degenerate (3D area:" << area2D << ")";
        }
        
        // 方法2：可选，使用三维法向量点积（如果需要更精确）
        // 计算三维三角形的两条边
        // float ax = x2 - x1, ay = y2 - y1, az = z2 - z1;
        // float bx = x3 - x1, by = y3 - y1, bz = z3 - z1;
        
        // // 计算三维法向量（未归一化）
        // float nx = ay * bz - az * by;
        // float ny = az * bx - ax * bz;
        // float nz = ax * by - ay * bx;
        
        // // 计算面积（用于检测退化）
        // float area3D = sqrt(nx*nx + ny*ny + nz*nz);
        
        // if (area3D < EPSILON) {
        //     qDebug() << "Triangle" << i << "is degenerate (3D area:" << area3D << ")";
        // }
        // 注意：需要参考法向量来比较方向，这里假设原始网格所有三角形法向量一致
    }
    
    // 调用文件输出方法
    //outputDebugFiles();
    std::cout << flipCount << std::endl;
    return flipCount;
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