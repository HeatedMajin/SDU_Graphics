#include "stdafx.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include<string>
#include<vector>
#include<iostream>
using namespace std;

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

typedef struct Vertex { //定义x，y，z坐标
	float x, y, z;
} Vertex;

typedef struct Face {
	Face(void) : nverts(0), verts(0) {};
	int nverts;
	Vertex **verts; 
	float normal[3];
} Face;

typedef struct Mesh {
	Mesh(void) : nverts(0), verts(0), nfaces(0), faces(0) {};
	int nverts;
	Vertex *verts;
	int nfaces;
	Face *faces;

	vector<Vertex>point;
} Mesh;

const char* filename= "bunny.off";

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

// Mesh 数组
static Mesh *mesh = NULL;

Mesh * ReadOffFile(const char *filename)
{
	int i;
	FILE *fp = fopen(filename, "r");
	Mesh *mesh = new Mesh();

	int nverts = 0;
	int nfaces = 0;
	int nedges = 0;

	int line_count = 0;
	char buffer[1024];

	//从文件中读取字符
	while (fgets(buffer, 1023, fp)) {
		line_count++; // Increment line counter

		char *bufferp = buffer;
		
		// 跳过空格、空行和注释行
		while (isspace(*bufferp)) bufferp++;
		if (*bufferp == '#') continue;
		if (*bufferp == '\0') continue;
		if (*bufferp == 'OFF') continue;
		if (nverts == 0) {
			if (!strstr(bufferp, "OFF")) {
				// 读取点数、面数、边数
				sscanf(bufferp, "%d%d%d", &nverts, &nfaces, &nedges);
				mesh->verts = new Vertex[nverts];
				mesh->faces = new Face[nfaces];
			}
		}
		else if (mesh->nverts < nverts) {
			//读取顶点的坐标
			Vertex& vert = mesh->verts[mesh->nverts++];
			if (sscanf(bufferp, "%f%f%f", &(vert.x), &(vert.y), &(vert.z)) != 3) {
				fprintf(stderr, "Syntax error with vertex coordinates on line %d in file %s\n", line_count, filename);
				fclose(fp);
				return NULL;
			}
		}
		else if (mesh->nfaces < nfaces) {
			Face& face = mesh->faces[mesh->nfaces++];

			// 读取面的顶点数
			bufferp = strtok(bufferp, " \t");
			face.nverts = atoi(bufferp);
			face.verts = new Vertex *[face.nverts];
			
			for (i = 0; i < face.nverts; i++) {
				bufferp = strtok(NULL, " \t");
				face.verts[i] = &(mesh->verts[atoi(bufferp)]);
			}

			// 计算法向量
			face.normal[0] = face.normal[1] = face.normal[2] = 0;
			Vertex *v1 = face.verts[face.nverts - 1];
			for (i = 0; i < face.nverts; i++) {
				Vertex *v2 = face.verts[i];
				face.normal[0] += (v1->y - v2->y) * (v1->z + v2->z);
				face.normal[1] += (v1->z - v2->z) * (v1->x + v2->x);
				face.normal[2] += (v1->x - v2->x) * (v1->y + v2->y);
				v1 = v2;
			}

			// 单位化法向量
			float squared_normal_length = 0.0;
			squared_normal_length += face.normal[0] * face.normal[0];
			squared_normal_length += face.normal[1] * face.normal[1];
			squared_normal_length += face.normal[2] * face.normal[2];
			float normal_length = sqrt(squared_normal_length);
			if (normal_length > 1.0E-6) {
				face.normal[0] /= normal_length;
				face.normal[1] /= normal_length;
				face.normal[2] /= normal_length;
			}
		}
	}
	fclose(fp);
	return mesh;
}

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
	for (int i = 0; i < mesh->nfaces; i++) {
		Face& face = mesh->faces[i];
		glBegin(GL_POLYGON);
		glNormal3fv(face.normal);
		for (int j = 0; j < face.nverts; j++) {
			Vertex *vert = face.verts[j];
			glVertex3f(vert->x, vert->y, vert->z);
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
	scaling = (button==GLUT_MIDDLE_BUTTON);
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
	for (int i = 0; i < mesh->nverts; i++) {
		Vertex& vert = mesh->verts[i];
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
	mesh = ReadOffFile(filename);
	GLUTMainLoop();
	return 0;
}