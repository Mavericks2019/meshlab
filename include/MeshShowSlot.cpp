#include "MeshViewerWidget.h"

void MeshViewerWidget::showLeftRight(int state)
{
	if(state == Qt::Checked)
	{
		showRight = true;
	}
	else
	{
		showRight = true;
	}
	constructShowMesh();
}

void MeshViewerWidget::showUpDown(int state)
{
	if(state == Qt::Checked)
	{
		showUp = true;
	}
	else
	{
		showUp = true;
	}
	constructShowMesh();
}

void MeshViewerWidget::showBackFront(int state)
{
	if(state == Qt::Checked)
	{
		showBack = true;
	}
	else
	{
		showBack = true;
	}
	constructShowMesh();
}

void MeshViewerWidget::xSliderShow(int value)
{
	assert( value <= 100 && value >= 0);
	x_per = value/100.0;
	constructShowMesh();
}

void MeshViewerWidget::ySliderShow(int value)
{
	assert( value <= 100 && value >= 0);
	y_per = value/100.0;
	constructShowMesh();
}

void MeshViewerWidget::zSliderShow(int value)
{
	assert( value <= 100 && value >= 0);
	z_per = value/100.0;
	constructShowMesh();
}

void MeshViewerWidget::save_show_mesh()
{
	QString fileName = QFileDialog::getSaveFileName(this,
		("Save mesh file"),
		("../models/untitled.obj"),
		("OBJ Files (*.obj);;"
		"All Files (*)"));
	if (!fileName.isEmpty())
	{
		FILE* f_show_mesh = fopen(fileName.toStdString().c_str(), "w");

		for(OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
		{
			OpenVolumeMesh::Geometry::Vec3d p = mesh_.vertex(*v_it);
			fprintf(f_show_mesh, "v %20.19f %20.19f %20.19f\n", p[0], p[1], p[2]);
		}

		fprintf(f_show_mesh, "g in_face"); 
		for(unsigned i=0;i<InnerFaceOnShowMesh_VID.size();++i)
		{
			std::vector<int>& one_face_id = InnerFaceOnShowMesh_VID[i];
			fprintf(f_show_mesh, "\nf %d %d %d",one_face_id[0]+1, one_face_id[1]+1, one_face_id[2]+1 );
		}

		fprintf(f_show_mesh, "\ng out_face");
		for(unsigned i=0;i<BoundaryFaceOnShowMesh_VID.size();++i)
		{
			std::vector<int>& one_face_id = BoundaryFaceOnShowMesh_VID[i];
			fprintf(f_show_mesh, "\nf %d %d %d",one_face_id[0]+1, one_face_id[1]+1, one_face_id[2]+1 );
		}

		fclose(f_show_mesh);
	}
}