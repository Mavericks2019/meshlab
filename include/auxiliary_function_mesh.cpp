#include "InteractiveWidget.h"
#include <Eigen/Dense>

void InteractiveWidget::subdivide_Tet_TO_Hex()
{
	if( meshMode() != TETRAHEDRAL)
	{
		printf("Only support Tet Mesh.\n");
		return;
	}

	typedef OpenVolumeMesh::GeometricHexahedralMeshV3d HexMesh;
	HexMesh hex_mesh;

	int nv = mesh_.n_vertices(); int ne = mesh_.n_edges();
	int nf = mesh_.n_faces(); int nc = mesh_.n_cells();
	std::vector<bool> vertexVisited(nv);
	std::vector<OpenVolumeMesh::VertexHandle> vertex_Vertex(nv);
	std::vector<bool> edgeVisited(ne);
	std::vector<OpenVolumeMesh::VertexHandle> edge_Vertex(ne);
	std::vector<bool> faceVisited(nf);
	std::vector<OpenVolumeMesh::VertexHandle> face_Vertex(nf);
	std::vector<OpenVolumeMesh::VertexHandle> cell_Vertex(nc);
	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
	std::vector<OpenVolumeMesh::HalfFaceHandle> hff_Vec = cell.halffaces();
	OpenVolumeMesh::OpenVolumeMeshFace face = mesh_.face(mesh_.face_handle(hff_Vec[0]));
	std::vector<OpenVolumeMesh::HalfEdgeHandle> hfe_Vec = face.halfedges();
	OpenVolumeMesh::OpenVolumeMeshEdge edge = mesh_.edge(mesh_.edge_handle(hfe_Vec[0]));
	OpenVolumeMesh::VertexHandle v;
	OpenVolumeMesh::Geometry::Vec3d p;
	double count = 0;
	for( c_it; c_it!=mesh_.cells_end(); ++c_it )
	{
		p = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
		OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
		for(cv_it;cv_it;++cv_it)
		{
			p  += mesh_.vertex(*cv_it);
			count += 1.0;
			if( !vertexVisited[cv_it->idx()] )
			{
				v = hex_mesh.add_vertex( mesh_.vertex(*cv_it) );
				vertex_Vertex[cv_it->idx()] = v;
				vertexVisited[cv_it->idx()] = true;
			}
		}
		p /= count;
		v = hex_mesh.add_vertex( p );
		cell_Vertex[c_it->idx()] = v;

		cell = mesh_.cell(*c_it);
		hff_Vec = cell.halffaces();
		for( unsigned int i=0; i<hff_Vec.size(); ++i )
		{
			if( !faceVisited[mesh_.face_handle(hff_Vec[i]).idx()] )
			{
				p = OpenVolumeMesh::Geometry::Vec3d(0,0,0); count = 0;
				OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_Vec[i]);
				for(hfv_it;hfv_it;++hfv_it)
				{
					p += mesh_.vertex(*hfv_it);
					++count;
				}
				p /= count;
				v = hex_mesh.add_vertex( p );
				face_Vertex[mesh_.face_handle(hff_Vec[i]).idx()] = v;
				faceVisited[mesh_.face_handle(hff_Vec[i]).idx()] = true;

				face = mesh_.face(mesh_.face_handle(hff_Vec[i]));
				hfe_Vec = face.halfedges();
				for(unsigned int j=0;j<hfe_Vec.size();++j)
				{
					if( !edgeVisited[mesh_.edge_handle(hfe_Vec[j]).idx()] )
					{
						edge = mesh_.edge(mesh_.edge_handle(hfe_Vec[j]));
						p = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
						p += mesh_.vertex(edge.from_vertex()); p += mesh_.vertex(edge.to_vertex());
						p /= 2.0;
						v = hex_mesh.add_vertex( p );
						edge_Vertex[mesh_.edge_handle(hfe_Vec[j]).idx()] = v;
						edgeVisited[mesh_.edge_handle(hfe_Vec[j]).idx()]= true;
					}
				}
			}
		}
	}

	c_it = mesh_.cells_begin();
	std::vector<OpenVolumeMesh::VertexHandle> one_cell(8);
	for( c_it; c_it!=mesh_.cells_end(); ++c_it )
	{
		OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
		for(cv_it;cv_it;++cv_it)
		{
			one_cell[0] = vertex_Vertex[cv_it->idx()];
			one_cell[6] = cell_Vertex[c_it->idx()];

			cell = mesh_.cell(*c_it);
			hff_Vec = cell.halffaces();
			int two_edge[2] = {-1,-1};
			int face_ID = -1;
			for( unsigned int i=0; i<hff_Vec.size(); ++i )
			{
				OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_Vec[i]);
				std::vector<OpenVolumeMesh::VertexHandle> v_on_f; int index = -1; int count_v =0;
				for(hfv_it;hfv_it;++hfv_it)
				{
					v_on_f.push_back(*hfv_it);
					if( hfv_it->idx() == cv_it->idx() )
					{
						index = count_v;
					}
					++count_v;
				}
				if( index >= 0 )
				{
					OpenVolumeMesh::VertexHandle v1 = v_on_f[(index+1)%3];
					OpenVolumeMesh::VertexHandle v2 = v_on_f[(index+2)%3];

					face = mesh_.face(mesh_.face_handle(hff_Vec[i]));
					hfe_Vec = face.halfedges();
					for(unsigned int j=0;j<hfe_Vec.size();++j)
					{
						edge = mesh_.edge(mesh_.edge_handle(hfe_Vec[j]));
						if( (edge.from_vertex() == *cv_it && edge.to_vertex() == v1) || (edge.to_vertex() == *cv_it && edge.from_vertex() == v1) )
						{
							one_cell[3] = edge_Vertex[mesh_.edge_handle(hfe_Vec[j]).idx()];
							two_edge[0] = mesh_.edge_handle(hfe_Vec[j]).idx();
						}
						else if( (edge.from_vertex() == *cv_it && edge.to_vertex() == v2) || (edge.to_vertex() == *cv_it && edge.from_vertex() == v2) )
						{
							one_cell[1] = edge_Vertex[mesh_.edge_handle(hfe_Vec[j]).idx()];
							two_edge[1] = mesh_.edge_handle(hfe_Vec[j]).idx();
						}
					}
					face_ID = mesh_.face_handle(hff_Vec[i]).idx();
					one_cell[2] = face_Vertex[ mesh_.face_handle(hff_Vec[i]).idx() ];

					break;
				}
			}

			for( unsigned int i=0; i<hff_Vec.size(); ++i )
			{
				if( mesh_.face_handle(hff_Vec[i]).idx() != face_ID )
				{
					face = mesh_.face(mesh_.face_handle(hff_Vec[i]));
					hfe_Vec = face.halfedges();
					for(unsigned int j=0;j<hfe_Vec.size();++j)
					{
						if( mesh_.edge_handle(hfe_Vec[j]).idx() == two_edge[0] )
						{
							one_cell[5] = face_Vertex[ mesh_.face_handle(hff_Vec[i]).idx() ];
						}
						else if( mesh_.edge_handle(hfe_Vec[j]).idx() == two_edge[1])
						{
							one_cell[7] = face_Vertex[ mesh_.face_handle(hff_Vec[i]).idx() ];
						}
						else
						{
							edge = mesh_.edge( mesh_.edge_handle(hfe_Vec[j]) );
							if( edge.from_vertex() == *cv_it || edge.to_vertex() == *cv_it )
							{
								one_cell[4] = edge_Vertex[mesh_.edge_handle(hfe_Vec[j]).idx()];
							}
						}

					}
				}
			}

			hex_mesh.add_cell(one_cell);
		}
	}

	mesh_.clear();
	OpenVolumeMesh::IO::FileManager fm;
	if( fm.writeFile("temp.ovm",hex_mesh) )
	{
		openMesh("temp.ovm");
	}
	updateGL();

	printf("Auxiliary Function: subdivide tet to hex.\n");
}

void InteractiveWidget::transform_mesh(std::vector<double>& m)
{
	if(mesh_.n_vertices() ==0) return;

	Eigen::Matrix4d T; Eigen::Vector4d p; Eigen::Vector4d x; OpenVolumeMesh::Geometry::Vec3d vp;
	T(0,0)  = m[0]; T(0,1)  = m[1]; T(0,2)  = m[2]; T(0,3)  = m[3];
	T(1,0)  = m[4]; T(1,1)  = m[5]; T(1,2)  = m[6]; T(1,3)  = m[7];
	T(2,0)  = m[8]; T(2,1)  = m[9]; T(2,2) = m[10]; T(2,3) = m[11];
	T(3,0) = m[12]; T(3,1) = m[13]; T(3,2) = m[14]; T(3,3) = m[15];
	std::cout << T << "\n";
	for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
	{
		vp = mesh_.vertex(v_it.cur_handle());
		p(0) = vp[0]; p(1) = vp[1]; p(2) = vp[2]; p(3) = 1.0;
		x = T * p; 
		if(std::abs( x(3) ) < 1e-10 ) x(3) = 1.0;
		mesh_.set_vertex(v_it.cur_handle(), OpenVolumeMesh::Geometry::Vec3d( x(0)/x(3), x(1)/x(3), x(2)/x(3) ) );
	}

	constructBoundaryMesh();
	emit surface_mesh_changed_signal(&boundaryMesh);
	updateMesh();
	printBasicMeshInfo();
	showRight = true; showUp = true; showBack = true; 
	x_per = 0.0; y_per = 0.0; z_per = 0.0;

	constructShowMesh();
	//cell center
	constructCellCenter();
	// loading done
	updateGL();
}

void InteractiveWidget::create_grid_all_hex_mesh(int x_grid_size, int y_grid_size, int z_grid_size, double length /* = 1.0 */)
{
	typedef OpenVolumeMesh::GeometricHexahedralMeshV3d HexMesh;
	HexMesh hex_mesh;
	
	int x_vertex_number = x_grid_size + 1;
	int y_vertex_number = y_grid_size + 1;
	int z_vertex_number = z_grid_size + 1;
	std::vector<OpenVolumeMesh::VertexHandle> grid_vertex(x_vertex_number*y_vertex_number*z_vertex_number);
	OpenVolumeMesh::Geometry::Vec3d p;
	for (int i = 0; i < x_vertex_number; ++i)
	{
		p[0] = i*length;
		for (int j = 0; j < y_vertex_number; ++j)
		{
			p[1] = j*length;
			for (int k = 0; k < z_vertex_number; ++k)
			{
				p[2] = k*length;

				int v_id = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				grid_vertex[v_id] = hex_mesh.add_vertex(p);
			}
		}
	}

	std::vector<OpenVolumeMesh::VertexHandle> one_cell(8);
	for (int i = 0; i < x_grid_size; ++i)
	{
		for (int j = 0; j < y_grid_size; ++j)
		{
			for (int k = 0; k < z_grid_size; ++k)
			{
				int v_id0 = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				int v_id1 = i*z_vertex_number*y_vertex_number + (j+1)*z_vertex_number + k;
				int v_id2 = (i+1)*z_vertex_number*y_vertex_number + (j+1)*z_vertex_number + k;
				int v_id3 = (i+1)*z_vertex_number*y_vertex_number + j*z_vertex_number + k;

				int v_id4 = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k + 1;
				int v_id7 = i*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k + 1;
				int v_id6 = (i + 1)*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k + 1;
				int v_id5 = (i + 1)*z_vertex_number*y_vertex_number + j*z_vertex_number + k + 1;

				one_cell[0] = grid_vertex[v_id0]; one_cell[1] = grid_vertex[v_id1]; one_cell[2] = grid_vertex[v_id2]; one_cell[3] = grid_vertex[v_id3];
				one_cell[4] = grid_vertex[v_id4]; one_cell[5] = grid_vertex[v_id5]; one_cell[6] = grid_vertex[v_id6]; one_cell[7] = grid_vertex[v_id7];

				hex_mesh.add_cell(one_cell);
			}
		}
	}

	mesh_.clear(true);
	OpenVolumeMesh::IO::FileManager fm;
	if (fm.writeFile("temp.ovm", hex_mesh))
	{
		openMesh("temp.ovm");
	}
	updateGL();

	printf("Auxiliary Function: Create Grid All-Hex Mesh.\n");
}

void InteractiveWidget::create_grid_mesh_mesh()
{
	if (mesh_.n_vertices() == 0) return;
	create_slice_mesh(); 
	return;

	OpenVolumeMesh::Geometry::Vec3d box_length = mesh_.BBox_max - mesh_.BBox_min;
	double grid_width = mesh_.avg_edge_length*1.0;
	int x_grid_number = (int)(std::ceil(box_length[0] / grid_width));
	if (x_grid_number % 2 == 1) x_grid_number += 1;
	int y_grid_number = (int)(std::ceil(box_length[1] / grid_width));
	if (y_grid_number % 2 == 1) y_grid_number += 1;
	int z_grid_number = (int)(std::ceil(box_length[2] / grid_width));
	if (z_grid_number % 2 == 1) z_grid_number += 1;

	int x_vertex_number = x_grid_number + 1;
	int y_vertex_number = y_grid_number + 1;
	int z_vertex_number = z_grid_number + 1;

	double x_vertex_min = mesh_.BBox_center[0] - (x_grid_number / 2)*grid_width;
	double y_vertex_min = mesh_.BBox_center[1] - (y_grid_number / 2)*grid_width;
	double z_vertex_min = mesh_.BBox_center[2] - (z_grid_number / 2)*grid_width;

	typedef OpenVolumeMesh::GeometricHexahedralMeshV3d HexMesh;
	HexMesh grid_mesh;

	std::vector<OpenVolumeMesh::VertexHandle> grid_vertex(x_vertex_number*y_vertex_number*z_vertex_number);
	OpenVolumeMesh::Geometry::Vec3d p;
	for (int i = 0; i < x_vertex_number; ++i)
	{
		p[0] = i*grid_width + x_vertex_min;
		for (int j = 0; j < y_vertex_number; ++j)
		{
			p[1] = j*grid_width + y_vertex_min;
			for (int k = 0; k < z_vertex_number; ++k)
			{
				p[2] = k*grid_width + z_vertex_min;

				int v_id = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				grid_vertex[v_id] = grid_mesh.add_vertex(p);
			}
		}
	}

	std::vector<OpenVolumeMesh::VertexHandle> one_cell(8);
	for (int i = 0; i < x_grid_number; ++i)
	{
		for (int j = 0; j < y_grid_number; ++j)
		{
			for (int k = 0; k < z_grid_number; ++k)
			{
				int v_id0 = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				int v_id1 = i*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k;
				int v_id2 = (i + 1)*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k;
				int v_id3 = (i + 1)*z_vertex_number*y_vertex_number + j*z_vertex_number + k;

				int v_id4 = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k + 1;
				int v_id7 = i*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k + 1;
				int v_id6 = (i + 1)*z_vertex_number*y_vertex_number + (j + 1)*z_vertex_number + k + 1;
				int v_id5 = (i + 1)*z_vertex_number*y_vertex_number + j*z_vertex_number + k + 1;

				one_cell[0] = grid_vertex[v_id0]; one_cell[1] = grid_vertex[v_id1]; one_cell[2] = grid_vertex[v_id2]; one_cell[3] = grid_vertex[v_id3];
				one_cell[4] = grid_vertex[v_id4]; one_cell[5] = grid_vertex[v_id5]; one_cell[6] = grid_vertex[v_id6]; one_cell[7] = grid_vertex[v_id7];

				grid_mesh.add_cell(one_cell);
			}
		}
	}

	//delete grid which is not in the original mesh
	std::vector<int> cell_flag(grid_mesh.n_cells(), -1);
	for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
	{
		OpenVolumeMesh::Geometry::Vec3d p = mesh_.vertex(*v_it);
		int x_id = (int)std::floor((p[0] - x_vertex_min) / grid_width);
		int y_id = (int)std::floor((p[1] - y_vertex_min) / grid_width);
		int z_id = (int)std::floor((p[2] - z_vertex_min) / grid_width);

		int cell_id = x_id *z_grid_number*y_grid_number + y_id * z_grid_number + z_id;
		cell_flag[cell_id] = 1;
	}

	for (OpenVolumeMesh::CellIter c_it = mesh_.cells_begin(); c_it != mesh_.cells_end(); ++c_it)
	{
		OpenVolumeMesh::Geometry::Vec3d bb_c_min = mesh_.BBox_max;
		OpenVolumeMesh::Geometry::Vec3d bb_c_max = mesh_.BBox_min;
		for (OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it); cv_it; ++cv_it)
		{
			bb_c_min.minimize(mesh_.vertex(cv_it.cur_handle()));
			bb_c_max.maximize(mesh_.vertex(cv_it.cur_handle()));
		}

		int x_id_min = (int)std::floor((bb_c_min[0] - x_vertex_min) / grid_width);
		int y_id_min = (int)std::floor((bb_c_min[1] - y_vertex_min) / grid_width);
		int z_id_min = (int)std::floor((bb_c_min[2] - z_vertex_min) / grid_width);

		int x_id_max = (int)std::floor((bb_c_max[0] - x_vertex_min) / grid_width);
		int y_id_max = (int)std::floor((bb_c_max[1] - y_vertex_min) / grid_width);
		int z_id_max = (int)std::floor((bb_c_max[2] - z_vertex_min) / grid_width);

		for (int i = x_id_min; i <= x_id_max; ++i)
		{
			for (int j = y_id_min; j <= y_id_max; ++j)
			{
				for (int k = z_id_min; k <= z_id_max; ++k)
				{
					int cell_id = i *z_grid_number*y_grid_number + j * z_grid_number + k;
					cell_flag[cell_id] = 1;
				}
			}
		}
	}

	std::vector<int> vertex_flag(grid_mesh.n_vertices(), 1); bool have_cell_flag = false;
	int grid_nv_count = grid_mesh.n_vertices();
	for (OpenVolumeMesh::VertexIter v_it = grid_mesh.vertices_begin(); v_it != grid_mesh.vertices_end(); ++v_it)
	{
		have_cell_flag = false;
		for (OpenVolumeMesh::VertexCellIter vc_it = grid_mesh.vc_iter(*v_it); vc_it; ++vc_it)
		{
			if (cell_flag[vc_it->idx()] == 1) //exists a cell which will not be deleted, then this vertex will not be deleted
			{
				have_cell_flag = true; break;
			}
		}

		if (!have_cell_flag) // all the cells need to delete, then we delete the vertex and mark a flag
		{
			vertex_flag[v_it->idx()] = -1;
			--grid_nv_count;
		}
	}
	printf("Grid nv : %d\n", grid_nv_count);

	std::vector<int> edge_flag(grid_mesh.n_edges(), 1);
	for (OpenVolumeMesh::EdgeIter e_it = grid_mesh.edges_begin(); e_it != grid_mesh.edges_end(); ++e_it)
	{
		have_cell_flag = false; int cell_count = 0;
		OpenVolumeMesh::HalfEdgeHandle heh = grid_mesh.halfedge_handle(*e_it, 0);
		for (OpenVolumeMesh::HalfEdgeHalfFaceIter hehf_it = grid_mesh.hehf_iter(heh); hehf_it; ++hehf_it)
		{
			OpenVolumeMesh::CellHandle ch = grid_mesh.incident_cell(*hehf_it);
			if (ch.idx() >= 0)
			{
				if (cell_flag[ch.idx()] == 1)
				{
					have_cell_flag = true;
					break;
				}
			}
		}

		if (!have_cell_flag)
		{
			edge_flag[e_it->idx()] = -1;
		}
	}

	std::vector<int> face_flag(grid_mesh.n_faces(), 1);
	for (OpenVolumeMesh::FaceIter f_it = grid_mesh.faces_begin(); f_it != grid_mesh.faces_end(); ++f_it)
	{
		have_cell_flag = false;
		OpenVolumeMesh::HalfFaceHandle hfh0 = grid_mesh.halfface_handle(*f_it, 0);
		OpenVolumeMesh::HalfFaceHandle hfh1 = grid_mesh.halfface_handle(*f_it, 1);
		OpenVolumeMesh::CellHandle ch = grid_mesh.incident_cell(hfh0);
		if (ch.idx() >= 0)
		{
			if (cell_flag[ch.idx()] == 1)
			{
				have_cell_flag = true; break;
			}
		}

		ch = grid_mesh.incident_cell(hfh1);
		if (ch.idx() >= 0)
		{
			if (cell_flag[ch.idx()] == 1)
			{
				have_cell_flag = true; break;
			}
		}

		if (!have_cell_flag)
		{
			face_flag[f_it->idx()] = -1;
		}
	}

	FILE* f_second_layer_p = fopen("D:\\software\\R2012a\\matlab_code\\second_layer_p.txt", "w");
	for (int i = 0; i < x_vertex_number; i += 2)
	{
		for (int j = 0; j < y_vertex_number; j += 2)
		{
			for (int k = 0; k < z_vertex_number; k += 2)
			{
				int v_id = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				if (vertex_flag[v_id] != -1)
				{
					p = grid_mesh.vertex(OpenVolumeMesh::VertexHandle(v_id));
					fprintf(f_second_layer_p, "%.19f %.19f %.19f\n", p[0], p[1], p[2]);
				}
			}
		}
	}

	FILE* f_fist_layer_p = fopen("D:\\software\\R2012a\\matlab_code\\first_layer_p.txt", "w");
	for (int i = 0; i < x_vertex_number; i += 3)
	{
		for (int j = 0; j < y_vertex_number; j += 3)
		{
			for (int k = 0; k < z_vertex_number; k += 3)
			{
				int v_id = i*z_vertex_number*y_vertex_number + j*z_vertex_number + k;
				if (vertex_flag[v_id] != -1)
				{
					p = grid_mesh.vertex(OpenVolumeMesh::VertexHandle(v_id));
					fprintf(f_fist_layer_p, "%.19f %.19f %.19f\n", p[0], p[1], p[2]);
				}
			}
		}
	}
	fclose(f_fist_layer_p); fclose(f_second_layer_p);

	printf("Delete Cell............\n");
	int nc = cell_flag.size();
	for (int i = 0; i < nc; ++i)
	{
		int c_id = nc - i - 1;
		if (cell_flag[c_id] == -1)
		{
			grid_mesh.delete_cell(OpenVolumeMesh::CellHandle(c_id));
		}
	}

	printf("Delete Face............\n");
	int nf = face_flag.size();
	for (int i = 0; i < nf; ++i)
	{
		int f_id = nf - i - 1;
		if (face_flag[f_id] == -1)
		{
			grid_mesh.delete_face(OpenVolumeMesh::FaceHandle(f_id));
		}
	}

	printf("Delete Edge............\n");
	int ne = edge_flag.size();
	for (int i = 0; i < ne; ++i)
	{
		int e_id = ne - i - 1;
		if (edge_flag[e_id] == -1)
		{
			grid_mesh.delete_edge(OpenVolumeMesh::EdgeHandle(e_id));
		}
	}

	printf("Delete Vertex............\n");
	int nv = vertex_flag.size();
	for (int i = 0; i < nv; ++i)
	{
		int v_id = nv - 1 - i;
		if (vertex_flag[v_id] == -1)
		{
			grid_mesh.delete_vertex(OpenVolumeMesh::VertexHandle(v_id));
		}
	}

	FILE* f_third_layer_p = fopen("D:\\software\\R2012a\\matlab_code\\third_layer_p.txt", "w");
	for (OpenVolumeMesh::VertexIter v_it = grid_mesh.vertices_begin(); v_it != grid_mesh.vertices_end(); ++v_it)
	{
		p = grid_mesh.vertex(*v_it);
		fprintf(f_third_layer_p, "%.19f %.19f %.19f\n", p[0], p[1], p[2]);
	}
	fclose(f_third_layer_p); 


	for (OpenVolumeMesh::VertexIter v_it = grid_mesh.vertices_begin(); v_it != grid_mesh.vertices_end(); ++v_it)
	{
		int valence_edge = grid_mesh.valence(*v_it);
		if (valence_edge == 1)
			printf("%d\n", v_it->idx());
	}

	mesh_.clear(true);
	OpenVolumeMesh::IO::FileManager fm;
	if (fm.writeFile("temp.ovm", grid_mesh))
	{
		openMesh("temp.ovm");
	}
	updateGL();

	printf("Auxiliary Function: Create Grid Mesh.\n");
}

void InteractiveWidget::create_slice_mesh()
{
	/*QImage image = QImage(2900, 400, QImage::Format_RGB32); QRgb pixel_v;
	for (int i = 0; i < 2900; i++)
	{
		for (int j = 0; j < 400; j++)
		{
			int id = j * 2900 + i;
			if ((i / 50 + j / 50) % 2 == 1)
			{
				pixel_v = qRgb(1, 1, 1);
				image.setPixel(i, j, pixel_v);
			}
			else
			{
				pixel_v = qRgb(255, 255, 255);
				image.setPixel(i, j, pixel_v);
			}
		}
	}
	//QImage image = QImage(RGBA,width,height,QImage::Format_RGB32);
	image.save("Image.png");
	return;*/

	printf("begin construct slice mesh.\n");

	unsigned n = mesh_.n_cells();
	ANNpointArray dataPts = annAllocPts(n, 3);
	OpenVolumeMesh::Geometry::Vec3d p; double max_d = 0.0;
	for (OpenVolumeMesh::CellIter c_it = mesh_.cells_begin(); c_it != mesh_.cells_end(); ++c_it)
	{
		int c_id = c_it->idx();
		OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
		p = mesh_.vertex(*cv_it); ++cv_it; double count = 1.0;
		for (; cv_it; ++cv_it)
		{
			p += mesh_.vertex(*cv_it); count += 1.0;
			double d = (mesh_.BBox_center - mesh_.vertex(*cv_it)).sqrnorm();
			if (d > max_d)
			{
				max_d = d;
			}
		}
		p /= count;
		dataPts[c_id][0] = p[0]; dataPts[c_id][1] = p[1]; dataPts[c_id][2] = p[2];
	}

	ANNkd_tree* cc_kdTree = new ANNkd_tree(dataPts, n, 3);

	SurfaceMesh s_mesh_;
	OpenMesh::IO::read_mesh(s_mesh_, "slice.obj");
	ANNpoint tp1 = annAllocPt(3); 
	OpenVolumeMesh::Geometry::Vec3d q1_;
	ANNidxArray nnIdx1 = new ANNidx[1]; ANNdistArray dists1 = new ANNdist[1];
	for (SurfaceMesh::VertexIter v_it = s_mesh_.vertices_begin(); v_it != s_mesh_.vertices_end();++v_it)
	{
		OpenMesh::Vec3d p = s_mesh_.point(v_it);
		tp1[0] = p[0]; tp1[1] = p[1]; tp1[2] = p[2];
		cc_kdTree->annkSearch(tp1, 1, nnIdx1, dists1);
		int result = nnIdx1[0];
		q1_[0] = p[0]; q1_[1] = p[1]; q1_[2] = p[2];
		//check in cells
		int in_tet_flag = -1;
		if (result < 0 || !check_in_tet(&mesh_, q1_, OpenVolumeMesh::CellHandle(result)))
		{
			for (OpenVolumeMesh::CellIter c_it = mesh_.cells_begin(); c_it != mesh_.cells_end(); ++c_it)
			{
				if (check_in_tet(&mesh_, q1_, *c_it))
				{
					in_tet_flag = 1;
					break;
				}
			}
		}
		else
		{
			in_tet_flag = 1;
		}

		if (in_tet_flag == -1)
		{
			s_mesh_.delete_vertex(v_it);
		}
		printf("%d %d\n", v_it.handle().idx(), int(s_mesh_.n_vertices()));
	}
	s_mesh_.delete_isolated_vertices();
	s_mesh_.garbage_collection();
	OpenMesh::IO::write_mesh(s_mesh_, "temp.obj");
	return;


	double grid_w = mesh_.avg_edge_length*0.5;
	//double grid_w = 0.1;
	//double z = mesh_.BBox_center[1];
	double z = 0.63;
	double x_min = mesh_.BBox_min[0]; double y_min = mesh_.BBox_min[2];
	int x_size = (int)( (mesh_.BBox_max[0] - mesh_.BBox_min[0]) / grid_w );
	int y_size = (int)( (mesh_.BBox_max[2] - mesh_.BBox_min[2]) / grid_w );
	ANNpoint tp = annAllocPt(3); tp[1] = z;
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	OpenMesh::Vec3d q(0, z ,0); OpenVolumeMesh::Geometry::Vec3d q_(0, z, 0);
	std::vector<SurfaceMesh::VertexHandle> slice_vh( (x_size+1)*(y_size+1) );
	std::vector<int> in_tet((x_size + 1)*(y_size + 1), 1);
	std::vector<int> in_tet_id((x_size + 1)*(y_size + 1), -1);
	printf("%d\n", (x_size + 1)*(y_size + 1));
	//SurfaceMesh s_mesh_;
	for (int i = 0; i <= x_size; ++i)
	{
		double x = x_min + i*grid_w;
		q[0] = x; q_[0] = x; tp[0] = x;
		for (int j = 0; j <= y_size; ++j)
		{
			double y = y_min + j*grid_w;
			q[2] = y; q_[2] = y; tp[2] = y;
			int v_id = i*(y_size + 1) + j;
			slice_vh[v_id] = s_mesh_.add_vertex(q);
			if ((x - mesh_.BBox_center[0])*(x - mesh_.BBox_center[0]) + (y - mesh_.BBox_center[1])*(y - mesh_.BBox_center[0]) > max_d)
			{
				//in_tet[v_id] = -1;
				//continue;
			}
			cc_kdTree->annkSearch(tp, 1, nnIdx, dists);
			int result = nnIdx[0];
			//check in cells
			if (result < 0 || !check_in_tet(&mesh_, q_, OpenVolumeMesh::CellHandle(result)))
			{
				int in_tet_flag = -1;
				for (OpenVolumeMesh::CellIter c_it = mesh_.cells_begin(); c_it != mesh_.cells_end(); ++c_it)
				{
					if (check_in_tet(&mesh_, q_, *c_it))
					{
						in_tet_flag = 1;
						in_tet_id[v_id] = c_it->idx();
						s_mesh_.data(slice_vh[v_id]).set_tet_vertex_id(c_it->idx());
						break;
					}
				}
				in_tet[v_id] = in_tet_flag;
			}
			else
			{
				in_tet_id[v_id] = result;
				s_mesh_.data(slice_vh[v_id]).set_tet_vertex_id(result);
			}
		}
		printf("%d %d\n", i, x_size);
	}
	//
	printf("Add Face\n");
	std::vector<SurfaceMesh::VertexHandle> one_face_vh(4);
	for (int i = 0; i < x_size; ++i)
	{
		for (int j = 0; j < y_size; ++j)
		{
			one_face_vh[0] = slice_vh[i*(y_size + 1) + j];
			if (in_tet[i*(y_size + 1) + j] != 1) continue;
			one_face_vh[1] = slice_vh[i*(y_size + 1) + j+1];
			if (in_tet[i*(y_size + 1) + j + 1] != 1) continue;
			one_face_vh[2] = slice_vh[(i+1)*(y_size + 1) + j+1];
			if (in_tet[(i + 1)*(y_size + 1) + j + 1] != 1) continue;
			one_face_vh[3] = slice_vh[(i+1)*(y_size + 1) + j];
			if (in_tet[(i + 1)*(y_size + 1) + j] != 1) continue;
			s_mesh_.add_face(one_face_vh);
		}
	}

	printf("Delete Face\n");
	for (int i = 0; i < in_tet.size(); ++i)
	{
		if (in_tet[i] != 1)
		{
			for (SurfaceMesh::VertexOHalfedgeIter voh_it = s_mesh_.voh_iter(s_mesh_.vertex_handle(i)); voh_it; ++voh_it)
			{
				SurfaceMesh::FaceHandle fh = s_mesh_.face_handle(voh_it.handle());
				if (fh.is_valid())
				{
					s_mesh_.delete_face(fh);
				}
			}
		}
	}
	s_mesh_.delete_isolated_vertices();
	s_mesh_.garbage_collection();

	printf("Write Data\n");
	OpenMesh::IO::write_mesh(s_mesh_, "temp.obj");
	printf("finish construct slice mesh.\n");
	FILE* f_tet_id = fopen("tet_id.txt", "w");
	for (SurfaceMesh::VertexIter v_it = s_mesh_.vertices_begin(); v_it != s_mesh_.vertices_end(); ++v_it)
	{
		fprintf( f_tet_id, "%d\n", s_mesh_.data(v_it).get_tet_vertex_id() );
	}
	fclose(f_tet_id);

	delete cc_kdTree;
}
