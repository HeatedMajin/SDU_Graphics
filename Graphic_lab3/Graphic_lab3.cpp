#include "stdafx.h"
#include <Eigen/Dense>
#include <list>
//#include <queue>
using namespace Eigen;

using namespace std;

struct HE_HalfEdge;
struct HE_Face;

struct HE_vert {
	float x;
	float y;
	float z;
	//HE_vert *vnew;

	Matrix4f Q;
	vector<HE_HalfEdge*> edges; //从该点出去的边

	int valid = -1;//点是不是有效 -1有效，其他为正确位置

	HE_vert() {}
	HE_vert(float x_,
		float y_,
		float z_) {
		x = x_;
		y = y_;
		z = z_;
	}

};
struct HE_HalfEdge {
	int v1;
	int v2;

	HE_HalfEdge *pre;
	HE_HalfEdge *next;
	HE_HalfEdge *oppo;

	HE_Face *right_face;

	//bool in_heap=false;//记录是不是在堆中
	bool valid = true;//是不是还有效
	float error;
	bool operator<(HE_HalfEdge e) {
		return error > e.error;
	}

	HE_HalfEdge() {}
	HE_HalfEdge(HE_HalfEdge &e) {
		v1 = e.v1;
		v2 = e.v2;
		pre = e.pre;
		next = e.next;
		oppo = e.oppo;
		right_face = e.right_face;
		error = e.error;
	}
};

struct HE_Face {


	int v[3];

	HE_HalfEdge *edges[3];

	float norm[3];

	bool erased = false;//面是不是被删掉了
	int index_in_faces;

	HE_Face() {}
	HE_Face(int v0, int v1, int v2) {
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
	}

	float canshu[4];//平面的abcd

	void calcNorm();
	void calcCan();
	void updateVert();
};

void fill_face(HE_Face *face);
void  fill_vert();

vector<HE_vert> verts;
map<pair<int,int>,HE_HalfEdge*> verts_to_edge;
vector<HE_Face> faces;

list<HE_HalfEdge*> sorted_edge;
struct sort_stru {
	bool operator()(const HE_HalfEdge* t1, const HE_HalfEdge* t2) {
		return t1->error<t2->error;    //会产生升序排序,若改为>,则变为降序
	}
};

int nverts = 0;
int nfaces = 0;

//priority_queue<HE_HalfEdge*> pio_queue;
float simple_rate = 0.8;//减少的比率 ： rate=0.4 ,faces from 100 to 60

void HE_Face::updateVert() {
	for (int i = 0; i < 3; i++)
	{
		HE_vert vt = verts[v[i]];
		if (-1 != vt.valid) {
			v[i] = vt.valid;
		}
	}
}
void HE_Face::calcNorm() {
	updateVert();
	// 计算法向量
	norm[0] = norm[1] = norm[2] = 0;

	HE_vert v1 = verts[v[2]];
	for (int i = 0; i < 3; i++) {
		HE_vert v2 = verts[v[i]];
		norm[0] += (v1.y - v2.y) * (v1.z + v2.z);
		norm[1] += (v1.z - v2.z) * (v1.x + v2.x);
		norm[2] += (v1.x - v2.x) * (v1.y + v2.y);
		v1 = v2;
	}

	// 单位化法向量
	float squared_normal_length = 0.0;
	squared_normal_length += norm[0] * norm[0];
	squared_normal_length += norm[1] * norm[1];
	squared_normal_length += norm[2] * norm[2];
	float normal_length = sqrt(squared_normal_length);
	if (normal_length > 1.0E-6) {
		norm[0] /= normal_length;
		norm[1] /= normal_length;
		norm[2] /= normal_length;
	}
}

auto getN() {
	const char* ac1 = "bunny1.off";
	const char* ac2 = "bunny2.off";
	const char* ac3 = "bunny3.off";
	const char* ac4 = "bunny4.off";
	if (simple_rate>0.89) {
		return ac4;
	}
	else if (simple_rate>0.79) {
		return ac3;
	}
	else if (simple_rate > 0.5) {
		return ac2;
	}
	else {
		return ac1;
	}
	
}

void HE_Face::calcCan() {
	updateVert();
	HE_vert v1 = verts[v[0]];
	HE_vert v2 = verts[v[1]];
	HE_vert v3 = verts[v[2]];
	float x1;
	float x2;
	float x3;
	float y1;
	float y2;
	float y3;
	float z1;
	float z2;
	float z3;
	x1 = v1.x;
	x2 = v2.x;
	x3 = v3.x;
	y1 = v1.y;
	y2 = v2.y;
	y3 = v3.y;
	z1 = v1.z;
	z2 = v2.z;
	z3 = v3.z;
	/*
a=y1z2-y1z3-y2z1+y2z3+y3z1-y3z2,
b=-x1z2+x1z3+x2z1-x2z3-x3z1+x3z2,
c=x1y2-x1y3-x2y1+x2y3+x3y1-x3y2,
d=-x1y2z3+x1y3z2+x2y1z3-x2y3z1-x3y1z2+x3y2z1
*/
	canshu[0] = y1 * z2 - y1 * z3 - y2 * z1 + y2 * z3 + y3 * z1 - y3 * z2;
	canshu[1] = -1 * x1*z2 + x1 * z3 + x2 * z1 - x2 * z3 - x3 * z1 + x3 * z2;
	canshu[2] = x1 * y2 - x1 * y3 - x2 * y1 + x2 * y3 + x3 * y1 - x3*y2;
	//canshu[3] = -1 * x1*y2*z3 + x1 * y3*z2 + x2 * y1*z3 - x2 * y3*z1 - x3 * y1*z2 + x3 * y2*z1;
	canshu[3] = -1 * (canshu[0] * x1 + canshu[1] * y1 + canshu[2] * z1);
	//double aas = (-1 * x1*y2*z3 + x1 * y3*z2 + x2 * y1*z3 - x2 * y3*z1 - x3 * y1*z2 + x3 * y2*z1);
	

	float fenmu = pow(pow(canshu[0], 2) + pow(canshu[1], 2) + pow(canshu[2], 2) + pow(canshu[3], 2),0.5);
	canshu[0] /= fenmu;
	canshu[1] /= fenmu;
	canshu[2] /= fenmu;
	canshu[3] /= fenmu;
}


float x;
float y;
float z;
void readFile(const char *filename) {
	FILE *fp = fopen(getN(), "r");

	int line_count = 0;
	char buffer[1024];


	//从文件中读取字符
	while (fgets(buffer, 1023, fp)) {

		line_count++; // Increment line counter

		if (line_count == 1) {
			continue;
		}
		else if (line_count == 2) {
			char *bufferp = buffer;
			sscanf(bufferp, "%d%d", &nverts, &nfaces);
		}
		else if (line_count <= 2 + nverts) {
			char *bufferp = buffer;

			//读取顶点的坐标
			HE_vert vert;

			sscanf(bufferp, "%f%f%f", &x, &y, &z);
			vert.x = x;
			vert.y = y;
			vert.z = z;

			//verts[line_count - 2-1] = vert;
			verts.push_back(vert);

		}
		else if (line_count <= 2 + nverts + nfaces) {
			char *bufferp = buffer;

			// 读取面的顶点数
			bufferp = strtok(bufferp, " \t");
			int faceVertNum = atoi(bufferp);

			HE_Face *face = new HE_Face();
			bufferp = strtok(NULL, " \t");
			face->v[0] = atoi(bufferp);
			bufferp = strtok(NULL, " \t");
			face->v[1] = atoi(bufferp);
			bufferp = strtok(NULL, " \t");
			face->v[2] = atoi(bufferp);

			fill_face(face);
			face->calcNorm();
			face->index_in_faces = faces.size();

			faces.push_back(*face);
		}
	}
	fill_vert();
};

// Face只包含顶点信息，补充边信息。
// 构造边的map
void fill_face(HE_Face *face) {
	HE_HalfEdge *edge1 = new HE_HalfEdge();
	edge1->v1 = face->v[0];
	edge1->v2 = face->v[1];
	edge1->right_face = face;
	//verts_to_edge.insert();
	verts_to_edge.insert(make_pair(make_pair(edge1->v1, edge1->v2), edge1));

	HE_HalfEdge *edge2 = new HE_HalfEdge();
	edge2->v1 = face->v[1];
	edge2->v2 = face->v[2];
	edge2->right_face = face;
	verts_to_edge.insert(make_pair(make_pair(edge2->v1, edge2->v2), edge2));

	HE_HalfEdge *edge3 = new HE_HalfEdge();
	edge3->v1 = face->v[2];
	edge3->v2 = face->v[0];
	edge3->right_face = face;
	verts_to_edge.insert(make_pair(make_pair(edge3->v1, edge3->v2), edge3));

	edge1->next = edge2;
	edge2->next = edge3;
	edge3->next = edge1;

	edge1->pre = edge3;
	edge2->pre = edge1;
	edge3->pre = edge2;

	face->edges[0] = edge1;
	face->edges[1] = edge2;
	face->edges[2] = edge3;
}


//使用边的map填充 点包含边的信息;(必须在文件读取完毕后)
void  fill_vert() {

	map<pair<int, int>, HE_HalfEdge*>::iterator iter;
	for (iter = verts_to_edge.begin(); iter != verts_to_edge.end(); iter++)
	{
		int from  = iter->first.first;
		int to = iter->first.second;
		
		HE_HalfEdge* currentEdge = iter->second;

		//填充边的oppo
		currentEdge->oppo = verts_to_edge.find(make_pair(to, from))->second;

		verts[from].edges.push_back(currentEdge);
	}

	
	//map信息不再需要，
	verts_to_edge.clear();
	//delete &verts_to_edge;
}

void calcQ_per_vertex(int vert_i) {
	Matrix4f Q;
	Q << 0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0;

	HE_vert current = verts[vert_i];
	vector<HE_HalfEdge*> edges = current.edges;
	for (int edge_i = 0; edge_i < edges.size(); edge_i++) {
		HE_Face* face = edges[edge_i]->right_face;
		float* canshu = face->canshu;
		float a = canshu[0];
		float b = canshu[1];
		float c = canshu[2];
		float d = canshu[3];
		Vector4f p(a, b, c, d);
		Matrix4f m;
		m << pow(a, 2), a*b, a*c, a*d,
			a*b, pow(b, 2), b*c, b*d,
			a*c, b*c, pow(c, 2), c*d,
			a*d, b*d, c*d, pow(d, 2);

		Q +=  m;
	}
	current.Q = Q;
}

void calcQ() {
	//所有的面计算abcd
	for (int face_i = 0; face_i < nfaces;face_i++) {
		HE_Face face= faces[face_i];
		if (!face.erased) {
			face.calcCan();
		}
	}
	//所有节点计算Q矩阵
	for (int vert_i = 0; vert_i < nverts; vert_i++) {
		calcQ_per_vertex(vert_i);
	}
}
void update_edge_error(HE_HalfEdge *edge) {
	if (!edge->valid) {
		return;
	}
	int v1 = edge->v1;
	int v2 = edge->v2;

	HE_vert vert1 = verts[v1];
	HE_vert vert2 = verts[v2];

	Matrix4f Q_bar = vert1.Q + vert2.Q;

	Vector4f v_bar((vert1.x + vert2.x) / 2.0, (vert1.y + vert2.y) / 2.0, (vert1.z + vert2.z) / 2.0, 1);

	float error = v_bar.transpose() * Q_bar * v_bar;

	edge->error = error;

}
void edges_to_heap() {
	for (int face_i = 0; face_i < nfaces; face_i++) {
		HE_Face* face = &faces[face_i];
		HE_HalfEdge** edges = face->edges;
		for (int edge_i = 0; edge_i < 3; edge_i++) {
			update_edge_error(edges[edge_i]);
			sorted_edge.push_back(edges[edge_i]);
		}
	}
}

//QEM 简化
void QEM() {
	//1.所有点计算Q
	calcQ();
	
	//2.所有的有效边加入堆
	//edges_to_heap();

	//改成有序链表
	edges_to_heap();
	sorted_edge.sort(sort_stru());
	
	int reduce_faces = (int)(simple_rate * nfaces);
	//printf("减少的面数为%d\n", reduce_faces);
	//3.每次移除最小的边，进行合并 
	for (int r_th = 0; r_th < reduce_faces/2; r_th++) {

		//拿出 error最小的 有效边
		HE_HalfEdge* edge = sorted_edge.front();
		sorted_edge.pop_front();
		sorted_edge.pop_front();
		while (!edge->valid) {
			edge = sorted_edge.front();
			sorted_edge.pop_front();
			sorted_edge.pop_front();
		}
		edge->valid = false;
		edge->oppo->valid = false;

		//要删除的两个点
		int v1 = edge->v1;
		int v2 = edge->v2;

		HE_vert* vert1 = &verts[v1];
		HE_vert* vert2 = &verts[v2];

		
		//要添加的新点
		HE_vert vert_new((vert1->x + vert2->x) / 2.0, (vert1->y + vert2->y) / 2.0, (vert1->z + vert2->z) / 2.0);
		vert_new.Q = vert1->Q + vert2->Q;
		//		新点的ID
		int index_new = verts.size();
		verts.push_back(vert_new);


		//vert1->valid = index_new;
		//vert2->valid = index_new;

		//		新点的邻接边是两个点的边并集
		for (int vert1_index = 0; vert1_index < vert1->edges.size(); vert1_index++) {
			vert_new.edges.push_back(vert1->edges[vert1_index]);
		}
		for (int vert2_index = 0; vert2_index < vert2->edges.size(); vert2_index++) {
			vert_new.edges.push_back(vert2->edges[vert2_index]);
		}

		//修改与删除点相关的边,使这些边和新点关联
		for (int i = 0; i < vert_new.edges.size(); i++) {
			HE_HalfEdge* re_edge = vert_new.edges[i];

			re_edge->v1 = index_new;	//从这点出去的边的源v1为新点id
			re_edge->oppo->v2 = index_new; //对边的目的v2为新点id

			update_edge_error(re_edge);
			update_edge_error(re_edge->oppo);
		}
			

		//标记删除的两个面
		HE_Face* face1=	edge->right_face;
		HE_Face* face2 = edge->oppo->right_face;

		faces[face1->index_in_faces].erased = true;
		faces[face2->index_in_faces].erased = true;
		
		//标记删除的六条边,并合并半边
		HE_HalfEdge** ed= face1->edges;
		for (int ed_i = 0; ed_i < 3;ed_i++) {
			ed[ed_i]->valid = false;
			ed[ed_i]->oppo = ed[ed_i]->next->oppo;
		}
		ed = face2->edges;
		for (int ed_i = 0; ed_i < 3; ed_i++) {
			ed[ed_i]->valid = false;
			ed[ed_i]->oppo = ed[ed_i]->next->oppo;
		}

		sorted_edge.sort(sort_stru());
	}

}


// GLUT参数
static int GLUTwindow = 0;
static int GLUTwindow_height = 600;
static int GLUTwindow_width = 900;
static int GLUTmouse[2] = { 0, 0 };
static int GLUTbutton[3] = { 0, 0, 0 };
static int GLUTarrows[4] = { 0, 0, 0, 0 };

// 显示参数
static int scaling = 0;
static int translating = 0;
static int rotating = 0;
static float scale = 20.0;
static float center[3] = { 0.0, 0.0, 0.0 };
static float rotation[3] = { 0.0, 0.0, 0.0 };
static float translation[3] = { 0.0, 0.0, -4.0 };

void GLUTRedraw(void)
{
	glLoadIdentity();
	glScalef(scale, scale, scale);
	glTranslatef(translation[0], translation[1], 0.0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLfloat)GLUTwindow_width / (GLfloat)GLUTwindow_height, 0.1, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(translation[0], translation[1], translation[2]);

	glScalef(scale, scale, scale);

	glRotatef(rotation[0], 1.0, 0.0, 0.0);
	glRotatef(rotation[1], 0.0, 1.0, 0.0);
	glRotatef(rotation[2], 0.0, 0.0, 1.0);


	glTranslatef(-center[0], -center[1], -center[2]);

	glClearColor(0.0, 0.5, 0.5, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 设置光源
	static GLfloat light0_position[] = { 3.0, 4.0, 5.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	int sum_erased = 0;
	int sum_non_erased = 0;
	for (int i=0; i < faces.size(); i++) {
		if (faces[i].erased) {
			sum_erased++;
		}else{
			sum_non_erased++;
		}
	}
	//printf("%d:%d\n",sum_erased,sum_non_erased);
	

	// 画出所有的面
	int i = 0;
	for (; i < faces.size(); i++) {
		HE_Face* face = &(faces[i]);
		/*if (face->erased) {
			continue;
		}*/
		face->calcNorm();
		face->calcCan();
		glBegin(GL_POLYGON);
		glNormal3fv(face->norm);
		for (int j = 0; j < 3; j++) {
			HE_vert vert = verts[face->v[j]];
			glVertex3f(vert.x, vert.y, vert.z);
		}

		glEnd();
	}
	glutSwapBuffers();

}

// 当鼠标拖动时，将每一帧都调用一次这个函数
void GLUTMotion(int x, int y)
{
	y = GLUTwindow_height - y;  //y坐标反转

								//处理鼠标事件
	if (rotating) {             //旋转
		rotation[0] += -0.5 * (y - GLUTmouse[1]);
		rotation[2] += 0.5 * (x - GLUTmouse[0]);
	}
	else if (translating) {     //移动
		translation[0] += 2.0 * (float)(x - GLUTmouse[0]) / (float)GLUTwindow_width;
		translation[1] += 2.0 * (float)(y - GLUTmouse[1]) / (float)GLUTwindow_height;
	}
	else if (scaling) {			//缩放
		scale *= exp(2.0 * (float)(x - GLUTmouse[0]) / (float)GLUTwindow_width);
	}

	// 记录鼠标的位置
	GLUTmouse[0] = x;
	GLUTmouse[1] = y;
}

// 处理鼠标左右键和中键摁下的事件
void GLUTMouse(int button, int state, int x, int y)
{
	y = GLUTwindow_height - y;      //y坐标反转

									//处理鼠标的状态
	rotating = (button == GLUT_LEFT_BUTTON);
	scaling = (button == GLUT_MIDDLE_BUTTON);
	translating = (button == GLUT_RIGHT_BUTTON);
	if (rotating || scaling || translating) glutIdleFunc(GLUTRedraw);
	else glutIdleFunc(0);

	// 记录按键的状态
	int b = (button == GLUT_LEFT_BUTTON) ? 0 : ((button == GLUT_MIDDLE_BUTTON) ? 1 : 2);
	GLUTbutton[b] = (state == GLUT_DOWN) ? 1 : 0;

	// 记录鼠标的位置
	GLUTmouse[0] = x;
	GLUTmouse[1] = y;
}

void GLUTInit(int *argc, char **argv)
{
	//创建窗口
	glutInit(argc, argv);
	glutInitWindowPosition(100, 10);
	glutInitWindowSize(GLUTwindow_width, GLUTwindow_height);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	GLUTwindow = glutCreateWindow(" ");

	glutDisplayFunc(GLUTRedraw);
	glutMouseFunc(GLUTMouse);
	glutMotionFunc(GLUTMotion);

	// 初始化光照
	static GLfloat lmodel_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	static GLfloat light1_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
	glEnable(GL_LIGHT1);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}

void GLUTMainLoop(void)
{
	// 计算 bounding box
	float bbox[2][3] = {
		{ 1.0E30F, 1.0E30F, 1.0E30F },
	{ -1.0E30F, -1.0E30F, -1.0E30F }
	};
	for (int i = 0; i < nverts; i++) {
		HE_vert vert = verts[i];
		if (vert.x < bbox[0][0]) bbox[0][0] = vert.x;
		else if (vert.x > bbox[1][0]) bbox[1][0] = vert.x;
		if (vert.y < bbox[0][1]) bbox[0][1] = vert.y;
		else if (vert.y > bbox[1][1]) bbox[1][1] = vert.y;
		if (vert.z < bbox[0][2]) bbox[0][2] = vert.z;
		else if (vert.z > bbox[1][2]) bbox[1][2] = vert.z;
	}

	// 设置初始中心
	center[0] = 0.5 * (bbox[1][0] + bbox[0][0]);
	center[1] = 0.5 * (bbox[1][1] + bbox[0][1]);
	center[2] = 0.5 * (bbox[1][2] + bbox[0][2]);

	glutMainLoop();
}



const char *file = "bunny.off";
int main(int argc, char **argv)
{
	GLUTInit(&argc, argv);

	
	readFile(file);
	QEM();
	GLUTMainLoop();
	return 0;
}
