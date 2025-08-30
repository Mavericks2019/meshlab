// uvparamwidget.cpp
#include "uvparamwidget.h"
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <queue>

UVParamWidget::UVParamWidget(QWidget *parent) : QOpenGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    uvVbo(QOpenGLBuffer::VertexBuffer),
    lineVbo(QOpenGLBuffer::VertexBuffer),
    lineEbo(QOpenGLBuffer::IndexBuffer),
    faceVbo(QOpenGLBuffer::VertexBuffer),
    faceColorVbo(QOpenGLBuffer::VertexBuffer),
    hasUV(false),
    squareSize(1.0f),
    showPoints(false),  // 改为false，默认不显示点
    showWireframe(false), // 改为false，默认不显示线
    showFaces(true),    // 保持true，默认显示面
    lineVertexCount(0),
    faceVertexCount(0),
    useAntialiasing(true)  // 默认启用抗锯齿
{
    setFocusPolicy(Qt::StrongFocus);
    
    // 设置抗锯齿格式
    QSurfaceFormat format;
    format.setSamples(4);  // 4倍多重采样
    setFormat(format);
    
    // Set colors - 修改正方形颜色为纯白色
    squareColor = QColor(255, 255, 255, 255);  // 纯白色
    pointColor = QColor(255, 0, 0);            // Red points
    lineColor = QColor(0, 0, 255);             // Blue lines
    
    // 初始化颜色调色板 - 将颜色加深约20%
    colorPalette = {
        QVector3D(0.8f, 0.56f, 0.56f),   // 加深的浅红色
        QVector3D(0.56f, 0.56f, 0.8f),   // 加深的浅蓝色
        QVector3D(0.56f, 0.8f, 0.56f),   // 加深的浅绿色
        QVector3D(0.64f, 0.56f, 0.8f),   // 加深的浅紫色
        QVector3D(0.8f, 0.8f, 0.56f),    // 加深的浅黄色
        QVector3D(0.8f, 0.64f, 0.48f)    // 加深的浅橙色
    };
}

UVParamWidget::~UVParamWidget() {
    makeCurrent();
    squareVao.destroy();
    squareVbo.destroy();
    squareEbo.destroy();
    uvVao.destroy();
    uvVbo.destroy();
    lineVao.destroy();
    lineVbo.destroy();
    lineEbo.destroy();
    faceVao.destroy();
    faceVbo.destroy();
    faceColorVbo.destroy();
    doneCurrent();
}

// 添加打印mesh信息的函数
void UVParamWidget::printMeshInfo() const {
    qDebug() << "Mesh Information:";
    qDebug() << "Vertex count:" << mesh.n_vertices();
    qDebug() << "Face count:" << mesh.n_faces();
    qDebug() << "Edge count:" << mesh.n_edges();
    
    // 检查Mesh中是否有纹理坐标属性
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    bool hasTextureInMesh = mesh.get_property_handle(mvt_list, "mvt_list") && 
                           mesh.get_property_handle(hvt_index, "hvt_index");

    if (hasTextureInMesh) {
        qDebug() << "Texture data found in mesh";
        const auto& meshTexCoords = mesh.property(mvt_list);
        qDebug() << "Texture coordinate count:" << meshTexCoords.size();
        
        if (!meshTexCoords.empty()) {
            qDebug() << "First 10 texture coordinates:";
            int count = 0;
            for (const auto& texCoord : meshTexCoords) {
                if (count >= 10) break;
                qDebug() << "TexCoord" << count << ": (" << texCoord[0] << "," << texCoord[1] << ")";
                count++;
            }
        }
    } else {
        qDebug() << "No texture data found in mesh";
    }
}

void UVParamWidget::initializeGL() {
    initializeOpenGLFunctions();
    // 修改背景颜色为与BaseGLWidget一致的蓝色
    glClearColor(0.0f, 85.0f/255.0f, 127.0f/255.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 启用多重采样抗锯齿
    if (useAntialiasing) {
        glEnable(GL_MULTISAMPLE);
    }

    // 初始化抗锯齿线程序
    antialiasedLineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "}\n");
    
    antialiasedLineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 lineColor;\n"
        "void main() {\n"
        "   FragColor = vec4(lineColor, 1.0);\n"
        "}\n");
    antialiasedLineProgram.link();
    
    // 初始化抗锯齿点程序
    antialiasedPointProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   gl_PointSize = 6.0;\n"  // 稍微大一点的点以便抗锯齿效果更明显
        "}\n");
    
    antialiasedPointProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 pointColor;\n"
        "void main() {\n"
        "   // 创建圆形点而不是方点\n"
        "   vec2 coord = gl_PointCoord - vec2(0.5);\n"
        "   if (length(coord) > 0.5)\n"
        "       discard;\n"
        "   FragColor = vec4(pointColor, 1.0);\n"
        "}\n");
    antialiasedPointProgram.link();

    // Initialize square
    squareVao.create();
    squareVbo.create();
    squareEbo.create();
    
    setupSquare();

    // Initialize UV points
    uvVao.create();
    uvVbo.create();
    
    uvProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   gl_PointSize = 5.0;\n"
        "}\n");
    
    uvProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 pointColor;\n"
        "void main() {\n"
        "   FragColor = vec4(pointColor, 1.0);\n"
        "}\n");
    uvProgram.link();

    // Initialize lines
    lineVao.create();
    lineVbo.create();
    lineEbo.create();
    
    lineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "}\n");
    
    lineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 lineColor;\n"
        "void main() {\n"
        "   FragColor = vec4(lineColor, 1.0);\n"
        "}\n");
    lineProgram.link();
    
    // Initialize faces
    faceVao.create();
    faceVbo.create();
    faceColorVbo.create();
    
    faceProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "out vec3 fragColor;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   fragColor = aColor;\n"
        "}\n");
    
    faceProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "in vec3 fragColor;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(fragColor, 0.7);\n"  // 半透明
        "}\n");
    faceProgram.link();
}

void UVParamWidget::setupSquare() {
    float vertices[] = {
        -squareSize, -squareSize, 0.0f,
         squareSize, -squareSize, 0.0f,
         squareSize,  squareSize, 0.0f,
        -squareSize,  squareSize, 0.0f
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
    squareProgram.enableAttributeArray(posLoc);
    squareProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    
    squareVao.release();
}

void UVParamWidget::analyzeTopology() {
    if (mesh.n_faces() == 0) return;
    
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    if (!mesh.get_property_handle(mvt_list, "mvt_list") || 
        !mesh.get_property_handle(hvt_index, "hvt_index")) {
        return;
    }
    
    const auto& texCoords = mesh.property(mvt_list);
    
    // 构建邻接表 - 纹理坐标索引到面的映射
    std::unordered_map<int, std::vector<int>> texIndexToFaces;
    for (int i = 0; i < mesh.n_faces(); i++) {
        Mesh::FaceHandle fh = mesh.face_handle(i);
        for (Mesh::ConstFaceHalfedgeIter fh_it = mesh.cfh_begin(fh); fh_it.is_valid(); ++fh_it) {
            int texIndex = mesh.property(hvt_index, *fh_it);
            texIndexToFaces[texIndex].push_back(i);
        }
    }
    
    // 使用BFS找到连通的纹理面组
    std::vector<bool> visited(mesh.n_faces(), false);
    faceColors.resize(mesh.n_faces(), -1);
    int currentColorIndex = 0;
    
    for (int i = 0; i < mesh.n_faces(); i++) {
        if (visited[i]) continue;
        
        std::queue<int> q;
        q.push(i);
        visited[i] = true;
        faceColors[i] = currentColorIndex;
        
        while (!q.empty()) {
            int faceIdx = q.front();
            q.pop();
            
            Mesh::FaceHandle fh = mesh.face_handle(faceIdx);
            
            // 遍历面的所有纹理坐标索引
            for (Mesh::ConstFaceHalfedgeIter fh_it = mesh.cfh_begin(fh); fh_it.is_valid(); ++fh_it) {
                int texIndex = mesh.property(hvt_index, *fh_it);
                
                // 找到共享相同纹理坐标索引的所有面
                for (int neighborFaceIdx : texIndexToFaces[texIndex]) {
                    if (!visited[neighborFaceIdx]) {
                        visited[neighborFaceIdx] = true;
                        faceColors[neighborFaceIdx] = currentColorIndex;
                        q.push(neighborFaceIdx);
                    }
                }
            }
        }
        
        currentColorIndex = (currentColorIndex + 1) % colorPalette.size();
    }
}

void UVParamWidget::setupFaces() {
    if (mesh.n_faces() == 0) return;
    
    // 分析拓扑结构
    analyzeTopology();
    
    // 准备顶点数据和颜色数据
    std::vector<QVector2D> faceVertices;
    std::vector<QVector3D> faceVertexColors;
    
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    if (!mesh.get_property_handle(mvt_list, "mvt_list") || 
        !mesh.get_property_handle(hvt_index, "hvt_index")) {
        return;
    }
    
    const auto& texCoords = mesh.property(mvt_list);
    
    for (int i = 0; i < mesh.n_faces(); i++) {
        Mesh::FaceHandle fh = mesh.face_handle(i);
        
        // 获取面的颜色
        int colorIdx = faceColors[i] % colorPalette.size();
        QVector3D faceColor = colorPalette[colorIdx];
        
        // 为面的每个顶点添加相同的颜色
        for (Mesh::ConstFaceHalfedgeIter fh_it = mesh.cfh_begin(fh); fh_it.is_valid(); ++fh_it) {
            int texIndex = mesh.property(hvt_index, *fh_it);
            if (texIndex < texCoords.size()) {
                // 将UV从[0,1]映射到[-1,1]
                float x = texCoords[texIndex][0] * 2.0f - 1.0f;
                float y = texCoords[texIndex][1] * 2.0f - 1.0f;
                faceVertices.emplace_back(x, y);
                faceVertexColors.push_back(faceColor);
            }
        }
    }
    
    faceVertexCount = faceVertices.size();
    
    // 设置面VAO和VBO
    faceVao.bind();
    
    // 顶点位置
    faceVbo.bind();
    faceVbo.allocate(faceVertices.data(), faceVertices.size() * sizeof(QVector2D));
    
    faceProgram.bind();
    int posLoc = faceProgram.attributeLocation("aPos");
    faceProgram.enableAttributeArray(posLoc);
    faceProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    
    // 顶点颜色
    faceColorVbo.bind();
    faceColorVbo.allocate(faceVertexColors.data(), faceVertexColors.size() * sizeof(QVector3D));
    
    int colorLoc = faceProgram.attributeLocation("aColor");
    faceProgram.enableAttributeArray(colorLoc);
    faceProgram.setAttributeBuffer(colorLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    
    faceVao.release();
}

void UVParamWidget::setupUVPoints() {
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    if (!mesh.get_property_handle(mvt_list, "mvt_list") || 
        !mesh.get_property_handle(hvt_index, "hvt_index")) {
        hasUV = false;
        return;
    }
    
    const auto& texCoords = mesh.property(mvt_list);
    if (texCoords.empty()) {
        hasUV = false;
        return;
    }
    
    hasUV = true;
    
    // 转换UV坐标到[-1,1]范围
    std::vector<QVector2D> transformedUV;
    for (const auto& texCoord : texCoords) {
        // 将UV从[0,1]映射到[-1,1]
        float x = texCoord[0] * 2.0f - 1.0f;
        float y = texCoord[1] * 2.0f - 1.0f;
        transformedUV.emplace_back(x, y);
    }
    
    // Setup points
    uvVao.bind();
    uvVbo.bind();
    uvVbo.allocate(transformedUV.data(), transformedUV.size() * sizeof(QVector2D));
    
    uvProgram.bind();
    int posLoc = uvProgram.attributeLocation("aPos");
    uvProgram.enableAttributeArray(posLoc);
    uvProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    
    uvVao.release();
    
    // Setup lines (connect points based on face indices)
    std::vector<QVector2D> lineVertices;
    for (int i = 0; i < mesh.n_faces(); i++) {
        Mesh::FaceHandle fh = mesh.face_handle(i);
        std::vector<int> texIndices;
        
        // 收集面的纹理索引
        for (Mesh::ConstFaceHalfedgeIter fh_it = mesh.cfh_begin(fh); fh_it.is_valid(); ++fh_it) {
            texIndices.push_back(mesh.property(hvt_index, *fh_it));
        }
        
        // 创建线
        for (int j = 0; j < texIndices.size(); j++) {
            int idx1 = texIndices[j];
            int idx2 = texIndices[(j + 1) % texIndices.size()];
            if (idx1 < transformedUV.size() && idx2 < transformedUV.size()) {
                lineVertices.push_back(transformedUV[idx1]);
                lineVertices.push_back(transformedUV[idx2]);
            }
        }
    }
    
    lineVao.bind();
    lineVbo.bind();
    lineVbo.allocate(lineVertices.data(), lineVertices.size() * sizeof(QVector2D));
    
    lineProgram.bind();
    posLoc = lineProgram.attributeLocation("aPos");
    lineProgram.enableAttributeArray(posLoc);
    lineProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    
    lineVao.release();
    
    // Store the number of line vertices for drawing
    lineVertexCount = lineVertices.size();
    
    // Setup faces
    setupFaces();
}

void UVParamWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    
    // 创建保持宽高比的投影矩阵
    projection.setToIdentity();
    float aspect = static_cast<float>(w) / h;
    if (aspect > 1.0f) {
        projection.ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -1.0f, 1.0f);
    }
}

void UVParamWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw square
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
    
    if (hasUV) {
        // Draw faces if enabled
        if (showFaces && faceVertexCount > 0) {
            faceProgram.bind();
            faceVao.bind();
            
            faceProgram.setUniformValue("projection", projection);
            
            glDrawArrays(GL_TRIANGLES, 0, faceVertexCount);
            
            faceVao.release();
            faceProgram.release();
        }
        
        // Draw wireframe if enabled - 使用抗锯齿
        if (showWireframe && lineVertexCount > 0) {
            if (useAntialiasing) {
                // 启用线抗锯齿
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                
                antialiasedLineProgram.bind();
                lineVao.bind();
                
                antialiasedLineProgram.setUniformValue("projection", projection);
                antialiasedLineProgram.setUniformValue("lineColor", 
                                           QVector3D(lineColor.redF(), lineColor.greenF(), lineColor.blueF()));
                
                glDrawArrays(GL_LINES, 0, lineVertexCount);
                
                lineVao.release();
                antialiasedLineProgram.release();
                
                glDisable(GL_LINE_SMOOTH);
            } else {
                lineProgram.bind();
                lineVao.bind();
                
                lineProgram.setUniformValue("projection", projection);
                lineProgram.setUniformValue("lineColor", 
                                           QVector3D(lineColor.redF(), lineColor.greenF(), lineColor.blueF()));
                
                glDrawArrays(GL_LINES, 0, lineVertexCount);
                
                lineVao.release();
                lineProgram.release();
            }
        }
        
        // Draw points if enabled - 使用抗锯齿
        if (showPoints && hasUV) {
            OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
            if (mesh.get_property_handle(mvt_list, "mvt_list")) {
                const auto& texCoords = mesh.property(mvt_list);
                if (!texCoords.empty()) {
                    if (useAntialiasing) {
                        antialiasedPointProgram.bind();
                        uvVao.bind();
                        
                        antialiasedPointProgram.setUniformValue("projection", projection);
                        antialiasedPointProgram.setUniformValue("pointColor", 
                                             QVector3D(pointColor.redF(), pointColor.greenF(), pointColor.blueF()));
                        
                        glDrawArrays(GL_POINTS, 0, texCoords.size());
                        
                        uvVao.release();
                        antialiasedPointProgram.release();
                    } else {
                        uvProgram.bind();
                        uvVao.bind();
                        
                        uvProgram.setUniformValue("projection", projection);
                        uvProgram.setUniformValue("pointColor", 
                                                 QVector3D(pointColor.redF(), pointColor.greenF(), pointColor.blueF()));
                        
                        glDrawArrays(GL_POINTS, 0, texCoords.size());
                        
                        uvVao.release();
                        uvProgram.release();
                    }
                }
            }
        }
    }
}

void UVParamWidget::parseOBJ(const QString &path) {
    clearData();

    // 使用Mesh_doubleIO加载mesh
    std::string stdPath = path.toStdString();
    bool success = Mesh_doubleIO::load_mesh(mesh, stdPath.c_str(), true);
    
    if (!success) {
        qWarning() << "Failed to load mesh using Mesh_doubleIO:" << path;
        return;
    }
    
    // 检查是否有纹理数据
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    hasUV = mesh.get_property_handle(mvt_list, "mvt_list") && 
            mesh.get_property_handle(hvt_index, "hvt_index") &&
            !mesh.property(mvt_list).empty();
    
    if (hasUV) {
        makeCurrent();
        setupUVPoints();
        doneCurrent();
    }
    
    update();
}

void UVParamWidget::loadOBJ(const QString &path) {
    parseOBJ(path);
}

void UVParamWidget::clearData() {
    hasUV = false;
    faceColors.clear();
    lineVertexCount = 0;
    faceVertexCount = 0;
    
    // 完全清理mesh
    mesh.clear();
    mesh.garbage_collection(); // 确保所有资源被释放
    
    // 清理OpenMesh的属性
    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;
    
    if (mesh.get_property_handle(mvt_list, "mvt_list")) {
        mesh.remove_property(mvt_list);
    }
    if (mesh.get_property_handle(hvt_index, "hvt_index")) {
        mesh.remove_property(hvt_index);
    }
    
    makeCurrent();
    
    // 重置所有缓冲区
    uvVbo.bind();
    uvVbo.allocate(0, 0);
    uvVbo.release();
    
    lineVbo.bind();
    lineVbo.allocate(0, 0);
    lineVbo.release();
    
    faceVbo.bind();
    faceVbo.allocate(0, 0);
    faceVbo.release();
    
    faceColorVbo.bind();
    faceColorVbo.allocate(0, 0);
    faceColorVbo.release();
    
    doneCurrent();
    
    update();
}

void UVParamWidget::setShowPoints(bool show) {
    showPoints = show;
    update();
}

void UVParamWidget::setShowWireframe(bool show) {
    showWireframe = show;
    update();
}

void UVParamWidget::setShowFaces(bool show) {
    showFaces = show;
    update();
}

void UVParamWidget::setAntialiasing(bool enabled) {
    if (useAntialiasing != enabled) {
        useAntialiasing = enabled;
        makeCurrent();
        if (useAntialiasing) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
        doneCurrent();
        update();
    }
}