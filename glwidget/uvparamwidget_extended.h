#ifndef UVPARAMWIDGET_EXTENDED_H
#define UVPARAMWIDGET_EXTENDED_H

#include "uvparamwidget.h"
#include "../meshutils/my_traits.h"

class UVParamWidgetExtended : public UVParamWidget
{
    Q_OBJECT

public:
    explicit UVParamWidgetExtended(QWidget *parent = nullptr);
    virtual ~UVParamWidgetExtended() = default;

    struct segment_type
	{
		int vert0, vert1, coord, tag;
	};

    void init();

    bool get_para_mesh();
    void calc_distortion();
    
    // 添加获取new_mesh和new_para的公共方法
    const Mesh& getNewMesh() const { return new_mesh; }
    const Mesh& getNewPara() const { return new_para; }
    
    OpenMesh::HalfedgeHandle get_mesh2para(OpenMesh::HalfedgeHandle h_mesh)
    {
        const auto& h_pair = new_mesh.property(h_mesh2para, h_mesh);
        return new_para.find_halfedge(new_para.vertex_handle(h_pair.first), new_para.vertex_handle(h_pair.second));
    };

protected:
    Mesh origin_para;
    Mesh new_mesh, new_para;
    Mesh mesh_unpack;
    std::vector<int> origin_h_mesh2para;
    OpenMesh::Vec3d BB_Max, BB_Min;
    OpenMesh::HPropHandleT<std::pair<int, int>> h_mesh2para, h_para2mesh;
    OpenMesh::EPropHandleT<int> e_segment;
    double cut_length = 0.0;
    std::set<int> flipped_faces;
    double para_distortion = 0.0;

    std::vector<int> vert_corner; // 存储所有角点的顶点ID
    std::vector<OpenMesh::Vec2i> quad_vertices; // 存储角点在参数空间中的整数坐标
    std::vector<int> accu_bh_chart; // 每个图（chart）的边界顶点累积数
    std::vector<int> accu_bseg_chart; // 每个图的角点累积数
    std::map<int, int> vert_k; // 记录每个顶点的度数（即相邻边数）
    std::vector<int> boundary_v; // 所有边界顶点的ID
    std::vector<int> boundary_h; // 所有边界半边的ID
    std::map<int, int> boundary_v_index; // 顶点ID -> 在 boundary_v 中的索引映射
    std::map<int, int> boundary_h_index; // 半边ID -> 在 boundary_h 中的索引映射
    std::vector<int> bh_segment;  // 每个边界半边所属的分段ID
    std::vector<int> seg_chart; // 每个分段所属的图（chart）ID
    int n_bsegs; // 边界分段的数量
    std::vector<UVParamWidgetExtended::segment_type> segments; // 所有分段的信息
    std::vector<std::vector<int>> seg_nodes;  // 每个分段上的节点（在 quad_vertices 中的索引）
    std::vector<std::set<int>> triangle_segs; // 每个三角形面包含的分段ID
    std::map<int, std::vector<int>> triangle_corner; // 每个三角形面包含的角点（在 quad_vertices 中的索引）
    std::map<int, int> v_in_quad; // 顶点ID -> 在 quad_vertices 中的索引映射

    void segment_detective();
    inline int chart_segment_cycle(int cur, int delta);
};

#endif // UVPARAMWIDGET_EXTENDED_H