#include "MeshViewerWidget.h"

void MeshViewerWidget::draw_scene(int drawmode)
{
	if (!mesh_.n_vertices()) { return; }

	//assert(drawmode < N_DRAW_MODES);
	setDefaultMaterial();
	switch (drawmode)
	{
	case WIRE_FRAME:
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh_wireframe();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case SOLID_FLAT:
		glEnable(GL_LIGHTING);
		glShadeModel(GL_FLAT);
		draw_mesh_solidflat();
		glDisable(GL_LIGHTING);
		break;
	case FLAT_LINES:
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.5f, 2.0f);
		glEnable(GL_LIGHTING);
		glShadeModel(GL_FLAT);
		draw_mesh_solidflat();
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh_wireframe();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case ALL_POINTS:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINTS);
		draw_mesh_all_points();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case ALL_LINES:
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh_all_lines();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case ALL_POLYHEDRALS:
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh_all_polys_lines();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		draw_mesh_all_polys();
		//glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		break;
	case SINGULARITY:
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh_singularity();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case VOLUME_TEXTURE:
		glDisable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_3D);
		draw_mesh_texture();
		glDisable(GL_TEXTURE_3D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	default:
		break;
	}
}

void MeshViewerWidget::draw_mesh_wireframe() const
{
	glLineWidth(1.0);
	glColor4f(0.1, 0.1, 0.5, 1.0);
	switch(meshMode())
	{
	case TETRAHEDRAL:
		glBegin(GL_TRIANGLES);
		for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
		{
			for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &BoundaryFaceOnShowMesh[i][j][0] );
			}
		}
		for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
		{
			//glNormal3dv(&NormalOfInnerFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &InnerFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();
		break;
	case HEXAHEDRA:
		glBegin(GL_QUADS);
		for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
		{
			for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &BoundaryFaceOnShowMesh[i][j][0] );
			}
		}
		for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
		{
			//glNormal3dv(&NormalOfInnerFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &InnerFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();
		break;
	default:
		for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
		{
			glBegin(GL_POLYGON);
			for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &BoundaryFaceOnShowMesh[i][j][0] );
			}
			glEnd();
		}
		for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
		{
			//glNormal3dv(&NormalOfInnerFFaceOnShowMesh[i][0]);
			glBegin(GL_POLYGON);
			for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &InnerFaceOnShowMesh[i][j][0] );
			}
			glEnd();
		}
		break;
	}
}

void MeshViewerWidget::draw_mesh_solidflat() const
{
	GLfloat mat_a[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat mat_d[] = { 0.88f, 0.84f, 0.76f, 1.0f };
	GLfloat mat_s[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat shine[] = { 120.0f };
	GLfloat mat_a_i[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat mat_d_i[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat mat_s_i[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat shine_i[] = { 120.0f };
	switch(meshMode())
	{
	case TETRAHEDRAL:
		//glDisable(GL_LIGHTING);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_a);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_d);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_s);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shine);
		glEnable(GL_LIGHTING);
		//glColor4f(0.88, 0.84, 0.76, 1.0);
		glBegin(GL_TRIANGLES);
		for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
		{
			glNormal3dv(&NormalOfBoundaryFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &BoundaryFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_a_i);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_d_i);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_s_i);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shine_i);

		//glDisable(GL_LIGHTING);
		//glColor4f(1.0,0.50,0.50,1.0);
		glBegin(GL_TRIANGLES);
		for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
		{
			glNormal3dv(&NormalOfInnerFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &InnerFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();
		glEnable(GL_LIGHTING);
		break;
	case HEXAHEDRA:
		//glDisable(GL_LIGHTING);
		//glColor4f(0.5,0.50,0.50,1.0);
		glEnable(GL_LIGHTING);
		glBegin(GL_QUADS);
		for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
		{
			glNormal3dv(&NormalOfBoundaryFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &BoundaryFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();

		glDisable(GL_LIGHTING);
		glColor4f(1.0,0.50,0.50,1.0);
		glBegin(GL_QUADS);
		for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
		{
			//glNormal3dv(&NormalOfInnerFFaceOnShowMesh[i][0]);
			for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
			{
				glVertex3dv( &InnerFaceOnShowMesh[i][j][0] );
			}
		}
		glEnd();
		glEnable(GL_LIGHTING);
		break;
	default:
		
		break;
	}

}

void MeshViewerWidget::draw_mesh_all_lines() const
{
	glLineWidth(2.0);
	glColor4f(0.0f,1.0f,1.0f,0.5f);

	glDrawBuffer(GL_NONE);
	glDepthRange(0.01, 1.0);

	OpenVolumeMesh::EdgeIter e_it = mesh_.edges_begin();
	OpenVolumeMesh::OpenVolumeMeshEdge e = mesh_.edge(*e_it);
	OpenVolumeMesh::Geometry::Vec3d v1; 	OpenVolumeMesh::Geometry::Vec3d v2;
	for(e_it; e_it != mesh_.edges_end();++e_it)
	{
		e = mesh_.edge(*e_it);
		v1 = mesh_.vertex(e.from_vertex()); v2 = mesh_.vertex(e.to_vertex());
		glBegin(GL_LINES);
		glVertex3dv(&v1[0]); glVertex3dv(&v2[0]);
		glEnd();
	}

	glDrawBuffer(GL_BACK);
	glDepthRange(0.0, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDepthFunc(GL_LEQUAL);

	e_it = mesh_.edges_begin();
	for(e_it; e_it != mesh_.edges_end();++e_it)
	{
		e = mesh_.edge(*e_it);
		v1 = mesh_.vertex(e.from_vertex()); v2 = mesh_.vertex(e.to_vertex());
		glBegin(GL_LINES);
		glVertex3dv(&v1[0]); glVertex3dv(&v2[0]);
		glEnd();
	}

	glDepthFunc(GL_LESS);
}

void MeshViewerWidget::draw_mesh_all_polys_lines() const
{
	glLineWidth(2.0);
	glColor4f(1.0f,1.0f,1.0f,1.0f);

	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
	std::vector<OpenVolumeMesh::HalfFaceHandle> hff_Vec = cell.halffaces();
	OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_Vec[0]);
	OpenVolumeMesh::Geometry::Vec3d v;
	OpenVolumeMesh::Geometry::Vec3d temp_cell_center;
	for( c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		temp_cell_center = CellCenter[c_it->idx()];
		cell = mesh_.cell(*c_it);
		hff_Vec = cell.halffaces();
		for(unsigned int i=0;i<hff_Vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_Vec[i]);
			glBegin(GL_LINE_STRIP);
			for(hfv_it; hfv_it; ++hfv_it)
			{
				v = mesh_.vertex(*hfv_it);
				v -= temp_cell_center;
				v *= 0.8;
				v += temp_cell_center;
				glVertex3dv(&v[0]);
			}
			glEnd();
		}
	}
}

void MeshViewerWidget::draw_mesh_all_polys() const
{
	glColor4f(0.5f,0.5f,0.5f,0.5f);
	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
	std::vector<OpenVolumeMesh::HalfFaceHandle> hff_Vec = cell.halffaces();
	OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_Vec[0]);
	OpenVolumeMesh::Geometry::Vec3d v;
	OpenVolumeMesh::Geometry::Vec3d temp_cell_center;
	for( c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		temp_cell_center = CellCenter[c_it->idx()];
		cell = mesh_.cell(*c_it);
		hff_Vec = cell.halffaces();
		for(unsigned int i=0;i<hff_Vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_Vec[i]);
			glBegin(GL_POLYGON);
			for(hfv_it; hfv_it; ++hfv_it)
			{
				v = mesh_.vertex(*hfv_it);
				v -= temp_cell_center;
				v *= 0.8;
				v += temp_cell_center;
				glVertex3dv(&v[0]);
			}
			glEnd();
		}
	}
}

void MeshViewerWidget::draw_mesh_all_points()
{
	glPointSize(20);
	glColor3f( 0.0, 1.0, 1.0 );
	glBegin( GL_POINTS );
	for(OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin();v_it != mesh_.vertices_end();++v_it)
	{
		glVertex3dv( mesh_.vertex( *v_it ).data() );
	}
	glEnd();
}

void MeshViewerWidget::draw_mesh_singularity()
{
}

void MeshViewerWidget::draw_mesh_texture()
{
	if (vertex_texture.size() != mesh_.n_vertices()) return;
	glBindTexture(GL_TEXTURE_3D, Verterx_Texture_ID);
	
	switch (meshMode())
	{
	case TETRAHEDRAL:
		glBegin(GL_TRIANGLES);
		for (unsigned int i = 0; i < BoundaryFaceOnShowMesh.size(); ++i)
		{
			for (unsigned int j = 0; j < BoundaryFaceOnShowMesh[i].size(); ++j)
			{
				glTexCoord3dv( vertex_texture[BoundaryFaceOnShowMesh_VID[i][j]].data() );
				glVertex3dv(&BoundaryFaceOnShowMesh[i][j][0]);
			}
		}
		glEnd();

		glBegin(GL_TRIANGLES);
		for (unsigned int i = 0; i < InnerFaceOnShowMesh.size(); ++i)
		{
			for (unsigned int j = 0; j < InnerFaceOnShowMesh[i].size(); ++j)
			{
				glTexCoord3dv(vertex_texture[InnerFaceOnShowMesh_VID[i][j]].data());
				glVertex3dv(&InnerFaceOnShowMesh[i][j][0]);
			}
		}
		glEnd();
		break;
	case HEXAHEDRA:
		glBegin(GL_QUADS);
		for (unsigned int i = 0; i < BoundaryFaceOnShowMesh.size(); ++i)
		{
			for (unsigned int j = 0; j < BoundaryFaceOnShowMesh[i].size(); ++j)
			{
				glTexCoord3dv(vertex_texture[BoundaryFaceOnShowMesh_VID[i][j]].data());
				glVertex3dv(&BoundaryFaceOnShowMesh[i][j][0]);
			}
		}
		glEnd();

		glBegin(GL_QUADS);
		for (unsigned int i = 0; i < InnerFaceOnShowMesh.size(); ++i)
		{
			for (unsigned int j = 0; j < InnerFaceOnShowMesh[i].size(); ++j)
			{
				glTexCoord3dv(vertex_texture[InnerFaceOnShowMesh_VID[i][j]].data());
				glVertex3dv(&InnerFaceOnShowMesh[i][j][0]);
			}
		}
		glEnd();
		break;
	default:

		break;
	}
}
