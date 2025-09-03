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
    void init();

    bool get_para_mesh();
    void calc_distortion(bool silence);
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
};

#endif // UVPARAMWIDGET_EXTENDED_H