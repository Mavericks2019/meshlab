#include "uvparamwidget_extended.h"

OpenMesh::Vec3uc boundary_edge_color = { 229, 156,  59 };
OpenMesh::Vec3uc interior_edge_color = { 100, 100, 100 };

UVParamWidgetExtended::UVParamWidgetExtended(QWidget *parent)
    : UVParamWidget(parent)
{
    // 初始化任何额外的成员变量
}

bool UVParamWidgetExtended::get_para_mesh()
{
    origin_para.clear();
    new_para.clear();

    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;

    if (!mesh.get_property_handle(mvt_list, "mvt_list") || !mesh.get_property_handle(hvt_index, "hvt_index"))
    {
        std::cout << "Texture data is invalid." << std::endl;
        return false;
    }

    for (int i = 0; i < mesh.property(mvt_list).size(); i++)
    {
        origin_para.add_vertex(Mesh::Point(mesh.property(mvt_list)[i][0], mesh.property(mvt_list)[i][1], 0.0));
        new_para.add_vertex(Mesh::Point(mesh.property(mvt_list)[i][0], mesh.property(mvt_list)[i][1], 0.0));
    }

    for (int i = 0; i < mesh.n_faces(); i++)
    {
        auto f_h = mesh.face_handle(i);
        std::vector<OpenMesh::VertexHandle> para_f;
        para_f.reserve(3);

        for (auto fh_h = mesh.cfh_begin(f_h); fh_h != mesh.cfh_end(f_h); fh_h++)
        {
            para_f.push_back(origin_para.vertex_handle(mesh.property(hvt_index, *fh_h)));
        }

        origin_para.add_face(para_f);
        new_para.add_face(para_f);
    }

    origin_h_mesh2para.resize(mesh.n_halfedges(), -1);
    new_mesh.add_property(h_mesh2para);
    new_para.add_property(h_para2mesh);
    for (int i = 0; i < mesh.n_faces(); i++)
    {
        auto f0 = mesh.face_handle(i);
        auto f1 = new_mesh.face_handle(i);
        auto f2 = new_para.face_handle(i);
        
        std::map<int, int> v_para2mesh;
        for (auto f0_h : mesh.fh_range(f0))
        {
            v_para2mesh[mesh.property(hvt_index, f0_h)] = mesh.to_vertex_handle(f0_h).idx();
        }

        std::map<int, int> mesh_v2h;
        for (auto f1_h : new_mesh.fh_range(f1))
        {
            mesh_v2h[new_mesh.to_vertex_handle(f1_h).idx()] = f1_h.idx();
        }

        for (auto f2_h : new_para.fh_range(f2))
        {
            auto f1_h = new_mesh.halfedge_handle(mesh_v2h[v_para2mesh[new_para.to_vertex_handle(f2_h).idx()]]);

            new_mesh.property(h_mesh2para, f1_h) = std::make_pair(new_para.from_vertex_handle(f2_h).idx(), new_para.to_vertex_handle(f2_h).idx());
            new_para.property(h_para2mesh, f2_h) = std::make_pair(new_mesh.from_vertex_handle(f1_h).idx(), new_mesh.to_vertex_handle(f1_h).idx());

            origin_h_mesh2para[f1_h.idx()] = f2_h.idx();
        }
    }

    cut_length = 0.0;
    OpenMesh::EPropHandleT<bool> e_oncut;
    mesh.get_property_handle(e_oncut, "e_oncut");
    for (auto e_h : mesh.edges())
    {
        if (mesh.is_boundary(e_h))
        {
            cut_length += mesh.calc_edge_length(e_h);
            continue;
        }
        
        int to0 = mesh.property(hvt_index, mesh.halfedge_handle(e_h, 0));
        int to1 = mesh.property(hvt_index, mesh.halfedge_handle(e_h, 1));
        int from0 = mesh.property(hvt_index, mesh.prev_halfedge_handle(mesh.halfedge_handle(e_h, 0)));
        int from1 = mesh.property(hvt_index, mesh.prev_halfedge_handle(mesh.halfedge_handle(e_h, 1)));

        bool is_cut = ((to0 != from1) || (to1 != from0));
        mesh.property(e_oncut, e_h) = is_cut;
        if (is_cut) cut_length += mesh.calc_edge_length(e_h) * 2.0;
    }

    mesh.request_vertex_normals();

    new_para.request_edge_status();
    new_para.request_face_status();

    new_mesh.request_edge_status();
    new_mesh.request_face_status();

    origin_para.request_face_normals();
    origin_para.update_face_normals();

    origin_para.request_edge_colors();
    for (auto e_h : origin_para.edges())
    {
        origin_para.set_color(e_h, origin_para.is_boundary(e_h) ? boundary_edge_color : interior_edge_color);
    }

    BB_Max = BB_Min = origin_para.point(origin_para.vertex_handle(0));
    for (auto v_h : origin_para.vertices())
    {
        const auto& p = origin_para.point(v_h);
        BB_Max = BB_Max.maximize(p);
        BB_Min = BB_Min.minimize(p);
    }
    return true;
}