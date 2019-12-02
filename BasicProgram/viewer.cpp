#define GLM_FORCE_RADIANS
#define _USE_MATH_DEFINES

#include <GL/glew.h>
#define GLFW_DLL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <stdio.h>
#include "Shaders.h"
#include "tiny_obj_loader.h"
#include <iostream>
#include <ctime>
#include <vector>
#include "viewer.h"
#include <FreeImage/FreeImage.h>
#include "texture.h"
#include "Mesh.h"


#define R_Factor 0.01f

float eyex, eyey, eyez;	// current user position
float vx, vy, vz;	// current user position
float scalef=0.1f;   // scale factor
double theta, phi;		// user's position  on a sphere centered on the object
double r;				// radius of the sphere
float rot = 0;
float rotspeed = 0.05;

glm::mat4 projection;	// projection matrix

//Boids
GLuint program;
Mesh boids("Boids");
Mesh sphere("Sphere");


int width = 512, height = 512;


double lastX = 0, lastY = 0;
double lastsY = 0;
double mouseX = 0, mouseY = 0;
double speed = 0.01f;
float speedk = 0.1f;

glm::mat4 view;
int viewLoc;
int projLoc;
int texLoc;
int eyeLoc;

bool animate = true;
glm::vec3 eyepos;


//pre-defines

static void error_callback(int error, const char* description);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void framebufferSizeCallback(GLFWwindow* window, int w, int h);


float sizedRandom(float size) {
	float a = size * rand() / (RAND_MAX);
	return  a*1.0f;
}

float normalizedRandom() {	
	return (double) rand() / (RAND_MAX);
}

int randomNeg() {
	int a = rand() % 2;
	return a == 0 ? -1 : 1;
}

float is0or1() {
	int a = rand() % 2;
	return a == 0 ? 0 : 1.0f;
}


int numBoids = 100;
float boidsSize = 0.5f;
float centerSize = 4.0f;
float flockSize = boidsSize + 1.0f;
float scater = 1;

glm::vec3 obstaclePos;// ((centerSize / 2 + flockSize)* center.x, 0.0, center.z);

std::vector<glm::vec3> listBoid;
std::vector<GLfloat> rotationOffset;

glm::vec3 center(3.0f, 0, 0);

void initBoidsPos() {
	listBoid.clear();
	for (int i = 0; i < numBoids; i++) {
		//float dy = centerSize * normalizedRandom() * randomNeg();
		//float dz = centerSize * normalizedRandom() * randomNeg();
		//glm::vec3 pos(dx, dy, dz);
		//glm::vec3 rotate(rx, ry, rz);

		double dx = centerSize*2 + center.x + sizedRandom(flockSize) * randomNeg();
		double dz = center.z + sizedRandom(flockSize) * randomNeg();
		double dy = -dz;

		glm::vec3 pos(dx, dy, dz);
		float offset = sizedRandom(M_2_PI); 

		//
		float speed = normalizedRandom();
		//

		if (numBoids < 10) {
			printf("%f %f %f\n", pos.x, pos.y, pos.z);
			printf("offeset %f\n", offset);
		}

		rotationOffset.push_back(offset);
		listBoid.push_back(pos);

	}

}

void init(const char* file, Mesh& model, GLuint& p) {
	GLuint vbuffer;
	GLuint tBuffer;
	GLint vPosition;
	GLint vNormal;
	GLint vTex;

	/*GLfloat vertices = model.vertices;
	GLfloat normals = model.normals;
	GLuint indices = model.indices;
	*/

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	GLuint& VAO = model.vao;
	int& Tri = model.triangles;
	GLuint& Ib = model.ibuffer;

	int& nv = model.nv;
	int& nn = model.nn;
	int& ni = model.ni;
	int nt;
	int i;

	float maxx = 0, maxz = 0;
	float minx = INFINITY, minz = INFINITY;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/*  Load the obj file */

	std::string err = tinyobj::LoadObj(shapes, materials, file, 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	/*  Retrieve the vertex coordinate data */

	nv = (int)shapes[0].mesh.positions.size();
	model.vertices = new GLfloat[nv];
	for (i = 0; i < nv; i++) {
		model.vertices[i] = shapes[0].mesh.positions[i];
	}

	/*  Retrieve the vertex normals */

	nn = (int)shapes[0].mesh.normals.size();
	model.normals = new GLfloat[nn];
	for (i = 0; i < nn; i++) {
		model.normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = (int)shapes[0].mesh.indices.size();
	Tri = ni / 3;
	model.indices = new GLuint[ni];
	for (i = 0; i < ni; i++) {
		model.indices[i] = shapes[0].mesh.indices[i];
	}
	/*
		calc text coord
	*/

	nt = (int)shapes[0].mesh.texcoords.size();
	GLfloat* tex = new GLfloat[nt];
	for (i = 0; i < nt; i++) {
		tex[i] = shapes[0].mesh.texcoords[i];
	}

	//double verts = nv / 3;
	//nt = 2 * verts;
	//for (i = 0; i < verts; i++) {
	//	GLfloat x = vertices[3 * i];
	//	GLfloat y = vertices[3 * i + 1];
	//	GLfloat z = vertices[3 * i + 2];
	//	theta = atan2(x,z);
	//	phi = atan2(y,sqrt(x*x + z*z));

	//	//tex[2 * i] = (theta+M_PI) / (2 * M_PI);
	//	tex[2 * i] = fabs(theta) / M_PI;
	//	
	//	tex[2 * i + 1] = phi / M_PI;
	//}


	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn + nt) * sizeof(GLfloat), NULL,
		GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nv * sizeof(GLfloat), model.vertices);
	glBufferSubData(GL_ARRAY_BUFFER, nv * sizeof(GLfloat), nn * sizeof(GLfloat), model.normals);
	glBufferSubData(GL_ARRAY_BUFFER, (nv + nn) * sizeof(GLfloat), nt * sizeof(GLfloat),
		tex);


	/*
	*  load the vertex indexes
	*/
	glGenBuffers(1, &Ib);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), model.indices, GL_STATIC_DRAW);


	/*
	*  link the vertex coordinates to the vPosition
	*  variable in the vertex program.  Do the same
	*  for the normal vectors.
	*/

	glUseProgram(p);
	vTex = glGetAttribLocation(p, "vTex");
	glVertexAttribPointer(vTex, 2, GL_FLOAT, GL_FALSE, 0, (void*)((nv + nn) * sizeof(GLfloat)));
	glEnableVertexAttribArray(vTex);
	vPosition = glGetAttribLocation(p, "vPosition");
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(p, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)((nv / 2) * sizeof(model.vertices)));
	glEnableVertexAttribArray(vNormal);
}

void start() {
	projection = glm::perspective(0.7f, 1.0f, 1.0f, 1000.0f);
	int fs;
	int vs;
	vs = buildShader(GL_VERTEX_SHADER, (char*)"Shaders/vert.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Shaders/frag.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, (char*)"shader program");

	init("Meshes/boids.obj", boids, program);
	init("Meshes/sphere.obj", sphere, program);

	//create init position
	initBoidsPos();
	eyex = 0.0;
	eyez = 0.0;
	eyey = 10.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	obstaclePos = glm::vec3((centerSize / 2 + flockSize) * center.x, 0.0, center.z);

}

void update() {
	//updates
	if (animate) {
		rot += rotspeed;
		if (rot > 360.0f) {
			rot = 0;
		}
	}
	scater -= 0.1;
	if (scater < 1) {
		scater = 1;
	}

	vx = (float)(r * sin(theta) * cos(phi));
	vy = (float)(r * sin(theta) * sin(phi));
	vz = (float)(r * cos(theta));

	//eyepos = glm::vec3(eyex, eyey, eyez);
	eyepos = glm::vec3(vx, vy, vz);
	obstaclePos = glm::vec3((centerSize / 2 + flockSize) * center.x, 0.0, center.z);

}

void display(void) {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//Boids
	glUseProgram(program);

	viewLoc = glGetUniformLocation(program, "modelView");
	projLoc = glGetUniformLocation(program, "projection");
	eyeLoc = glGetUniformLocation(program, "Eye");

	view = glm::lookAt(eyepos,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	/*view = glm::rotate(view, vx, glm::vec3(1.0, 0.0, 0.0));
	view = glm::rotate(view, vy, glm::vec3(0.0, 1.0, 0.0));
	view = glm::rotate(view, vz, glm::vec3(0.0, 0.0, 1.0));*/

	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(glm::max(centerSize * scalef, 0.0f)));

	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view * scale));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

	glBindVertexArray(sphere.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ibuffer);
	glDrawElements(GL_TRIANGLES, 3 * sphere.triangles, GL_UNSIGNED_INT, NULL);
	
	
	glm::mat4 transform(1.0f);

	transform = glm::translate(transform,obstaclePos);
	transform = glm::scale(transform, glm::vec3(0.5));

	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view * scale * transform));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

	glBindVertexArray(sphere.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ibuffer);
	glDrawElements(GL_TRIANGLES, 3 * sphere.triangles, GL_UNSIGNED_INT, NULL);

	for (int i = 0; i < numBoids; i++) {
		glm::mat4 transform(1.0f);

	/*	view = glm::lookAt(eyepos,
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));

		view = glm::rotate(view, vx, glm::vec3(1.0, 0.0, 0.0));
		view = glm::rotate(view, vy, glm::vec3(0.0, 1.0, 0.0));
		view = glm::rotate(view, vz, glm::vec3(0.0, 0.0, 1.0));*/

		//
		transform = glm::rotate(transform, (rot + rotationOffset[i]), glm::vec3(0, 0, 1.0f));

		transform = glm::translate(transform, scater * listBoid[i] * glm::vec3(1.0, 0.0 , 1.0));
	
		//translate = glm::translate(translate, modelScale * glm::vec3(0.0,1.0,0.0));
		transform = glm::rotate(transform,90.0f, glm::vec3(1.0,0.0,0.0));

		//transform = glm::translate(transform, scater * listBoid[i] * glm::vec3(0.0, 0.0, 1.0));

		transform = glm::scale(transform, glm::vec3(glm::max(boidsSize, 0.0f)));


		glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view *scale * transform));
		glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
		glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

		glBindVertexArray(boids.vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boids.ibuffer);
		glDrawElements(GL_TRIANGLES, 3*boids.triangles, GL_UNSIGNED_INT, NULL);
	}
}

int main(int argc, char **argv) {
	GLFWwindow *window;

	srand(time(NULL));
	// start by setting error callback in case something goes wrong


	// initialize glfw

	if (!glfwInit()) {
		fprintf(stderr, "can't initialize GLFW\n");
	}

	// create the window used by our application

	window = glfwCreateWindow(512, 512, "GLFW", NULL, NULL);

	if (!window){
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// establish framebuffer size change and input callbacks
	glfwSetErrorCallback(					error_callback);
	glfwSetFramebufferSizeCallback(window,	framebufferSizeCallback);
	glfwSetKeyCallback(window,				key_callback);
	glfwSetMouseButtonCallback(window,		mouse_button_callback);
	glfwSetCursorPosCallback(window,		cursor_position_callback);
	glfwSetScrollCallback(window,			scroll_callback);
	// now initialize glew our extension handler

	glfwMakeContextCurrent(window);

	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n", glewGetErrorString(error));
		exit(0);
	}
	//printf("%f\n", 1.0f/(rand()%10));
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glViewport(0, 0, 512, 512);

	glfwSwapInterval(1);


	// GLFW main loop, display model, swapbuffer and check for input

	start();

	while (!glfwWindowShouldClose(window)) {
		update();
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}













// CallBacks

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	float distance =  yoffset;
	float scaleChange = distance * R_Factor;
	/*if (yoffset > 0.0f) {
		scaleChange *= -1.0f;
	}*/
	scalef += scaleChange;
	scalef = scalef > 0 ? scalef : (0+.000000001f);
	//printf("%f\n", scalef);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	int stateLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	int stateRight = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

	bool areScaling = stateRight == GLFW_PRESS;
	bool mouseDown = (stateLeft == GLFW_PRESS) || (stateRight == GLFW_PRESS);

	if (mouseDown) {
		int x = (int)xpos;
		int y = (int)ypos;
		if (!std::isinf(lastX) && !std::isinf(lastY)) {
			float dx = lastX - (float)x;
			float dy = lastY - (float)y;
			printf("%f %f\n", dx, dy);

			float xrot = glm::abs(dx / 100);
			if (dx > 0) {
				phi += xrot;
			}
			else {
				phi -= xrot;
			}

			float yrot = glm::abs(dy / 100);
			if (dy < 0) {
				theta += yrot;
			}
			else {
				theta -= yrot;
			}

			lastX = (float)x;
			lastY = (float)y;

		}
		else {
			lastX = (float)x;
			lastY = (float)y;
		}
	}
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_RELEASE) {
		lastX = std::numeric_limits<float>::infinity();
		lastY = std::numeric_limits<float>::infinity();
	}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		//printf("Key: %s\n", glfwGetKeyName(key, 0));
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		if (action == GLFW_REPEAT) {
			if (key == GLFW_KEY_EQUAL) {
				rotspeed += 0.0001;
				printf("set rotaion speed %f\n", rotspeed);
			}
			else if (key == GLFW_KEY_MINUS) {
				rotspeed -= 0.0001;
				printf("set rotaion speed %f\n", rotspeed);
			}
		}
		else if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_R)
				initBoidsPos();
			if (key == GLFW_KEY_S)
				scater += 10;
			if (key == GLFW_KEY_EQUAL) {
				rotspeed += 0.01;
				printf("set rotaion speed %f\n", rotspeed);
			}
			else if (key == GLFW_KEY_MINUS) {
				rotspeed -= 0.01;
				printf("set rotaion speed %f\n", rotspeed);
			}
			if (key == GLFW_KEY_SPACE)
				animate = !animate;
		}
	}
}

void framebufferSizeCallback(GLFWwindow* window, int w, int h) {

	// Prevent a divide by zero, when window is too short

	if (h == 0)
		h = 1;

	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);

	width = w;
	height = h;

	glViewport(0, 0, w, h);

	projection = glm::perspective(0.7f, ratio, 1.0f, 1000.0f);

}
