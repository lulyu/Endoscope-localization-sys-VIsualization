#include <gl/glut.h>
#include <gl/GL.h>
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>

// 窗口宽度、高度
#define WIDTH 400 
#define HEIGHT 400
// 摄像机放置的旋转角、z轴位置
static GLfloat angle = 0.0f;
static GLfloat zCamera = -50;
// 键盘控制的旋转方向
static GLfloat xRot = 0.0f;
static GLfloat yRot = 0.0f;
// 近似圆管的分段数
static int segments = 20;
// 圆管的直径
static int radius = 10; 
static int outRadius = 20;
static float pi = 3.1415926;
// 三维点和三位矢量的结构体
typedef struct point3d
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
	point3d *next;
	point3d *last;
};
typedef struct vector3d
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

// 记录轨迹点坐标的数组的指针
point3d *pTrack;
// 标记是否在进行内镜拔出过程
static int flagIfPullOut = 0;
// 轨迹点个数
static GLint trackLength = 0;
// 当前累计有效数据点个数
static GLint nPoints;
static GLint nPointsNow = 0; // 画动画用的

// 三维向量a绕轴n转theta弧度
vector3d rotateVector(vector3d a, vector3d n, GLfloat theta) 
{
	vector3d b;
	b.x = a.x*(cos(theta)+(1-cos(theta))*n.x*n.x) + a.y*((1-cos(theta))*n.x*n.y-sin(theta)*n.z) + a.z*((1-cos(theta))*n.x*n.z+sin(theta)*n.y);
	b.y = a.x*((1-cos(theta))*n.y*n.x+sin(theta)*n.z) + a.y*(cos(theta)+(1-cos(theta))*n.y*n.y) + a.z*((1-cos(theta))*n.y*n.z-sin(theta)*n.x);
	b.z = a.x*((1-cos(theta))*n.z*n.x-sin(theta)*n.y) + a.y*((1-cos(theta))*n.z*n.y+sin(theta)*n.x) + a.z*(cos(theta)+(1-cos(theta))*n.z*n.z);
	return b;
}
// 坐标和一个向量相加得到新坐标
point3d addVector(point3d a, vector3d b)
{
	a.x=a.x+b.x;
	a.y=a.y+b.y;
	a.z=a.z+b.z;
	return a;
}
// 单位化一个向量
vector3d normalizeVector(vector3d a) 
{
	double length;
	length = sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
	a.x=a.x/length;
	a.y=a.y/length;
	a.z=a.z/length;
	return a;
}
// 求三角形平面的法向量
vector3d normofTriangle(point3d pointA, point3d pointB, point3d pointC)
{
	vector3d ab, bc, norm;
	ab.x = pointA.x - pointB.x;
	ab.y = pointA.y - pointB.y;
	ab.z = pointA.z - pointB.z;
	bc.x = pointB.x - pointC.x;
	bc.y = pointB.y - pointC.y;
	bc.z = pointB.z - pointC.z;
	norm.x = (ab.y * bc.z) - (ab.z * bc.y);
	norm.y = -((ab.x * bc.z) - (ab.z * bc.x));
	norm.z = (ab.x * bc.y) - (ab.y * bc.x);
	norm = normalizeVector(norm);
	return norm;
}
// 求向量的长度
double lengthVector(vector3d a)
{
	return sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
}

// 定义tube材质
void tubeMaterial()
{
	// 定义材质
	GLfloat tube_mat_ambient[] = {0.20f, 0.09f, 0.12f, 0.3f}; 
	GLfloat tube_mat_diffuse[] = {0.26f, 0.0f, 0.52f, 0.3f};
	GLfloat tube_mat_specular[] = {1.0f, 1.0f, 1.0f, 0.3f};
	GLfloat tube_mat_emission[] = {0.0f, 0.0f, 0.0f, 0.0f}; 
	GLfloat tube_mat_shininess = 40.0f; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tube_mat_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tube_mat_diffuse); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tube_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, tube_mat_emission); 
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, tube_mat_shininess);
}
// 定义tube前端的材质
void tubeTipMaterial()
{
	// 定义材质
	GLfloat tube_mat_ambient[] = {0.2f, 0.2f, 0.2f, 0.1f}; 
	GLfloat tube_mat_diffuse[] = {0.3f, 0.3f, 0.3f, 0.1f};
	GLfloat tube_mat_specular[] = {1.0f, 1.0f, 1.0f, 0.1f};
	GLfloat tube_mat_emission[] = {0.0f, 0.0f, 0.0f, 0.0f}; 
	GLfloat tube_mat_shininess = 100.0f; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tube_mat_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tube_mat_diffuse); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tube_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, tube_mat_emission); 
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, tube_mat_shininess);
}
// 定义tube头部的材质
void tubeTipSphereMaterial()
{
	// 定义材质
	GLfloat tube_mat_ambient[] = {0.3f, 0.0f, 0.0f, 1.0f}; 
	GLfloat tube_mat_diffuse[] = {0.3f, 0.0f, 0.0f, 1.0f};
	GLfloat tube_mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat tube_mat_emission[] = {0.5f, 0.0f, 0.0f, 1.0f}; 
	GLfloat tube_mat_shininess = 128.0f; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tube_mat_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tube_mat_diffuse); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tube_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, tube_mat_emission); 
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, tube_mat_shininess);
}
// 定义轨迹的材质
void trackMaterial()
{
	GLfloat track_mat_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
	GLfloat track_mat_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat track_mat_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};
	GLfloat track_mat_emission[] = {0.0f, 0.0f, 0.0f, 1.0f}; 
	GLfloat track_mat_shininess = 0.0f; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, track_mat_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, track_mat_diffuse); 
	//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, track_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, track_mat_emission); 
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, track_mat_shininess);

	glLineWidth(2.0);
}
// 画每一小节tube（FLAT）
void drawTubeSec(point3d point1, point3d point2, GLfloat r, vector3d n1, vector3d n2)
{
	int i;
	// 计算起始的两点，用n1和n2叉乘某一单位向量再*r来得到
	vector3d a1,a2;
	if((n1.x<1)&&(n2.x<1)) // 当n1 n2中没有(1,0,0)就和(1,0,0)叉乘。。。有的话换一个叉乘
	{
		a1.x=0; a1.y=n1.z; a1.z=-n1.y; // 这俩是从当前曲线上的点指向小三角形的顶点的向量
		a2.x=0; a2.y=n2.z; a2.z=-n2.y; // 每一个平面的法向量先用相邻两点相对曲线上的点的向量的平均值代替吧。。。
		a1 = normalizeVector(a1);
		a2 = normalizeVector(a2);
		a1.x=r*a1.x;a1.y=r*a1.y;a1.z=r*a1.z;
		a2.x=r*a2.x;a2.y=r*a2.y;a2.z=r*a2.z;
	}
	else if ((n1.y<1)&&(n2.y<1)) // 和(0,1,0)叉乘好了
	{
		a1.x=-n1.z; a1.y=0; a1.z=n1.x;
		a2.x=-n2.z; a2.y=0; a2.z=n2.x; 
		a1 = normalizeVector(a1);
		a2 = normalizeVector(a2);
		a1.x=r*a1.x;a1.y=r*a1.y;a1.z=r*a1.z;
		a2.x=r*a2.x;a2.y=r*a2.y;a2.z=r*a2.z;
	}
	vector3d b1,b2;
	b1 = rotateVector(a1,n1,2*pi/segments); 
	b2 = rotateVector(a2,n2,2*pi/segments); 
	point3d v1,v2,v3,v4; // 每两个三角形单元用到的的四个顶点坐标
	vector3d norm;

	glBegin(GL_TRIANGLE_STRIP);
		v1=addVector(point1,a1);
		v2=addVector(point2,a2);
		v3=addVector(point1,b1);
		v4=addVector(point2,b2);
		glVertex3f(v1.x,v1.y,v1.z);
		norm = normofTriangle(v2,v1,v3); // 用normofTriangle求每一个三角形面片的法线
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v2.x,v2.y,v2.z);
		glVertex3f(v3.x,v3.y,v3.z);
		norm = normofTriangle(v4,v2,v3);
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v4.x,v4.y,v4.z);

		for(i=0; i<segments-1; ++i) 
		{  
			a1.y=b1.y;a1.z=b1.z;a1.x=b1.x;//之前的上一个点变成了下一个点
			a2.y=b2.y;a2.z=b2.z;a2.x=b2.x;
			b1 = rotateVector(a1,n1,2*pi/segments);//下一个角度的矢量
			b2 = rotateVector(a2,n2,2*pi/segments); 
			v1=addVector(point1,a1);
			v2=addVector(point2,a2);
			v3=addVector(point1,b1);
			v4=addVector(point2,b2);
			glVertex3f(v1.x,v1.y,v1.z);
			norm = normofTriangle(v2,v1,v3);
			glNormal3f(norm.x,norm.y,norm.z);
			glVertex3f(v2.x,v2.y,v2.z);
			glVertex3f(v3.x,v3.y,v3.z);
			norm = normofTriangle(v4,v2,v3);
			glNormal3f(norm.x,norm.y,norm.z);//第二个三角形用了2个2环上的点所以这么近似法向量
			glVertex3f(v4.x,v4.y,v4.z);
		} 
	glEnd();
}
// 画每一小节tube（SMOOTH）
void drawTubeSecSmooth(point3d point1, point3d point2, GLfloat r, vector3d n1, vector3d n2)
{
	int i;
	// 计算起始的两点，用n1和n2叉乘某一单位向量再*r来得到
	vector3d a1,a2;
	if((n1.x<1)&&(n2.x<1)) // 当n1 n2中没有(1,0,0)就和(1,0,0)叉乘。。。有的话换一个叉乘
	{
		a1.x=0; a1.y=n1.z; a1.z=-n1.y; // 这俩是从当前曲线上的点指向小三角形的顶点的向量
		a2.x=0; a2.y=n2.z; a2.z=-n2.y; // 每一个平面的法向量先用相邻两点相对曲线上的点的向量的平均值代替吧。。。
		a1 = normalizeVector(a1);
		a2 = normalizeVector(a2);
		a1.x=r*a1.x;a1.y=r*a1.y;a1.z=r*a1.z;
		a2.x=r*a2.x;a2.y=r*a2.y;a2.z=r*a2.z;
	}
	else if ((n1.y<1)&&(n2.y<1)) // 和(0,1,0)叉乘好了
	{
		a1.x=-n1.z; a1.y=0; a1.z=n1.x;
		a2.x=-n2.z; a2.y=0; a2.z=n2.x; 
		a1 = normalizeVector(a1);
		a2 = normalizeVector(a2);
		a1.x=r*a1.x;a1.y=r*a1.y;a1.z=r*a1.z;
		a2.x=r*a2.x;a2.y=r*a2.y;a2.z=r*a2.z;
	}
	vector3d b1,b2;
	b1 = rotateVector(a1,n1,2*pi/segments); 
	b2 = rotateVector(a2,n2,2*pi/segments); 
	//printf("l(a1)=%lf  l(a2)=%lf\n", lengthVector(a1),lengthVector(a2));
	//printf("l(b1)=%lf  l(b2)=%lf\n", lengthVector(b1),lengthVector(b2));
	//printf("l(n1)=%lf  l(n2)=%lf\n", lengthVector(n1),lengthVector(n2));
	//printf("l(r)=%lf\n", r);
	point3d v1,v2,v3,v4; // 每两个三角形单元用到的的四个顶点坐标
	vector3d norm;

	glBegin(GL_TRIANGLE_STRIP);
		v1=addVector(point1,a1);
		v2=addVector(point2,a2);
		v3=addVector(point1,b1);
		v4=addVector(point2,b2);
		norm = normalizeVector(a1);
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v1.x,v1.y,v1.z);
		norm = normalizeVector(a2);
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v2.x,v2.y,v2.z);
		norm = normalizeVector(b1);
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v3.x,v3.y,v3.z);
		norm = normalizeVector(b2);
		glNormal3f(norm.x,norm.y,norm.z);
		glVertex3f(v4.x,v4.y,v4.z);

		for(i=0; i<segments-1; ++i) 
		{  
			a1.y=b1.y;a1.z=b1.z;a1.x=b1.x;//之前的上一个点变成了下一个点
			a2.y=b2.y;a2.z=b2.z;a2.x=b2.x;
			b1 = rotateVector(a1,n1,2*pi/segments);//下一个角度的矢量
			b2 = rotateVector(a2,n2,2*pi/segments); 
			//printf("l(a1)=%lf  l(a2)=%lf\n", lengthVector(a1),lengthVector(a2));
			//printf("l(b1)=%lf  l(b2)=%lf\n", lengthVector(b1),lengthVector(b2));
			//printf("l(n1)=%lf  l(n2)=%lf\n", lengthVector(n1),lengthVector(n2));
			//printf("l(r)=%lf\n", r);
			v1=addVector(point1,a1);
			v2=addVector(point2,a2);
			v3=addVector(point1,b1);
			v4=addVector(point2,b2);
			norm = normalizeVector(a1);
			glNormal3f(norm.x,norm.y,norm.z);
			glVertex3f(v1.x,v1.y,v1.z);
			norm = normalizeVector(a2);
			glNormal3f(norm.x,norm.y,norm.z);
			glVertex3f(v2.x,v2.y,v2.z);
			norm = normalizeVector(b1);
			glNormal3f(norm.x,norm.y,norm.z);
			glVertex3f(v3.x,v3.y,v3.z);
			norm = normalizeVector(b2);
			glNormal3f(norm.x,norm.y,norm.z);
			glVertex3f(v4.x,v4.y,v4.z);
		} 
	glEnd();
}
// 画管底
void drawTubeCover(point3d point, vector3d n)
{
	int i;
	vector3d a;
	if(n.x<1) // 当n不是(1,0,0)就和(1,0,0)叉乘。。。有的话换一个叉乘
	{
		a.x=0; a.y=n.z; a.z=-n.y; 
		a = normalizeVector(a);
		a.x=radius*a.x;a.y=radius*a.y;a.z=radius*a.z;
	}
	else 
	{
		a.x=-n.z; a.y=0; a.z=n.x;
		a = normalizeVector(a);
		a.x=radius*a.x;a.y=radius*a.y;a.z=radius*a.z;
	}
	glBegin(GL_POLYGON);
	glNormal3f(n.x,n.y,n.z);
	for(i=0; i<segments; i++) 
	{ 
		glVertex3f(point.x+a.x, point.y+a.y, point.z+a.z); 
		a = rotateVector(a,n,2*pi/segments);
	} 
	glEnd();
}
// 画管头部的红球（？）
void drawTubeTip(point3d point)
{
	glTranslatef(point.x, point.y, point.z);
	glutSolidSphere(radius/3, 10, 10);
}
// 画每一段轨迹虚线
void drawTrackSec(point3d point1, point3d point2, GLfloat r, vector3d n1, vector3d n2)
{
	// 计算起始的两点，用n1和n2叉乘某一单位向量再*r来得到
	vector3d a1,a2;
	vector3d nview;
	nview.x = -tan(yRot/180*pi);
	nview.y = tan(xRot/180*pi);
	nview.z = 1;
	nview = normalizeVector(nview);

	if((n1.x<1)&&(n2.x<1)) // 与(0,0,1)叉乘
	{
		//a1.x=-n1.y; a1.y=n1.x; a1.z=0; // 这俩是从当前曲线上的点指向小三角形的顶点的向量
		//a2.x=-n2.y; a2.y=n2.x; a2.z=0; // 每一个平面的法向量先用相邻两点相对曲线上的点的向量的平均值代替吧。。。
		a1.x = n1.z*nview.y - n1.y*nview.z;
		a1.y = n1.x*nview.z - n1.z*nview.x;
		a1.z = n1.y*nview.x - n1.x*nview.y;
		a2.x = n2.z*nview.y - n2.y*nview.z;
		a2.y = n2.x*nview.z - n2.z*nview.x;
		a2.z = n2.y*nview.x - n2.x*nview.y;
		a1 = normalizeVector(a1);
		a2 = normalizeVector(a2);
		a1.x=r*a1.x;a1.y=r*a1.y;a1.z=r*a1.z;
		a2.x=r*a2.x;a2.y=r*a2.y;a2.z=r*a2.z;
	}
	else
	{
		a1.x=0; a1.y=0; a1.z=0;
		a2.x=0; a2.y=0; a2.z=0;
	}
	point3d v1,v2,v3,v4; // 每两个三角形单元用到的的四个顶点坐标

	glBegin(GL_LINES);
		v1=addVector(point1,a1);
		v2=addVector(point2,a2);
		a1.x=-a1.x; a1.y=-a1.y; a1.z=-a1.z;
		a2.x=-a2.x; a2.y=-a2.y; a2.z=-a2.z;
		v3=addVector(point1,a1);
		v4=addVector(point2,a2);
		glVertex3f(v1.x,v1.y,v1.z);
		glVertex3f(v2.x,v2.y,v2.z);
		glVertex3f(v3.x,v3.y,v3.z);
		glVertex3f(v4.x,v4.y,v4.z);
	glEnd();

	//double temp;
	//temp = nview.x*a1.x + nview.y*a1.y + nview.z*a1.z;
	//printf("%lf\n", temp);
}

// 画完整的tube
void drawTube(point3d *ppoints) // 输入点的信息和当前点的个数, 画出完整的tube
{
	//加载管道材质
	tubeMaterial();
	int i;
	vector3d n1,n2;
	if (nPoints==2) // 只有两个点的情况
	{
		n1.x=ppoints[1].x-ppoints[0].x;
		n1.y=ppoints[1].y-ppoints[0].y;
		n1.z=ppoints[1].z-ppoints[0].z;
		n1 = normalizeVector(n1);
		n2=n1;
		drawTubeSecSmooth(ppoints[0], ppoints[1], radius, n1, n2);
	}
	else  // 多于两个点的情况
	{
		for(i=0;i<nPoints-1;i++)
		{ 
			if (i==0)
			{
				n1.x=ppoints[1].x-ppoints[0].x;
				n1.y=ppoints[1].y-ppoints[0].y;
				n1.z=ppoints[1].z-ppoints[0].z;
				n2.x=ppoints[i+2].x-ppoints[i].x;
				n2.y=ppoints[i+2].y-ppoints[i].y;
				n2.z=ppoints[i+2].z-ppoints[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			else if (i==nPoints-2)
			{
				n1.x=ppoints[i+1].x-ppoints[i-1].x;
				n1.y=ppoints[i+1].y-ppoints[i-1].y;
				n1.z=ppoints[i+1].z-ppoints[i-1].z;
				n2.x=ppoints[nPoints-1].x-ppoints[nPoints-2].x;
				n2.y=ppoints[nPoints-1].y-ppoints[nPoints-2].y;
				n2.z=ppoints[nPoints-1].z-ppoints[nPoints-2].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			else
			{
				n1.x=ppoints[i+1].x-ppoints[i-1].x;
				n1.y=ppoints[i+1].y-ppoints[i-1].y;
				n1.z=ppoints[i+1].z-ppoints[i-1].z;
				n2.x=ppoints[i+2].x-ppoints[i].x;
				n2.y=ppoints[i+2].y-ppoints[i].y;
				n2.z=ppoints[i+2].z-ppoints[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			drawTubeSecSmooth(ppoints[i], ppoints[i+1], radius, n1, n2);
		}
	}
}
// 动态画完整的tube
void drawTubeAnime(point3d *ppoints) // 输入点的信息和当前点的个数, 画出完整的tube
{
	//加载管道材质
	tubeMaterial();
	int i;
	vector3d n1,n2;
	if (nPointsNow==2) // 只有两个点的情况
	{
		n1.x=ppoints[1].x-ppoints[0].x;
		n1.y=ppoints[1].y-ppoints[0].y;
		n1.z=ppoints[1].z-ppoints[0].z;
		n1 = normalizeVector(n1);
		n2=n1;
		drawTubeCover(ppoints[0], n1);
		drawTubeSecSmooth(ppoints[0], ppoints[1], radius, n1, n2);
	}
	else  // 多于两个点的情况
	{
		for(i=0;i<nPointsNow-1;i++)
		{ 
			if (i==0)
			{
				n1.x=ppoints[1].x-ppoints[0].x;
				n1.y=ppoints[1].y-ppoints[0].y;
				n1.z=ppoints[1].z-ppoints[0].z;
				n2.x=ppoints[i+2].x-ppoints[i].x;
				n2.y=ppoints[i+2].y-ppoints[i].y;
				n2.z=ppoints[i+2].z-ppoints[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
				// 画管底
				drawTubeCover(ppoints[0], n1);
			}
			else if (i==nPointsNow-2)
			{
				n1.x=ppoints[i+1].x-ppoints[i-1].x;
				n1.y=ppoints[i+1].y-ppoints[i-1].y;
				n1.z=ppoints[i+1].z-ppoints[i-1].z;
				n2.x=ppoints[nPointsNow-1].x-ppoints[nPointsNow-2].x;
				n2.y=ppoints[nPointsNow-1].y-ppoints[nPointsNow-2].y;
				n2.z=ppoints[nPointsNow-1].z-ppoints[nPointsNow-2].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			else
			{
				// 离管头最近的一节换成灰色
				if (i==nPointsNow-3) tubeTipMaterial();
				n1.x=ppoints[i+1].x-ppoints[i-1].x;
				n1.y=ppoints[i+1].y-ppoints[i-1].y;
				n1.z=ppoints[i+1].z-ppoints[i-1].z;
				n2.x=ppoints[i+2].x-ppoints[i].x;
				n2.y=ppoints[i+2].y-ppoints[i].y;
				n2.z=ppoints[i+2].z-ppoints[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			drawTubeSecSmooth(ppoints[i], ppoints[i+1], radius, n1, n2);
			if(i==nPointsNow-2) 
			{
				// 画管头的红球
				tubeTipSphereMaterial();
				drawTubeTip(ppoints[nPointsNow-1]);
			}
		}
	}
}
// 测试用一段圆管
void drawTest() 
{
	int i;
	// 画管子
	glBegin(GL_TRIANGLE_STRIP);
	for(i=0; i<=segments; ++i) 
	{ 
		GLfloat tmp = 2 * pi * i / segments;
		GLfloat tmp2 = 2 * pi * (i+1/2) /segments;
		glNormal3f(cos(tmp2),sin(tmp2),0);
		glVertex3f(100*cos(tmp), 100*sin(tmp),0); 
		glVertex3f(100*cos(tmp2), 100*sin(tmp2), 500);
	} 
	glEnd();
	// 画盖子？
	glBegin(GL_POLYGON);
	glNormal3f(0,0,-1);
	for(i=0; i<=segments; ++i) 
	{ 
		GLfloat tmp = 2 * pi * i / segments;
		glVertex3f(100*cos(tmp), 100*sin(tmp),0); 
	} 
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(0,0,1);
	for(i=0; i<=segments; ++i) 
	{ 
		GLfloat tmp = 2 * pi * (i+1/2) / segments;
		glVertex3f(100*cos(tmp), 100*sin(tmp),500); 
	} 
	glEnd();
}
// 画轨迹
void drawTrack(point3d *pTrack)
{
	// 加载轨迹材质
	trackMaterial();
	int i;
	vector3d n1,n2;
	if (trackLength==2) // 只有两个点的情况
	{
		n1.x=pTrack[1].x-pTrack[0].x;
		n1.y=pTrack[1].y-pTrack[0].y;
		n1.z=pTrack[1].z-pTrack[0].z;
		n1 = normalizeVector(n1);
		n2=n1;
		drawTrackSec(pTrack[0], pTrack[1], outRadius, n1, n2);
	}
	else  // 多于两个点的情况
	{
		for(i=0;i<trackLength-1;i=i+2)
		{ 
			if (i==0)
			{
				n1.x=pTrack[1].x-pTrack[0].x;
				n1.y=pTrack[1].y-pTrack[0].y;
				n1.z=pTrack[1].z-pTrack[0].z;
				n2.x=pTrack[i+2].x-pTrack[i].x;
				n2.y=pTrack[i+2].y-pTrack[i].y;
				n2.z=pTrack[i+2].z-pTrack[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			else if (i==trackLength-2)
			{
				n1.x=pTrack[i+1].x-pTrack[i-1].x;
				n1.y=pTrack[i+1].y-pTrack[i-1].y;
				n1.z=pTrack[i+1].z-pTrack[i-1].z;
				n2.x=pTrack[trackLength-1].x-pTrack[trackLength-2].x;
				n2.y=pTrack[trackLength-1].y-pTrack[trackLength-2].y;
				n2.z=pTrack[trackLength-1].z-pTrack[trackLength-2].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			else
			{
				n1.x=pTrack[i+1].x-pTrack[i-1].x;
				n1.y=pTrack[i+1].y-pTrack[i-1].y;
				n1.z=pTrack[i+1].z-pTrack[i-1].z;
				n2.x=pTrack[i+2].x-pTrack[i].x;
				n2.y=pTrack[i+2].y-pTrack[i].y;
				n2.z=pTrack[i+2].z-pTrack[i].z;
				n1 = normalizeVector(n1);
				n2 = normalizeVector(n2);
			}
			drawTrackSec(pTrack[i], pTrack[i+1], outRadius, n1, n2);
		}
	}
}

// 从文件读入点的信息
point3d *inputPoints() 
{
	FILE *fp;
	fp = fopen("input.txt", "r");
	if(fp==NULL) //如果失败了
	{
		printf("错误！");
		exit(1); //中止程序
	}
	fscanf(fp, "%d\n", &flagIfPullOut); // 读入是否拔出的标记
	fscanf(fp, "%d\n", &nPoints); // 读入节点数
	point3d *ppoints = new point3d[nPoints];
	int i;
	if(flagIfPullOut==0)
		{
			trackLength = nPoints;
			delete pTrack;
			pTrack = new point3d[trackLength]; // pTrack指向记录轨迹点坐标的数组
		}
	for(i=0;i<nPoints;i++)
	{
		fscanf(fp, "%f %f %f\n", &ppoints[i].x, &ppoints[i].y, &ppoints[i].z);
		pTrack[i].x=ppoints[i].x;
		pTrack[i].y=ppoints[i].y;
		pTrack[i].z=ppoints[i].z;
	}
	fclose(fp);
	return ppoints;
}
// 画背景（好像没什么用）
void drawBackground()
{
	// 定义材质
	GLfloat bg_mat_ambient[] = {1.0f, 0.3f, 0.3f, 0.3f}; 
	GLfloat bg_mat_diffuse[] = {1.0f, 1.0f, 1.0f, 0.3f};
	GLfloat bg_mat_specular[] = {0.0f, 0.0f, 0.0f, 0.3f};
	GLfloat bg_mat_emission[] = {1.0f, 0.0f, 0.0f, 0.1f}; 
	GLfloat bg_mat_shininess = 100.0f; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bg_mat_ambient); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bg_mat_diffuse); 
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, bg_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, bg_mat_emission); 
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, bg_mat_shininess);
	//画个背景平板
	glBegin(GL_POLYGON);
	glNormal3f(0,0,1);
	glVertex3f(-3000,-3000,500);
	glVertex3f(-3000,3000,500);
	glVertex3f(3000,3000,500);
	glVertex3f(3000,-3000,500);
	glEnd();
}
// 初始化光照
void iniLight(void)
{
	//定义光源
	//背景光
	GLfloat sun_light_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
	GLfloat sun_light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
	GLfloat sun_light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, sun_light_ambient); 
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, sun_light_diffuse);
	//glLightfv(GL_LIGHT0, GL_SPECULAR, sun_light_specular);

	//LIGHT1
	GLfloat light1_light_position[] = {0, 1000, -200, 1.0f};
	//GLfloat light1_light_ambient[] = {1.0f, 1.0f, 1.0f, 0.2f}; 
	GLfloat light1_light_diffuse[] = {1.0f, 1.0f, 1.0f, 0.0f}; 
	glLightfv(GL_LIGHT1, GL_POSITION, light1_light_position);
	//glLightfv(GL_LIGHT1, GL_AMBIENT, light1_light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_light_diffuse);

	//LIGHT2
	GLfloat light2_light_position[] = {0, 300, -500, 1.0f};
	//GLfloat light2_light_ambient[] = {1.0f, 1.0f, 1.0f, 0.02f}; 
	GLfloat light2_light_diffuse[] = {0.0f, 1.0f, 1.0f, 0.02f}; 
	glLightfv(GL_LIGHT2, GL_POSITION, light2_light_position);
	//glLightfv(GL_LIGHT2, GL_AMBIENT, light2_light_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_light_diffuse);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	//glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); // 两面光照效果
	glEnable(GL_LIGHTING); 
	glEnable(GL_DEPTH_TEST);
}
// 设置摄像机运行方式
void setCamera()
{
	//gluLookAt(300.0*cos(angle/180*pi), 300.0*sin(angle/180*pi), zCamera, 0.0, 0.0, 100.0, 1.0, 1.0, 1.0);
	gluLookAt(0.0, 300.0, -500, 0.0, 300.0, 0.0, 0, 1, 0);
	//gluLookAt(0,0,zCamera, 0,0,10000, cos(angle/180*pi),sin(angle/180*pi),0);
}
// 键盘控制的变换视角
void SpecialKeys(int key, int x, int y)
	{
	if(key == GLUT_KEY_UP)
		yRot-= 5.0f;

	if(key == GLUT_KEY_DOWN)
		yRot += 5.0f;

	if(key == GLUT_KEY_LEFT)
		xRot -= 5.0f;

	if(key == GLUT_KEY_RIGHT)
		xRot += 5.0f;

	if(key > 356.0f)
		xRot = 0.0f;

	if(key < -1.0f)
		xRot = 355.0f;

	if(key > 356.0f)
		yRot = 0.0f;

	if(key < -1.0f)
		yRot = 355.0f;

	// Refresh the Window
	glutPostRedisplay();
	}
// 绘图函数
void myDisplay(void)
{ 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	// 背景颜色
	//glClearColor(1.0f, 0.89f, 0.518f, 1.0f); // 淡黄色
	//glClearColor(0.74f, 0.99f, 0.79f, 1.0f); // 薄荷绿
	//glClearColor(0.75f, 0.75f, 0.75f, 1.0f); // 浅灰色
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // 浅灰色

	// 创建透视效果视图 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1.0, 1.0, 50000); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// 设置雾化效果参数
	GLfloat fog_color[4] = {0.0, 0.0, 0.0, 0.0};
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogf(GL_FOG_START, 300.0);
	glFogf(GL_FOG_END, 1000.0);

	// 摄像机运行方式
	setCamera();
	glClear(GL_COLOR_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 初始化光照情况。【为什么一定要插在这里才正常呢？…………我也不知道。
	iniLight();

	// 键盘控制的旋转
	glPushMatrix();
	glRotatef(xRot, 1.0f, 0.0f, 0.0f);
	glRotatef(yRot, 0.0f, 1.0f, 0.0f);

	// 绘制管道
	point3d *ppoints = inputPoints();
	drawTrack(pTrack);
	drawTubeAnime(ppoints);

	//drawTube(ppoints);
	// 画背景
	//drawBackground();
	//drawTest();

	glPopMatrix();
	glFlush();
	glutSwapBuffers();
}

void myIdle(void) 
{ 
	angle += 1.0f;
	if( angle >= 360.0f )
		angle = 0.0f;
	zCamera +=1.0f;
	if(nPointsNow<nPoints)
	{
		nPointsNow +=1;
		trackLength +=1;
	}
	Sleep(50);
	myDisplay();
}

int main(int argc, char* argv[])
{ 
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(WIDTH, HEIGHT); 
	glutCreateWindow("OpenGL光照演示");

	glutSpecialFunc(SpecialKeys);
	glutDisplayFunc(&myDisplay); 
	glutIdleFunc(&myIdle); 

	glutMainLoop();
	return 0; 
}