// MeshAndSpritesApp.cpp -  display of 2D sprites and 3D meshes

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Mesh.h"
#include "Misc.h"
#include "Sprite.h"
#include "VecMat.h"

using std::string;

class FallingSprite : public Sprite {
public:
	vec3 position;
	float fallingRate = 1;
	void SetTransform() { ptTransform = Translate(position)*Scale(.2f); }
};

class RotatingMesh : public Mesh {
public:
	vec3 position, rotation, dRotation;
	void SetTransform() { toWorld = Translate(position)*Scale(.5f)*RotateX(rotation.x)*RotateY(rotation.y)*RotateZ(rotation.z); }
};

// sprites
Sprite background;
const int nFallingSprites = 4;
FallingSprite fallingSprites[nFallingSprites];

// meshes
string meshNames[] = {"Bench", "Cat"};
const int nMeshes = sizeof(meshNames)/sizeof(string);
RotatingMesh meshes[nMeshes];

// display
int winW = 600, winH = 600;
Camera camera(0, 0, winW, winH, vec3(0,0,0), vec3(0,0,-5));

// interaction
float objectX = 0, objectY = 0, objectScale = 1;

// interaction
vec3        light(-.2f, .4f, .3f);
Mover       mover;
void       *picked = &camera;

// animation
time_t prevTime = clock();

// Mouse

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) {
		picked = NULL;
		if (MouseOver(x, y, light, camera.fullview)) {
			picked = &mover;
			mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
		}
		if (!picked) {
			picked = &camera;
			camera.Down(x, y, Shift());
		}
	}
	if (!down) camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) { // drag
		if (picked == &mover)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) { if (picked == &camera) camera.Wheel(spin, Shift()); }

// Display and Animation

void Animate() {
	time_t now = clock();
	float dt = (float)(now-prevTime)/CLOCKS_PER_SEC;
	prevTime = now;
	// animate sprites
	for (int i = 0; i < nFallingSprites; i++) {
		FallingSprite &f = fallingSprites[i];
		f.position.y -= dt*f.fallingRate;
		if (f.position.y < -1)
			f.position.y = 1;
		f.SetTransform();
	}
	// animate meshes
	for (int i = 0; i < nMeshes; i++) {
		RotatingMesh &m = meshes[i];
		m.rotation += dt*m.dRotation;
		m.SetTransform();
	}
}

void Display() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	background.Display();
	int s = UseMeshShader();
	// update light
	vec3 xlight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(s, "nLights", 1);
	SetUniform3v(s, "lights", 1, (float *) &xlight);
	// draw meshes
	glEnable(GL_DEPTH_TEST);
	for (int i = 0; i < nMeshes; i++)
		meshes[i].Display(camera);
	// draw sprites
	for (int i = 0; i < nFallingSprites; i++)
		fallingSprites[i].Display();
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.fullview);
	Disk(light, 9, vec3(1, 1, 0));
	glFlush();
}

// Application

float Random(float mn, float mx) { return mn+((float)(rand()%1000)/1000.f)*(mx-mn); }

vec3 RandomVec() { return vec3(Random(-1, 1), Random(-1, 1), Random(-1, 1)); }

string dirImages = "C:/Users/longt/Code/Assets/Images/";
string dirMeshes = "C:/Users/longt/Code/Assets/Models/";

void ReadSprites() {
	// read background, set sprites
	background.Initialize(dirImages+"Earth.tga");
	srand((unsigned) time(NULL));
	for (int i = 0; i < nFallingSprites; i++) {
		FallingSprite &f = fallingSprites[i];
		f.Initialize(dirImages+"Lily.tga", dirImages+"Mat.tga");
		float scale = Random(.2f, .5f);
		f.fallingRate = Random(.1f, 1.25f);
		f.position = RandomVec();
		f.SetTransform();
	}
}

void Resize(int width, int height) { glViewport(0, 0, winW = width, winH = height); }

void ReadMeshes() {
	for (int i = 0; i < nMeshes; i++) {
		RotatingMesh &m = meshes[i];
		string name = meshNames[i], objName = dirMeshes+name+".obj", texName = dirImages+name+".tga";
		//string name = meshNames[i], objName = dirMeshes + name + ".obj", texName = dirImages + name + ".tga";
		m.Read(objName, texName);
		m.position = RandomVec();
		m.rotation = 20*RandomVec();
		m.dRotation = 20*RandomVec();
		m.SetTransform();
	}
}

int main(int ac, char **av) {
	// init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winW, winH, "Mesh And Sprites");
	// init characters
	ReadSprites();
	ReadMeshes();
	// callbacks
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Animate();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &background.textureName);
	for (int i = 0; i < nFallingSprites; i++) {
		glDeleteBuffers(1, &fallingSprites[i].textureName);
		glDeleteBuffers(1, &fallingSprites[i].matName);
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
