#include <QDragEnterEvent>
#include <QtCore>
#include <QUrl>

#include "InteractiveWidget.h"

InteractiveWidget::InteractiveWidget(QWidget* parent /* = 0 */)
	:MeshViewerWidget(parent)
{
	//mouse_mode_ = TRANS;
	setMouseMode(TRANS);
	draw_main_scene = true;
	setAllFunction(false);
	kdTree = NULL;
	picked_vertex = VolumeMesh::InvalidVertexHandle;
	tempfont = QFont("Helvetica", 20);
	iso_d_str = QString("ID: Max : 1.000; Avg : 1.000; FPS : 0 ");
	//picked_vertex = OpenVolumeMesh::VertexHandle(104231);
}

InteractiveWidget::InteractiveWidget(QGLFormat& _fmt, QWidget* _parent)
	:MeshViewerWidget(_fmt, _parent)
{
	//mouse_mode_ = TRANS;
	setMouseMode(TRANS);
	draw_main_scene = true;
	setAllFunction(false);
	kdTree = NULL;
	picked_vertex = VolumeMesh::InvalidVertexHandle;
	tempfont = QFont("Helvetica", 20);
	iso_d_str = QString("ID: Max : 1.000; Avg : 1.000; FPS : 0 ");
	//picked_vertex = OpenVolumeMesh::VertexHandle(104231);
}

InteractiveWidget::~InteractiveWidget()
{
	if(kdTree) delete kdTree;
}

void InteractiveWidget::pick_point(int x,int y,double p[3])
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble winX = double(x);
	GLdouble winY = double(viewport[3] - y);
	GLfloat winZ = 0.0;
	glReadPixels((int)winX, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	gluUnProject(winX, winY, (GLdouble)winZ, modelview_matrix_, projection_matrix_, viewport, &p[0], &p[1], &p[2]);
}

void InteractiveWidget::pick_point_get_depth(int x, int y, double p[3], double& d)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble winX = double(x);
	GLdouble winY = double(viewport[3] - y);
	GLfloat winZ = 0.0;
	glReadPixels((int)winX, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	d = (GLdouble)winZ;
	gluUnProject(winX, winY, (GLdouble)winZ, modelview_matrix_, projection_matrix_, viewport, &p[0], &p[1], &p[2]);
}

void InteractiveWidget::pick_point_with_depth(int x, int y, double p[3], double d)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble winX = double(x);
	GLdouble winY = double(viewport[3] - y);
	gluUnProject(winX, winY, d, modelview_matrix_, projection_matrix_, viewport, &p[0], &p[1], &p[2]);
}

void InteractiveWidget::mousePressEvent(QMouseEvent *_event)
{
	if(mouse_mode_ == TRANS)
	{
		QGLViewerWidget::mousePressEvent(_event);
	}
	else
	{
		double p[3];
		pick_point( _event->x(), _event->y(), p);
		if(mouse_mode_ == VolumePICK)
		{
			if(HaveFunction_TO)
			{
				pick_Column(p);
			}
		}
		else if(mouse_mode_ == SHEETPICK)
		{
			if(HaveFunction_TO)
			{
				pick_Sheet(p);
			}
		}
		else if(mouse_mode_ == CELLPICK)
		{
			if(HaveFunction_TO)
			{
				pick_cell(p);
			}
		}
		else if(mouse_mode_ == VERTEXPICK)
		{
			if(HaveFunction_TO)
			{
				pick_vertex(p);
				pick_vertex_column_split(p);
			}

			if (HaveFunction_TM )
			{
				pick_vertex(p);
				if (picked_vertex != VolumeMesh::InvalidVertexHandle)
				{
					emit pick_vertex_as_handle_DE_signal(picked_vertex);
				}
			}
			if (HaveFunction_GMD && picked_vertex != VolumeMesh::InvalidVertexHandle)
			{
				pick_vertex(p);
				emit pick_vertex_as_handle_GMD_signal(picked_vertex);
			}
			if (HaveFunction_MLD)
			{
				emit pick_vertex_as_handle_MLD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0],p[1], p[2]));
			}
		}
		else if(mouse_mode_ == POINTPICK)
		{
		}
		else if(mouse_mode_ == FACEPICK)
		{
			pick_face(p);
			if(HaveFunction_TS)
			{
				emit pick_face_handle_for_trace_sheet(picked_face);
			}
		}
		else if (mouse_mode_ == EDGEPICK)
		{
			pick_edge(p);
			if (HaveFunction_TM && picked_edge != VolumeMesh::InvalidEdgeHandle)
			{
				emit pick_edge_as_feature_signal(picked_edge);
			}
		}
		else if (mouse_mode_ == VERTEXMOVE)
		{
			pick_point_get_depth(_event->x(), _event->y(), p, pick_point_depth);
			if (HaveFunction_TM && picked_vertex != VolumeMesh::InvalidVertexHandle)
			{
				pick_vertex(p);
				printf("%f %f %f\n", p[0], p[1], p[2]);
				OpenVolumeMesh::Geometry::Vec3d q = mesh_.vertex(picked_vertex);
				printf("%f %f %f\n", q[0], q[1], q[2]);
				emit pick_handle_for_move_DE_signal(picked_vertex);
			}
			if (HaveFunction_GMD && picked_vertex != VolumeMesh::InvalidVertexHandle)
			{
				pick_vertex(p);
				printf("%f %f %f\n", p[0], p[1], p[2]);
				OpenVolumeMesh::Geometry::Vec3d q = mesh_.vertex(picked_vertex);
				printf("%f %f %f\n", q[0], q[1], q[2]);
				emit pick_handle_for_move_GMD_signal(picked_vertex);
			}
			if (HaveFunction_MLD )
			{
				emit pick_handle_for_move_MLD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
		}

		updateGL();
		//setMouseMode(TRANS);
	}
}

void InteractiveWidget::mouseMoveEvent(QMouseEvent *_event)
{
	if (mouse_mode_ == TRANS)
	{
		QGLViewerWidget::mouseMoveEvent(_event);
	}
	else
	{
		if (mouse_mode_ == VERTEXMOVE)
		{
			double p[3];
			pick_point_with_depth(_event->x(), _event->y(), p, pick_point_depth);
			//printf("%f %f %f\n", p[0], p[1], p[2]);
			if (HaveFunction_TM)
			{
				emit handle_move_DE_signal(OpenVolumeMesh::Geometry::Vec3d( p[0], p[1], p[2]) ); 
			}
			if (HaveFunction_GMD)
			{
				emit handle_move_GMD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
			if (HaveFunction_MLD)
			{
				emit handle_move_MLD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
		}
		updateGL();
	}
}

void InteractiveWidget::mouseReleaseEvent(QMouseEvent *_event)
{
	if (mouse_mode_ == TRANS)
	{
		QGLViewerWidget::mouseMoveEvent(_event);
	}
	else
	{
		if (mouse_mode_ == VERTEXMOVE)
		{
			double p[3];
			pick_point_with_depth(_event->x(), _event->y(), p, pick_point_depth);
			//printf("%f %f %f\n", p[0], p[1], p[2]);
			if (HaveFunction_TM)
			{
				emit handle_release_DE_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
			if (HaveFunction_GMD)
			{
				emit handle_release_GMD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
			if (HaveFunction_MLD)
			{
				emit handle_release_MLD_signal(OpenVolumeMesh::Geometry::Vec3d(p[0], p[1], p[2]));
			}
		}
		updateGL();
	}
}

void InteractiveWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if( event->mimeData()->hasFormat("text/uri-list") )
	{
		event->acceptProposedAction();
	}
}

void InteractiveWidget::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if( urls.isEmpty() )
		return;

	QString fileName = urls.first().toLocalFile();
	if (fileName.isEmpty())
		return;

	if( fileName.endsWith(".ovm") )
	{
		bool open_OK =  openMesh(fileName.toLocal8Bit());
		if(open_OK)
		{
			emit(loadMeshOK(true,fileName));
			if (HaveFunction_TM && boundaryMesh.n_vertices() > 0)
			{
				emit surface_mesh_changed_signal(&boundaryMesh);
			}
		}
		else
		{
			//emit(loadMeshOK(false));
		}
	}
}

void InteractiveWidget::buildIndex()
{
	unsigned n = FaceCenterOnShowMesh.size();
	ANNpointArray dataPts = annAllocPts(n, 3);
	OpenVolumeMesh::Geometry::Vec3d p;
	for(unsigned int i=0; i < FaceCenterOnShowMesh.size(); ++i)
	{
		p = FaceCenterOnShowMesh[i];
		dataPts[i][0] = p[0]; dataPts[i][1] = p[1]; dataPts[i][2] = p[2];
	}

	if(kdTree) delete kdTree;
	kdTree = new ANNkd_tree(dataPts, n, 3);
}

void InteractiveWidget::pick_Sheet(double p[3])
{
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{
		//find the face
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell
		double minLen = 2*radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0,0,0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for(c_it; c_it != c_end; ++c_it)
		{
			if( ValidCell[c_it->idx()] )
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if( minLen > len )
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		//desired face
		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount =0;
		for(unsigned int i=0;i<hff_vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
			faceVertexCount = 0;
			for( hfv_it; hfv_it; ++hfv_it )
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if( faceCenter.length() < minLen )
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}

		emit(pick_Sheet_signal(desired_HalffaceHnadle));
	}
}

void InteractiveWidget::pick_Column(double p[3])
{
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell
		double minLen = 2*radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0,0,0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for(c_it; c_it != c_end; ++c_it)
		{
			if( ValidCell[c_it->idx()] )
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if( minLen > len )
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount =0;
		for(unsigned int i=0;i<hff_vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
			faceVertexCount = 0;
			for( hfv_it; hfv_it; ++hfv_it )
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if( faceCenter.length() < minLen )
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}

		emit(pick_Column_signal(desired_HalffaceHnadle));
	}
}

void InteractiveWidget::pick_vertex_column_split(double p[3])
{
	OpenVolumeMesh::Geometry::Vec3d s_p( p[0], p[1], p[2] );
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell handle
		double minLen = 2*radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0,0,0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for(c_it; c_it != c_end; ++c_it)
		{
			if( ValidCell[c_it->idx()] )
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if( minLen > len )
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		//find the half face handle
		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount =0;
		for(unsigned int i=0;i<hff_vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
			faceVertexCount = 0;
			for( hfv_it; hfv_it; ++hfv_it )
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if( faceCenter.length() < minLen )
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}
		//find the desired vertex
		hfv_it = mesh_.hfv_iter(desired_HalffaceHnadle);
		OpenVolumeMesh::Geometry::Vec3d v; minLen = 2*radius();
		OpenVolumeMesh::VertexHandle desired_VertexHandle;
		for( hfv_it; hfv_it; ++hfv_it )
		{
			v = s_p - mesh_.vertex(*hfv_it);
			len = v.length();
			if( len < minLen )
			{
				minLen = len;
				desired_VertexHandle = *hfv_it;
			}
		}

		emit(pick_vertex_column_split_signal(desired_cellHandle,desired_HalffaceHnadle,desired_VertexHandle));
	}
}

void InteractiveWidget::pick_vertex(double p[3])
{
	OpenVolumeMesh::Geometry::Vec3d s_p( p[0], p[1], p[2] );
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell handle
		double minLen = 2*radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0,0,0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for(c_it; c_it != c_end; ++c_it)
		{
			if( ValidCell[c_it->idx()] )
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if( minLen > len )
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		//find the half face handle
		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount =0;
		for(unsigned int i=0;i<hff_vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
			faceVertexCount = 0;
			for( hfv_it; hfv_it; ++hfv_it )
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if( faceCenter.length() < minLen )
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}
		//find the desired vertex
		hfv_it = mesh_.hfv_iter(desired_HalffaceHnadle);
		OpenVolumeMesh::Geometry::Vec3d v; minLen = 2*radius();
		OpenVolumeMesh::VertexHandle desired_VertexHandle;
		for( hfv_it; hfv_it; ++hfv_it )
		{
			v = s_p - mesh_.vertex(*hfv_it);
			len = v.length();
			if( len < minLen )
			{
				minLen = len;
				desired_VertexHandle = *hfv_it;
			}
		}

		picked_vertex = desired_VertexHandle;
		printf("Picked V : %d\n", picked_vertex.idx());
		updateGL();
	}
}

void InteractiveWidget::pick_cell(double p[3])
{
	//selectedPoint.push_back( Mesh::Point(p[0], p[1], p[2]) );*/
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{

	}
}

void InteractiveWidget::pick_edge(double p[3])
{
	OpenVolumeMesh::Geometry::Vec3d s_p(p[0], p[1], p[2]);
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if (result >= 0)
	{
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell handle
		double minLen = 2 * radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0, 0, 0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for (c_it; c_it != c_end; ++c_it)
		{
			if (ValidCell[c_it->idx()])
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if (minLen > len)
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		//find the half face handle
		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount = 0;
		for (unsigned int i = 0; i < hff_vec.size(); ++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0, 0, 0);
			faceVertexCount = 0;
			for (hfv_it; hfv_it; ++hfv_it)
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if (faceCenter.length() < minLen)
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}
		//find the desired vertex
		OpenVolumeMesh::OpenVolumeMeshFace face = mesh_.face(mesh_.face_handle(desired_HalffaceHnadle) );
		OpenVolumeMesh::Geometry::Vec3d v; minLen = 2 * radius();
		OpenVolumeMesh::EdgeHandle desired_EdgeHandle;
		std::vector<OpenVolumeMesh::HalfEdgeHandle> heh_vec = face.halfedges();
		for (int i = 0; i < heh_vec.size();++i)
		{
			OpenVolumeMesh::EdgeHandle eh = mesh_.edge_handle(heh_vec[i]);
			OpenVolumeMesh::OpenVolumeMeshEdge edge = mesh_.edge(eh);
			v = s_p - 0.5*( mesh_.vertex(edge.from_vertex()) + mesh_.vertex(edge.to_vertex()) );
			len = v.length();
			if (len < minLen)
			{
				minLen = len;
				desired_EdgeHandle = eh;
			}
		}
		picked_edge = desired_EdgeHandle;
		printf("Picked E : %d\n", picked_edge.idx());
		updateGL();
	}
}

void InteractiveWidget::pick_face(double p[3])
{
	OpenVolumeMesh::Geometry::Vec3d s_p( p[0], p[1], p[2] );
	ANNpoint tp = annAllocPt(3); tp[0] = p[0]; tp[1] = p[1]; tp[2] = p[2];
	ANNidxArray nnIdx = new ANNidx[1]; ANNdistArray dists = new ANNdist[1];
	kdTree->annkSearch(tp, 1, nnIdx, dists);
	int result = nnIdx[0];
	if( result >= 0 )
	{
		OpenVolumeMesh::Geometry::Vec3d resultVolumeFaceP = FaceCenterOnShowMesh[result];
		//find the cell handle
		double minLen = 2*radius(); double len;
		OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
		OpenVolumeMesh::CellIter c_end = mesh_.cells_end();
		OpenVolumeMesh::Geometry::Vec3d cc(0,0,0);
		OpenVolumeMesh::CellHandle desired_cellHandle;
		for(c_it; c_it != c_end; ++c_it)
		{
			if( ValidCell[c_it->idx()] )
			{
				cc = CellCenter[c_it->idx()] - resultVolumeFaceP;
				len = cc.length();
				if( minLen > len )
				{
					minLen = len;
					desired_cellHandle = *c_it;
				}
			}
		}

		//find the half face handle
		OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle;
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(desired_cellHandle);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		OpenVolumeMesh::Geometry::Vec3d faceCenter;
		int faceVertexCount =0;
		for(unsigned int i=0;i<hff_vec.size();++i)
		{
			hfv_it = mesh_.hfv_iter(hff_vec[i]);

			faceCenter = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
			faceVertexCount = 0;
			for( hfv_it; hfv_it; ++hfv_it )
			{
				faceCenter += mesh_.vertex(*hfv_it);
				faceVertexCount++;
			}
			faceCenter /= faceVertexCount;

			faceCenter -= resultVolumeFaceP;
			if( faceCenter.length() < minLen )
			{
				desired_HalffaceHnadle = hff_vec[i];
				break;
			}
		}

		picked_face = mesh_.face_handle(desired_HalffaceHnadle);
	}
}

void InteractiveWidget::draw_flipped_tet()
{
	std::vector<OpenVolumeMesh::CellHandle> negative_cell;
	get_negative_volume(&mesh_, negative_cell);
	glLineWidth( 2.0 );
	for(unsigned i=0;i<negative_cell.size();++i)
	{
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(negative_cell[i]);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hfh_Vec = cell.halffaces();

		std::vector<OpenVolumeMesh::Geometry::Vec3d> v; v.reserve(4);
		for(unsigned int j=0;j<hfh_Vec.size();++j)
		{
			OpenVolumeMesh::HalfFaceVertexIter fhv_it = mesh_.hfv_iter(hfh_Vec[j]);
			glColor3f(1.0,0.25,0.0);
			glBegin(GL_TRIANGLES);
			for(fhv_it;fhv_it;++fhv_it)
			{
				glVertex3dv(mesh_.vertex(*fhv_it).data());
			}
			glEnd();
		}

		for(OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(negative_cell[i]); cv_it; ++cv_it )
		{
			v.push_back( mesh_.vertex( cv_it.cur_handle() ) );
		}
		glColor3f(1.0,1.0,1.0);
		glBegin(GL_LINES);
		for( unsigned int i=0; i<3; ++i )
		{
			for(unsigned j=i+1; j<4;++j)
			{
				glVertex3dv(v[i].data());
				glVertex3dv(v[j].data());
			}
		}
		glEnd();

		glPointSize(7);
		glColor3f( 0.75, 0.75, 0.0 );
		glBegin(GL_POINTS);
		for( unsigned int i=0; i<4; ++i )
		{
			glVertex3dv(v[i].data());
		}
		glEnd();
	}
}

void InteractiveWidget::draw_scene(int drawmode)
{
	if (!mesh_.n_vertices()) { return; }

#if 0
	glDisable(GL_LIGHTING);

	double r = (bbMin-bbMax).norm()*0.5;
	OpenMesh::Vec3d c = (bbMin+ bbMax)*0.5;
	OpenMesh::Vec3d x(1.0*r,0,0); OpenMesh::Vec3d y(0,1.0*r,0); OpenMesh::Vec3d z(0,0,1.0 *r);
	glDisable(GL_LIGHTING);
	glLineWidth(2.0);
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3dv( c.data() );
	glVertex3dv( (c + x).data() );
	glVertex3dv(c.data());
	glVertex3dv((c - x).data());
	glEnd();

	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3dv( c.data() );
	glVertex3dv( (c + y).data() );
	glVertex3dv(c.data());
	glVertex3dv((c - y).data());
	glEnd();

	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3dv( c.data() );
	glVertex3dv( (c + z).data() );
	glVertex3dv(c.data());
	glVertex3dv((c - z).data());
	glEnd();

	if (picked_vertex != VolumeMesh::InvalidVertexHandle)
	{
		glColor3f(1.0, 0.5, 0.0);
		glPointSize(12);
		glBegin(GL_POINTS);
		glVertex3dv(mesh_.vertex(picked_vertex).data());
		glEnd();
		glPointSize(1);
	}
#endif
	

	if( HaveFunction_TO )
	{
		emit(draw_selected_Sheet_signal());
		emit(draw_selected_Column_signal());
		emit(draw_Doublet_signal());
		emit(draw_adjColumn_signal());
		emit(draw_Column_split_signal());

		emit draw_scene_hexSim_signal();
	}

	//glColor3f(0, 0, 0);
	//renderText(width() * 36 / 100, height() * 1 / 20, iso_d_str, tempfont);

	if (draw_main_scene)
	{
		MeshViewerWidget::draw_scene(draw_mode_);
	}

	if( HaveFunction_GP )
	{
		emit draw_for_GP();
	}
	
	if( HaveFunction_TS )
	{
		emit draw_for_TS();
	}

	if( HaveFunction_TM )
	{
		emit draw_for_TM();
	}

	if (HaveFunction_GMD)
	{
		emit draw_for_GMD();
	}

	if (HaveFunction_MLD)
	{
		emit draw_for_MLD();
	}

	if( draw_flipped_tet_ok && meshMode() == TETRAHEDRAL )
	{
		draw_flipped_tet();
	}
}

