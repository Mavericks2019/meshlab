#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <OpenMesh/Core/IO/MeshIO.hh>

void GLWidget::clearMeshData() {
    openMesh.clear();
    faces.clear();
    edges.clear();
    modelLoaded = false;
}

bool GLWidget::loadOBJToOpenMesh(const QString &path) {
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    return OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt);
}

void GLWidget::computeBoundingBox(Mesh::Point& min, Mesh::Point& max) {
    if (openMesh.n_vertices() > 0) {
        min = max = openMesh.point(*openMesh.vertices_begin());
        for (auto vh : openMesh.vertices()) {
            min.minimize(openMesh.point(vh));
            max.maximize(openMesh.point(vh));
        }
    }
}

void GLWidget::centerAndScaleMesh(const Mesh::Point& center, float maxSize) {
    // 归一化处理：将模型缩放到单位球内
    float scaleFactor = 2.0f / maxSize;
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        p = (p - center) * scaleFactor;
        openMesh.set_point(vh, p);
    }
}

void GLWidget::prepareFaceIndices() {
    for (auto fh : openMesh.faces()) {
        auto fv_it = openMesh.fv_ccwbegin(fh);
        auto fv_end = openMesh.fv_ccwend(fh);
        int vertexCount = openMesh.valence(fh);
        
        if (vertexCount < 3) continue;
        
        if (vertexCount == 3) {
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx());
        } else {
            unsigned int centerIdx = (*fv_it).idx();
            ++fv_it;
            unsigned int prevIdx = (*fv_it).idx();
            ++fv_it;
            
            for (int i = 2; i < vertexCount; i++) {
                unsigned int currentIdx = (*fv_it).idx();
                faces.push_back(centerIdx);
                faces.push_back(prevIdx);
                faces.push_back(currentIdx);
                prevIdx = currentIdx;
                ++fv_it;
            }
        }
    }
}

void GLWidget::prepareEdgeIndices() {
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    for (auto heh : openMesh.halfedges()) {
        if (openMesh.is_boundary(heh) || heh.idx() < openMesh.opposite_halfedge_handle(heh).idx()) {
            unsigned int from = openMesh.from_vertex_handle(heh).idx();
            unsigned int to = openMesh.to_vertex_handle(heh).idx();
            
            if (from > to) std::swap(from, to);
            uniqueEdges.insert({from, to});
        }
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
}

void GLWidget::saveOriginalMesh() {
    originalMesh = openMesh;
    hasOriginalMesh = true;
}

void GLWidget::loadOBJ(const QString &path) {
    clearMeshData();
    if (!loadOBJToOpenMesh(path)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    Mesh::Point min, max;
    computeBoundingBox(min, max);
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 归一化处理：将模型缩放到单位球内
    centerAndScaleMesh(center, maxSize);
    
    // 设置模型中心和视图距离
    modelCenter = QVector3D(0, 0, 0); // 归一化后模型中心在原点
    viewDistance = 3.0f; // 基于模型大小的距离
    
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    openMesh.update_normals();
    
    prepareFaceIndices();
    prepareEdgeIndices();
    
    calculateCurvatures();
    saveOriginalMesh();
    modelLoaded = true;
    
    makeCurrent();
    initializeShaders();
    doneCurrent();
    
    rotationX = rotationY = 0;
    zoom = 1.0f;
    update();
}