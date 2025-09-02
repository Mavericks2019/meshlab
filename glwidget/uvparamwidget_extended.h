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

    bool get_para_mesh();

protected:
    Mesh origin_para;
    Mesh new_mesh, new_para;
    Mesh mesh_unpack;
    std::vector<int> origin_h_mesh2para;
    OpenMesh::Vec3d BB_Max, BB_Min;
    OpenMesh::HPropHandleT<std::pair<int, int>> h_mesh2para, h_para2mesh;
    double cut_length = 0.0;
};

#endif // UVPARAMWIDGET_EXTENDED_H