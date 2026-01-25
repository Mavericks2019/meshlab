#ifndef MESHPROCESSING_QGLVIEWERWIDGET_HH
#define MESHPROCESSING_QGLVIEWERWIDGET_HH

#include <OpenVolumeMesh/Geometry/VectorT.hh>
#include "MeshDefinition.h"
#include "OpenGLHeaders.h"

class QGLViewerWidget : public QGLWidget 
{
	Q_OBJECT
public:
	// Constructors.
	QGLViewerWidget( QWidget* _parent=0 );
	QGLViewerWidget( QGLFormat& _fmt, QWidget* _parent=0 );
	// Destructor.
	virtual ~QGLViewerWidget();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;
private:
	void init(void);
public:
	/* Sets the center and size of the whole scene. 
	   The _center is used as fixpoint for rotations and for adjusting
	   the camera/viewer (see view_all()). */
	void set_scene_pos(const OpenVolumeMesh::Geometry::Vec3d& _center, double _radius);  

	/* view the whole scene: the eye point is moved far enough from the
	   center so that the whole scene is visible. */
	void view_all();

	float radius() const { return radius_; }
	const OpenVolumeMesh::Geometry::Vec3d& center() const { return center_; }

	const GLdouble* modelview_matrix() const  { return modelview_matrix_;  }
	const GLdouble* projection_matrix() const { return projection_matrix_; }

	float fovy() const { return 45.0f; }
public:
	// draw modes.
	enum { WIRE_FRAME=0, SOLID_FLAT, FLAT_LINES, ALL_LINES, ALL_POINTS, ALL_POLYHEDRALS, SINGULARITY, VOLUME_TEXTURE, N_DRAW_MODES };
	void setDrawMode(int dm) 
	{ 
		draw_mode_ = dm;
		emit setDrawMode_signal(dm);
		updateGL(); 
	}
	int draw_mode() const { return draw_mode_; }
signals:
	void setDrawMode_signal(int);

protected:
	// draw the scene: will be called by the painGL() method.
	virtual void draw_scene(int drawmode);
	
	void setDefaultMaterial(void);
	void setDefaultLight(void);
	
private: // inherited
	// initialize OpenGL states (triggered by Qt)
	void initializeGL();
	// draw the scene (triggered by Qt)
	void paintGL();
	// handle resize events (triggered by Qt)
	void resizeGL(int w, int h);

public slots:
	void UpdateGL_Manual_slot()
	{
		updateGL();
	}
	void set_scene_pos_slot(OpenVolumeMesh::Geometry::Vec3d bb_min_, OpenVolumeMesh::Geometry::Vec3d bb_max_)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix_);
		bbMin[0] = bb_min_[0]; bbMin[1] = bb_min_[1]; bbMin[2] = bb_min_[2];
		bbMax[0] = bb_max_[0]; bbMax[1] = bb_max_[1]; bbMax[2] = bb_max_[2];
		set_scene_pos(0.5*(bb_min_ + bb_max_), (bb_min_ - bb_max_).norm()*0.5);
		//printf("%f %f %f\n", _center[0], _center[1], _center[2]);
	}
signals:
	void change_to_insert_mode();

protected:
	// Qt mouse events
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void wheelEvent(QWheelEvent*);
	virtual void keyPressEvent(QKeyEvent*);
private:
	// updates projection matrix
	void update_projection_matrix();
	// translate the scene and update modelview matrix
	void translate(const OpenVolumeMesh::Geometry::Vec3d& _trans);
	// rotate the scene (around its center) and update modelview matrix
	void rotate(const OpenVolumeMesh::Geometry::Vec3d& _axis, double _angle);

	void translation(QPoint p);
	void rotation(QPoint p);
protected:
	int draw_mode_;
	int mouse_mode_;
	OpenVolumeMesh::Geometry::Vec3d  center_;
	double            radius_;
	SurfaceMesh::Point bbMin;//boudning box
	SurfaceMesh::Point bbMax;

	GLdouble projection_matrix_[16];
	GLdouble modelview_matrix_[16];

	// virtual trackball: map 2D screen point to unit sphere
	bool map_to_sphere(const QPoint& _point, OpenVolumeMesh::Geometry::Vec3d& _result);
	
	QPoint           last_point_2D_;
	OpenVolumeMesh::Geometry::Vec3d  last_point_3D_;
	bool             last_point_ok_;

};

#endif // MESHPROCESSING_QGLVIEWERWIDGET_HH
