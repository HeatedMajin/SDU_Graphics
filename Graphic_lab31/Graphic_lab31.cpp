#include "stdafx.h"


using namespace std;

struct HE_vert;
struct HE_HalfEdge;
struct HE_Face;

struct HE_vert {
	float x;
	float y;
	float z;
	HE_vert *vnew;

	vector<HE_HalfEdge*> edges; //从该点出去的边
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

	//HE_vert *ve; //用于细分时计算边上的E-顶点
	int ve_i = -1; //保存到vector中的index
};

struct HE_Face {
	int v[3];

	HE_HalfEdge *edges[3];

	float norm[3];

	HE_Face() {}
	HE_Face(int v0, int v1, int v2) {
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
	}

	void calcNorm();
};

//#######################################################################################################
void fill_face(HE_Face &face);
void  fill_vert();
//#######################################################################################################

vector<HE_vert> verts;
map<pair<int, int>, HE_HalfEdge*> verts_to_edge;
vector<HE_Face> faces;
vector<HE_Face> faces_tmp;
int nverts = 0;
int nfaces = 0;
int nedges = 0;

void HE_Face::calcNorm() {
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

float x;
float y;
float z;
void readFile(const char *filename) {

	FILE *fp = fopen(filename, "r");

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
			sscanf(bufferp, "%d%d%d", &nverts, &nfaces, &nedges);
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

			HE_Face face;
			bufferp = strtok(NULL, " \t");
			face.v[0] = atoi(bufferp);
			bufferp = strtok(NULL, " \t");
			face.v[1] = atoi(bufferp);
			bufferp = strtok(NULL, " \t");
			face.v[2] = atoi(bufferp);


			//#################################################################################################
			fill_face(face);
			//#################################################################################################
			face.calcNorm();
			faces.push_back(face);
		}
	}

	//########################################################################################################
	fill_vert();
	//########################################################################################################
};

// Face只包含顶点信息，补充边信息。
// 构造边的map
void fill_face(HE_Face &face) {
	HE_HalfEdge *edge1 = new HE_HalfEdge();
	edge1->v1 = face.v[0];
	edge1->v2 = face.v[1];
	edge1->right_face = &face;
	verts_to_edge.insert(make_pair(make_pair(edge1->v1, edge1->v2), edge1));

	HE_HalfEdge *edge2 = new HE_HalfEdge();
	edge2->v1 = face.v[1];
	edge2->v2 = face.v[2];
	edge2->right_face = &face;
	verts_to_edge.insert(make_pair(make_pair(edge2->v1, edge2->v2), edge2));

	HE_HalfEdge *edge3 = new HE_HalfEdge();
	edge3->v1 = face.v[2];
	edge3->v2 = face.v[0];
	edge3->right_face = &face;
	verts_to_edge.insert(make_pair(make_pair(edge3->v1, edge3->v2), edge3));

	edge1->next = edge2;
	edge2->next = edge3;
	edge3->next = edge1;

	edge1->pre = edge3;
	edge2->pre = edge1;
	edge3->pre = edge2;

	face.edges[0] = edge1;
	face.edges[1] = edge2;
	face.edges[2] = edge3;
}


//使用边的map填充 点包含边的信息;(必须在文件读取完毕后)
void  fill_vert() {

	map<pair<int, int>, HE_HalfEdge*>::iterator iter;
	for (iter = verts_to_edge.begin(); iter != verts_to_edge.end(); iter++)
	{
		int from = iter->first.first;
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
float new_x = 0;
float new_y = 0;
float new_z = 0;
int v0, v1, v2, v3;
//loop 细分
void loopSub() {
	/////////////////  计算边点  ///////////////////
	for (int face_i = 0; face_i < nfaces; face_i++) {
		HE_HalfEdge **edges = faces[face_i].edges;
		for (int edge_i = 0; edge_i < 3; edge_i++) {
			HE_HalfEdge *current_edge = edges[edge_i];
			v0 = current_edge->v1;
			v1 = current_edge->v2;
			v2 = current_edge->next->v2;
			v3 = current_edge->oppo->next->v2;

			new_x = 3.0 / 8 * (verts[v0].x + verts[v1].x) + 1.0 / 8 * (verts[v2].x + verts[v3].x);
			new_y = 3.0 / 8 * (verts[v0].y + verts[v1].y) + 1.0 / 8 * (verts[v2].y + verts[v3].y);
			new_z = 3.0 / 8 * (verts[v0].z + verts[v1].z) + 1.0 / 8 * (verts[v2].z + verts[v3].z);

			////////////////////// 添加新点 //////////////////////////
			verts.push_back(HE_vert(new_x, new_y, new_z));
			if (current_edge->ve_i == -1) {
				current_edge->ve_i = verts.size() - 1;
				current_edge->oppo->ve_i = verts.size() - 1;
			}
			/////////////////////////////////////////////////////////

		}
	}

	//////////////  计算新的顶点位置  ///////////////
	for (int i = 0; i < verts.size(); i++) {
		HE_vert v0 = verts[i];
		vector<HE_HalfEdge*> edges = v0.edges;

		int n = edges.size();
		double beita = 1.0 / n * (5.0 / 8 - pow(3.0 / 8 + 1.0 / 4 * cos(2 * atan(1.) * 4), 2));

		double sum_x = 0.0;
		double sum_y = 0.0;
		double sum_z = 0.0;

		//该点所有的邻接点
		for (int ii = 0; ii < n; ii++) {
			if (i != edges[ii]->v1) {
				sum_x += verts[edges[ii]->v1].x;
				sum_y += verts[edges[ii]->v1].y;
				sum_z += verts[edges[ii]->v1].z;
			}
			else {
				sum_x += verts[edges[ii]->v2].x;
				sum_y += verts[edges[ii]->v2].y;
				sum_z += verts[edges[ii]->v2].z;
			}
		}

		v0.x = (1 - n * beita) * v0.x + beita * sum_x;
		v0.y = (1 - n * beita) * v0.y + beita * sum_y;
		v0.z = (1 - n * beita) * v0.z + beita * sum_z;

	}

	//////// 添加新的点、修改所有的面
	// 弹出每一个面，加入四个小面
	// 遇到问题：如何保证正反两个边上的中点只被加入一次
	// 
	for (int i = 0; i < nfaces; i++) {
		HE_Face f = faces[i];
		HE_HalfEdge **edges = f.edges;

		int v0 = edges[0]->v1;
		int v1 = edges[0]->ve_i;
		int v2 = edges[1]->v1;
		int v3 = edges[1]->ve_i;
		int v4 = edges[2]->v1;
		int v5 = edges[2]->ve_i;

		HE_Face f1(v0, v1, v5);
		f1.calcNorm();
		HE_Face f2(v1, v2, v3);
		f2.calcNorm();
		HE_Face f3(v5, v3, v4);
		f2.calcNorm();
		HE_Face f4(v1, v3, v5);
		f4.calcNorm();

		faces_tmp.push_back(f1);
		faces_tmp.push_back(f2);
		faces_tmp.push_back(f3);
		faces_tmp.push_back(f4);
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
const char *file = "bunny.off";
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

	// 画出所有的面
	for (int i = 0; i < nfaces * 4; i++) {
		HE_Face face = faces_tmp[i];
		glBegin(GL_POLYGON);
		glNormal3fv(face.norm);
		for (int j = 0; j < 3; j++) {
			HE_vert vert = verts[face.v[j]];
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




int main(int argc, char **argv)
{
	GLUTInit(&argc, argv);

	
	readFile(file);
	loopSub();
	GLUTMainLoop();
	return 0;
}
