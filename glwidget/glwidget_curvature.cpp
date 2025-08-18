#include "glwidget.h"
#include <cmath>
#include <algorithm>

#define EPSILON 1E-4F 

void GLWidget::calculateCurvatures() {
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