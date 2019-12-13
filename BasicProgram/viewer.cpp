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

//#define DEBUGs

#define R_Factor 0.01f		//camera Rotation speed

#define BOIDS_S 20			// slop of Boids movement

float eyex, eyey, eyez;		// current user position
float vx, vy, vz;			// current user position
float scalef=0.1f;			// scale factor
double theta, phi;			// user's position  on a sphere centered on the object
double r;					// radius of the sphere
float rot = 0;				// dynamic rotation angle 
float rotspeed = 0.013f;	// rotation speed

glm::mat4 projection;		// projection matrix

//Boids
GLuint boidprogram;
Mesh boids("Boids");

//Center
GLuint program;
Mesh sphere("Sphere");

// Obstacle
GLuint obsprogram;
Mesh vox("VOX");

// Circle
GLuint cirprogram;
Mesh circle("cir");

//Skybox
GLuint cubeProgram;
GLuint cube_VAO;
GLuint cube_triangles;
GLuint cube_ibuffer;
Texture* texture;
Cube* textureCube;
GLuint tBuffer;
const char* cubmapUrl = "Cubemap\\galaxy";


int width = 512, height = 512;	//	Window Size 

//	Mouse Controls

double lastX = 0, lastY = 0;	
double lastsY = 0;
double mouseX = 0, mouseY = 0;
double speed = 0.01f;			// Camera Rotaion speed


//	Uniform Locations 
glm::mat4 view;							// Profectoin Matrix
glm::vec3 eyepos;						// Camera position
glm::vec3 origin = glm::vec3(0,0,0);	// Center 
glm::vec3 lookCenter = origin;			// Look at 
bool isFocus = false;

int viewLoc;					
int projLoc;
int texLoc;
int eyeLoc;

bool animate = true;			// [Space] key for start or pause the animation

//	callback defines
static void error_callback(int error, const char* description);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void framebufferSizeCallback(GLFWwindow* window, int w, int h);

// Random functions
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

//Euclidean distance
float distof(float a, float b) {
	return sqrt(a*a + b*b);
}

float scatterf(float a, float x) {
	//a = abs(a);
	return a * exp(-exp(-a / (BOIDS_S) * x));
}
float convergef(float a,float x) {
	//a = abs(a);
	return  a * exp(-exp(a / (BOIDS_S) * x));
}

// Boids Properties
int numBoids = 100;						//	Number Of Boids
float boidsSize = 0.5f;					//	Object Size of each Boid
float centerSize = 4.0f;				//	The Gap in Center
float flockSize = boidsSize + 1.0f;		//	Flock Size of Boids
float scater = 1;						//	dynamic Scater value scater boids when [S] hit 
float threshold = 0.2f;					//	reaction time of boids movment (How close to the obstacle)

glm::vec3 obstaclePos;					// ((centerSize / 2 + flockSize)* boidCenter.x, 0.0, boidCenter.z);
float obsAngle = 0;						//obstacle rotation angle

// Storage of boids
std::vector<glm::vec3> listBoid;		
std::vector<glm::vec3> shifts;
std::vector<GLfloat> rotationOffset;

glm::vec3 boidCenter(3.0f, 0, 0);

void initBoidsPos() {	
	listBoid.clear();
	shifts.clear();
	for (int i = 0; i < numBoids; i++) {

		double dx = centerSize*2 + boidCenter.x + sizedRandom(flockSize) * randomNeg();
		double dz = boidCenter.z + sizedRandom(flockSize) * randomNeg();
		double dy = 0;

		glm::vec3 pos(dx, dy, dz);
		float offset = sizedRandom(M_PI/2); 

		if (numBoids < 10) {
			printf("%f %f %f\n", pos.x, pos.y, pos.z);
			printf("offeset %f\n", offset);
		}

		rotationOffset.push_back(offset);
		listBoid.push_back(pos);
		shifts.push_back(glm::vec3(0.0f));

	}

}

void createSky() {
	GLuint vbuffer;
	GLint vPosition;
	GLint vNormal;

	glGenVertexArrays(1, &cube_VAO);
	glBindVertexArray(cube_VAO);

	GLfloat vertices[8][4] = {
			{ -1.0, -1.0, -1.0, 1.0 },		//0
			{ -1.0, -1.0, 1.0, 1.0 },		//1
			{ -1.0, 1.0, -1.0, 1.0 },		//2
			{ -1.0, 1.0, 1.0, 1.0 },		//3
			{ 1.0, -1.0, -1.0, 1.0 },		//4
			{ 1.0, -1.0, 1.0, 1.0 },		//5
			{ 1.0, 1.0, -1.0, 1.0 },		//6
			{ 1.0, 1.0, 1.0, 1.0 }			//7
	};

	GLfloat normals[8][3] = {
			{ -1.0, -1.0, -1.0 },			//0
			{ -1.0, -1.0, 1.0 },			//1
			{ -1.0, 1.0, -1.0 },			//2
			{ -1.0, 1.0, 1.0 },				//3
			{ 1.0, -1.0, -1.0 },			//4
			{ 1.0, -1.0, 1.0 },				//5
			{ 1.0, 1.0, -1.0 },				//6
			{ 1.0, 1.0, 1.0 }				//7
	};

	GLuint indexes[36] = { 0, 1, 3, 0, 2, 3,
		0, 4, 5, 0, 1, 5,
		2, 6, 7, 2, 3, 7,
		0, 4, 6, 0, 2, 6,
		1, 5, 7, 1, 3, 7,
		4, 5, 7, 4, 6, 7 };

	cube_triangles = 12;

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(normals), normals);

	glGenBuffers(1, &cube_ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

	glUseProgram(cubeProgram);
	vPosition = glGetAttribLocation(cubeProgram, "vPosition");
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(cubeProgram, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*) sizeof(vertices));
	glEnableVertexAttribArray(vNormal);
}

void createCubemap(const char* url, GLuint& Buffer) {
	/*
		load texture
	*/
	textureCube = loadCube(url);
	glGenTextures(1, &Buffer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Buffer);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, textureCube->width, textureCube->height,
			0, GL_RGB, GL_UNSIGNED_BYTE, textureCube->data[i]);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void init(const char* file, Mesh& model, GLuint& p) {
	GLuint vbuffer;
	GLuint tBuffer;
	GLint vPosition;
	GLint vNormal;
	GLint vTex;

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

	//find size
	float xmax = 0, xmin = INFINITY;
	float ymax = 0, ymin = INFINITY;
	float zmax = 0, zmin = INFINITY;
	for (int i = 0; i < model.nv; i+=3) {
		//x
		if (xmax < model.vertices[i])
			xmax = model.vertices[i];
		if (xmin > model.vertices[i])
			xmin = model.vertices[i];
		//y
		if (ymax < model.vertices[i + 1])
			ymax = model.vertices[i + 1];
		if (ymin > model.vertices[i + 1])
			ymin = model.vertices[i + 1];
		//z
		if (zmax < model.vertices[i + 2])
			zmax = model.vertices[i + 2];
		if (zmin > model.vertices[i + 2])
			zmin = model.vertices[i + 2];
	}

	model.size = glm::abs(glm::vec3((xmax - xmin)/xmax, (ymax - ymin)/ymax, (zmax - zmin)/zmax));


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

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn + nt) * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
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

	vs = buildShader(GL_VERTEX_SHADER, (char*)"Shaders/vert.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Shaders/frag.fs");
	cirprogram = buildProgram(vs, fs, 0);
	//dumpProgram(program, (char*)"shader program");

	vs = buildShader(GL_VERTEX_SHADER, (char*)"Shaders/boids.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Shaders/boids.fs");
	boidprogram = buildProgram(vs, fs, 0);
	dumpProgram(boidprogram, (char*)"boids program");

	vs = buildShader(GL_VERTEX_SHADER, (char*)"Shaders/obstacle.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Shaders/obstacle.fs");
	obsprogram = buildProgram(vs, fs, 0);
	dumpProgram(obsprogram, (char*)"obs program");

	vs = buildShader(GL_VERTEX_SHADER, (char*)"Shaders/sky.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Shaders/sky.fs");
	cubeProgram = buildProgram(vs, fs, 0);
	dumpProgram(program, (char*)"Sky program");

	init("Meshes/drone.obj", boids, boidprogram);
	//init("Meshes/boids.obj", boids, boidprogram);
	init("Meshes/sphere.obj", sphere, program);
	init("Meshes/circle.obj", circle, cirprogram);
	init("Meshes/voxelBall.obj", vox, obsprogram);
	createSky();
	createCubemap(cubmapUrl,tBuffer);

	//create init position
	initBoidsPos();
	eyex = 0.0;
	eyez = 0.0;
	eyey = 10.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	obstaclePos = glm::vec3((centerSize / 2 + flockSize) * boidCenter.x, 0.0, boidCenter.z);

}

void update() {
	if (animate) {
		//updates
		rot += rotspeed;
		if (rot >= 2*M_PI) {
			rot = 0;
		}

		scater -= 0.1;
		if (scater < 1) {
			scater = 1;
		}

		//	Collisions 
		obstaclePos = glm::vec3((centerSize / 2 + flockSize) * boidCenter.x, 0.0, boidCenter.z);

		for (int i = 0; i < listBoid.size(); i++) {
			float angle = rot + rotationOffset[i];							//	Current Boid Angle
			float angleDiff = abs((angle - obsAngle) / (2 * M_PI));				//	angle distance between Boids & Obstacle
			float px = (listBoid[i].x + shifts[i].x);						//	X position of boid
			float pz = (listBoid[i].z + shifts[i].z);						//	Z position of boid
			float distx = px * cos(angle) - obstaclePos.x * cos(obsAngle);	//	linear-X distace between Boids & Obstacle
			float distz = pz - obstaclePos.z;								//	linear-Z distace between Boids & Obstacle
			 
			float weightx = 1.0f;											//	weight of x
			float weightz = 1.0f;											//	weight of z
			/*
				Weight is caculated by compareing differece of 
				x - distance and z - distance and then appliy
				diference on the multiplier.
			*/
			if (distx > distz) {
				weightx = 1.5 * distx / abs(distx);
				weightz = 0.5 * distz / abs(distz);
			}else {
				weightx = 0.5 * distx / abs(distx);
				weightz = 1.5 * distz / abs(distz);
			}

			//	Before
			/* 
				Due to the differece is repersented by percentage of 2 PI,
				when AngleDiff == 1 the Boids is meet up with Obstacle.	
			*/
			if ( angleDiff > 1 ) {
				if (angleDiff > threshold) {
					if (distx >= 0) {
						shifts[i].x = scatterf(distx, angleDiff) * weightx;
					}else {
						shifts[i].x = scatterf(-distx, angleDiff) * weightx;
					}

					if (distz >= 0) {
						shifts[i].z = scatterf(distz, angleDiff) * weightz;
					}else {
						shifts[i].z = scatterf(-distz, angleDiff) * weightz;
					}
				}
			}
			
			//	After
			else{
				if (angleDiff < 1+threshold) {
					if (distx >= 0) {
						shifts[i].x = convergef(distx, angleDiff) * weightx;
					}else {
						shifts[i].x = convergef(-distx, angleDiff) * weightx;
					}

					if (distz >= 0) {
						shifts[i].z = convergef(distz, angleDiff) * weightz;
					}else {
						shifts[i].z = convergef(-distz, angleDiff) * weightz;
					}
				}
			}
			#ifdef DEBUG
				glm::vec3  p1 = (shifts[0]);
				glm::vec3  p2 = (shifts[1]);
				glm::vec3 dist = glm::abs(p1 - p2);
				if(abs((p1 / p2).x) < 2)
					if(abs((p1 / p2).x) < 0.2)
						printf("%f %f P: %f\n",dist.x,dist.z,(p1/p2).x);

				if (i == 0) {
				
					if (angleDiff > therhold) {
						printf("Sacter: ");
					}
					else{
						printf("Converge:");
					}
					printf("shiftx %d: %f, distx %f,distz %f, angle %f, diff %f\n"
						"",
						i, shifts[i].x, distx, distz, angle, angleDiff);
				}
			#endif	
			//	Self awareness
			for (int j = 0; j < listBoid.size(); j++) {
				glm::vec3  p1 = (listBoid[i]+shifts[i]);
				glm::vec3  p2 = (listBoid[j]+shifts[j]);
				glm::vec3 dist = glm::abs( p1 - p2);
				
				//x
				if (abs((p1 / p2).x) < 2 && abs((p1 / p2).x) > 0)
					if (abs((p1 / p2).x) < 0.2)
						shifts[i].x -= abs((p1 / p2).x) * dist.x /abs(dist.x);
			}
		}
	}
	//	Update Camera
	vx = (float)(r * sin(theta) * cos(phi));
	vy = (float)(r * sin(theta) * sin(phi));
	vz = (float)(r * cos(theta));

	eyepos = glm::vec3(vx, vy, vz);
	
	view = glm::lookAt(eyepos,
		lookCenter,
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//	(Unfinished Function: Focus Look)
	if (isFocus) {
		lookCenter = obstaclePos;
	}else {
		lookCenter = origin;
	}
}

void display(void) {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 skySize = glm::scale(glm::mat4(1.0f),glm::vec3(10.0f));

	glUseProgram(cubeProgram);
	viewLoc = glGetUniformLocation(cubeProgram, "modelView");
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view*skySize));
	projLoc = glGetUniformLocation(cubeProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tBuffer);
	GLuint skyboxTextureId = glGetUniformLocation(cubeProgram, "tex");
	glUniform1i(skyboxTextureId, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);

	glBindVertexArray(cube_VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ibuffer);
	glDrawElements(GL_TRIANGLES, 3 * cube_triangles, GL_UNSIGNED_INT, NULL);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);


	// Center
	glUseProgram(program);

	viewLoc = glGetUniformLocation(program, "modelView");
	projLoc = glGetUniformLocation(program, "projection");
	eyeLoc = glGetUniformLocation(program, "Eye");

	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(glm::max(centerSize * scalef, 0.0f)));

	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view * scale));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));
	   
	glBindVertexArray(sphere.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ibuffer);
	glDrawElements(GL_TRIANGLES, 3 * sphere.triangles, GL_UNSIGNED_INT, NULL);

#ifdef DEBUG
	//circle
	glm::mat4 cirtransform(1.0f);
	float cirround = ((centerSize / 2 + flockSize) * boidCenter).x;
	cirtransform = glm::scale(cirtransform, glm::vec3(cirround, cirround, 1.0));
	cirtransform = glm::rotate(cirtransform,90.0f,glm::vec3(1.0f,0.0,0.0));

	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view * scale *cirtransform));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

	glBindVertexArray(circle.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circle.ibuffer);
	glDrawElements(GL_LINES, 3 * circle.triangles, GL_UNSIGNED_INT, NULL);
#endif // DEBUG
	
	//Obstacle 
	glUseProgram(obsprogram);

	viewLoc = glGetUniformLocation(obsprogram, "modelView");
	projLoc = glGetUniformLocation(obsprogram, "projection");
	eyeLoc = glGetUniformLocation(obsprogram, "Eye");

	glm::mat4 transform(1.0f);

	transform = glm::rotate(transform, obsAngle, glm::vec3(0, 0, 1.0f));
	transform = glm::translate(transform,obstaclePos);
	transform = glm::scale(transform, glm::vec3(0.5));

	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view * scale * transform));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

	glBindVertexArray(vox.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vox.ibuffer);
	glDrawElements(GL_TRIANGLES, 3 * vox.triangles, GL_UNSIGNED_INT, NULL);


	//Boids
	glUseProgram(boidprogram);

	viewLoc = glGetUniformLocation(boidprogram, "modelView");
	projLoc = glGetUniformLocation(boidprogram, "projection");
	eyeLoc = glGetUniformLocation(boidprogram, "Eye");
	GLuint markLoc = glGetUniformLocation(boidprogram, "marked");

	for (int i = 0; i < numBoids; i++) {
		glm::mat4 transform(1.0f);

		transform = glm::rotate(transform, (rot + rotationOffset[i]), glm::vec3(0, 0, 1.0f));

		transform = glm::translate(transform, scater * (listBoid[i] + shifts[i]));
		
		transform = glm::rotate(transform,90.0f, glm::vec3(1.0,0.0,0.0));

		transform = glm::scale(transform, glm::vec3(glm::max(boidsSize, 0.0f)));

		glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view *scale * transform));
		glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
		glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));
		if (i == 0) 
			glUniform1i(markLoc, 1);
		else 
			glUniform1i(markLoc, 0);

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
	scalef += scaleChange;
	scalef = scalef > 0 ? scalef : (0.000000001f);
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
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		if (action == GLFW_REPEAT) {
			if (key == GLFW_KEY_EQUAL) {
				rotspeed += 0.001;
				printf("set rotaion speed %f\n", rotspeed);
			}
			else if (key == GLFW_KEY_MINUS) {
				rotspeed -= 0.001;
				printf("set rotaion speed %f\n", rotspeed);
			}
		}
		else if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_R) {
				initBoidsPos();
				scater = 0;
			}
		/*	if (key == GLFW_KEY_S)
				scater += 10;*/
			if (key == GLFW_KEY_EQUAL) {
				rotspeed += 0.0001;
				printf("set rotaion speed %f\n", rotspeed);
			}
			else if (key == GLFW_KEY_MINUS) {
				rotspeed -= 0.0001;
				printf("set rotaion speed %f\n", rotspeed);
			}
			if (key == GLFW_KEY_SPACE)
				animate = !animate;
		}
		if (rotspeed <= 0) {
			rotspeed = 0;
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
