#include "stdafx.h"

#include <iostream>
#include<Windows.h>
#include<gl/GL.h>
#include<gl/GLU.h>
#include<GL/glut.h>
#include <math.h>
#include<list>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#define SCALE_RATE 0.1
#define ROTATE_RATE 0.05

using namespace std;

 

//图形的结构体
typedef struct mystruc
{
	//图形颜色
	float* color;

	//顶点的数量
	int node_num;

	//顶点的顺时针坐标
	double** coordinate;

	//是不是被选中
	bool clicked = false;

	//点击时坐标
	double* clickedCoor;
}figure;

bool inCloser(double cx, double cy, figure &f);
void rotate(figure& f, int flag);
void scaleFigure(figure& f,int flag);

list<figure> figures;

void addFigures(int num , double coordinate[][2]) {
	figure f;
	f.node_num = num;

	f.coordinate = new double*[num];
	for (int i = 0; i < num; i++) {
		f.coordinate[i] = new double[2];
		f.coordinate[i][0] = coordinate[i][0] * WIN_WIDTH / 2;
		f.coordinate[i][1] = coordinate[i][1] * WIN_HEIGHT / 2;
	}
	figures.push_front(f);
}

float colors[3][3] = { 
	{ 1.0f, 0.0f, 1.0f },
	{ 0.5f, 0.0f, 1.0f },
	{ 0.5f, 0.0f, 1.0f } };
//初始化三个图案
void initFigures() {

	//三角形
	{
		int num = 3;
		double coordinate[3][2] = { {0.0,0.0},{-0.3,-0.3},{0.3,-0.3} };
		
		addFigures(num, coordinate);
	}
	//四边形
	{
		int num = 4;
		double coordinate[4][2] = { { -0.6,0.3 },{ -0.6,0.0 },{ 0.0,0.0 },{ 0.0,0.3 } };

		addFigures(num, coordinate);
	}
	//三角形
	{
		int num = 5;
		double coordinate[5][2] = { { 0.0,0.0 },{ 0.4,0. },{ 0.4,0.3 } ,{0.2,0.5},{0,0.3} };

		addFigures(num, coordinate);
	}
}

void InitSence()
{
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 30, 0, 20);
	glViewport(0, WIN_WIDTH, 0,WIN_HEIGHT);
}

void display()
{
	glClearColor(1.0, 1.0, 0.6, 1.0);  //改变默认背景有颜色为浅黄色，要写在glLoadIdentity()前  

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glPushMatrix();

	list<figure>::iterator iter = figures.begin();

	int index = 0;
	for (;iter != figures.end();iter++)
	{
		figure f = (*iter);
		
		glColor3f(colors[index][0], colors[index][1], colors[index][2]);
		index++;

		glBegin(GL_POLYGON);
		for (int i = 0; i < f.node_num; i++) {
			glVertex2f(f.coordinate[i][0] / (WIN_WIDTH/2),f.coordinate[i][1]/(WIN_HEIGHT/2));
		}
		glEnd();
	}
	glPopMatrix();

	glutSwapBuffers();
}
 

void processMouseClick(int button, int state, int x, int y) {
	/****************************       左键按下      **********************************/
	if (button == GLUT_LEFT_BUTTON && GLUT_DOWN == state) {

		//click coordinate（左上角为(0,0) => 中心为(0,0)）
		double cx = x - WIN_WIDTH/2;
		double cy = -y + (WIN_HEIGHT)/2;

		//click 点和所有的凸包图案判断位置
		for (list<figure>::iterator iter = figures.begin(); iter != figures.end(); iter++)	//per figure
		{
			figure f = *iter;
			bool in_figure = inCloser(cx, cy,f);
			if (in_figure) {
				//点中当前的这个进行标记
				f.clicked = true;
				f.clickedCoor = new double[2]{ cx,cy };

				//将当前这个figure放到list的头部
				iter = figures.erase(iter);
				figures.push_front(f);

				break;
			}
		}		
	}
	/****************************       左键松开      **********************************/
	else if (GLUT_LEFT_BUTTON == button && GLUT_UP == state) {
		//第一个figure的标记修改
		figure* f= &*(figures.begin());
		f->clicked = false;
		f->clickedCoor = NULL;
	}
	else if (GLUT_WHEEL_UP == button || GLUT_WHEEL_DOWN == button) {
		
		//click coordinate（左上角为(0,0) => 中心为(0,0)） 
		double cx = x - WIN_WIDTH / 2;
		double cy = -y + (WIN_HEIGHT) / 2;

		//鼠标所在点 和 所有的凸包图案 判断位置 
		for (list<figure>::iterator iter = figures.begin(); iter != figures.end(); iter++)	//per figure
		{
			figure f = *iter;
			bool in_figure = inCloser(cx, cy, f);
			if (in_figure) {

				int mod = glutGetModifiers();
				if (mod == GLUT_ACTIVE_SHIFT) {
					//对这个图案进行缩放
					scaleFigure(f, button);
					break;
				}
				else {
					//对这个图案进行旋转
					rotate(f,button);
					break;
				}
			}
		}
	}
}
void rotate(figure& f, int flag) {

	int fig_node_nums = f.node_num;

	double sum_x = 0, sum_y = 0;
	for (int i = 0; i < fig_node_nums; i++) {	//per node of the figure
		sum_x += f.coordinate[i][0];
		sum_y += f.coordinate[i][1];
	}
	double centerx = sum_x / fig_node_nums;
	double centery = sum_y / fig_node_nums;

	for (int i = 0; i < fig_node_nums; i++) {	//per node of the figure
		if (flag == GLUT_WHEEL_UP) {


			double tempx = (f.coordinate[i][0] - centerx)*cos(ROTATE_RATE) - (f.coordinate[i][1] - centery)*sin(ROTATE_RATE) + centerx;
			double tempy = (f.coordinate[i][0] - centerx)*sin(ROTATE_RATE) + (f.coordinate[i][1] - centery)*cos(ROTATE_RATE) + centery;

			f.coordinate[i][0] = tempx;
			f.coordinate[i][1] = tempy;

		}
		else if (flag == GLUT_WHEEL_DOWN) {

			double tempx = (f.coordinate[i][0] - centerx)*cos(-ROTATE_RATE) - (f.coordinate[i][1] - centery)*sin(-ROTATE_RATE) + centerx;
			double tempy = (f.coordinate[i][0] - centerx)*sin(-ROTATE_RATE) + (f.coordinate[i][1] - centery)*cos(-ROTATE_RATE) + centery;

			f.coordinate[i][0] = tempx;
			f.coordinate[i][1] = tempy;
		}
	}
	glutPostRedisplay();

}
void scaleFigure(figure& f,int flag) {
	int fig_node_nums = f.node_num;
	
	double sum_x = 0, sum_y = 0;
	for (int i = 0; i < fig_node_nums; i++) {	//per node of the figure
		sum_x += f.coordinate[i][0];
		sum_y += f.coordinate[i][1];
	}
	double centerx = sum_x / fig_node_nums;
	double centery = sum_y / fig_node_nums;

	for (int i = 0; i < fig_node_nums; i++) {	//per node of the figure
		if (flag == GLUT_WHEEL_UP) {
			f.coordinate[i][0] = (f.coordinate[i][0] - centerx)*(1 - SCALE_RATE) + centerx;
			f.coordinate[i][1] = (f.coordinate[i][1] - centery)*(1 - SCALE_RATE) + centery;
		}
		else if (flag== GLUT_WHEEL_DOWN) {
			f.coordinate[i][0] = (f.coordinate[i][0] - centerx)*(1 + SCALE_RATE) + centerx;
			f.coordinate[i][1] = (f.coordinate[i][1] - centery)*(1 + SCALE_RATE) + centery;
		}
	}
	glutPostRedisplay();
}
bool inCloser(double cx,double cy,figure &f){
	
	int fig_node_nums = f.node_num;

	//PA,PB,PC     (3,2)
	double** veters = new double*[fig_node_nums];
	for (int i = 0; i < fig_node_nums; i++) {	//per node of the figure
		veters[i] = new double[2];
		veters[i][0] = f.coordinate[i][0] - cx;
		veters[i][1] = f.coordinate[i][1] - cy;
	}

	bool in_figue = true;

	//叉乘的第三坐标大于零记为true , 记录第一次叉乘的结果
	bool flag;
	for (int i = 0; i < fig_node_nums; i++) {
		//交叉相乘相减
		bool res = (veters[i][0] * veters[(i + 1) % f.node_num][1]
			- veters[i][1] * veters[(i + 1) % f.node_num][0])
			> 0;

		// 其余的和第一个比较
		if (0 == i) { //第一个
			flag = res;
		}
		else if ((flag && !res) || (!flag && res)) {  //第2,3...个 比较(异或)
			in_figue = false;
			break;
		}
	}
	return in_figue;
}
void processMouseMove(int x,int y) {
	int cx = x - 400;
	int cy = 300 - y;

	figure f= *(figures.begin());
	if (f.clicked) {
		double** coor = f.coordinate;
		int node_num = f.node_num;

		for (int i = 0; i < node_num; i++) {
			coor[i][0] += cx - f.clickedCoor[0];
			coor[i][1] += cy - f.clickedCoor[1];	
		}
		f.clickedCoor[0] = cx;
		f.clickedCoor[1] = cy;
		
		glutPostRedisplay();
	}
}

int main(int argc, char** argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);


	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	glutCreateWindow("双車霸王鸡");

	initFigures();
	glutDisplayFunc(display);

	glutMouseFunc(processMouseClick);
	glutMotionFunc(processMouseMove);

	glutMainLoop();
}