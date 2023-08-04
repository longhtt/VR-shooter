// Demo-5-Mesh.cpp - cube with textured image

#include <glad.h>										// OpenGL access 
#include <glfw3.h>										// application framework
#include "Camera.h"										// view transforms on mouse input
#include "GLXtras.h"									// SetUniform
#include "Mesh.h"										// cube.Read
#include "Misc.h"										// Shift

int winWidth = 700, winHeight = 700;
Camera camera(0, 0, winWidth, winHeight, vec3(0,0,0), vec3(0,0,-10), 30, .001f, 500);

Mesh cube;
const char *meshFile = "C:/Users/longt/Code/Assets/Models/pistol.obj";
const char *textureFile = "C:/Users/longt/Code/Assets/Images/pistol.png";

void Display(GLFWwindow *w) {
	glClearColor(.5f, .5f, .5f, 1);						// set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear background and z-buffer
	glEnable(GL_DEPTH_TEST);							// see only nearest surface
	SetUniform(UseMeshShader(), "useLight", false);		// disable shading
	cube.Display(camera, 0);							// draw mesh with camera transform
	glFlush();											// finish
}

void MouseButton(float x, float y, bool left, bool down) {
	if (down) camera.Down(x, y, Shift()); else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) camera.Drag(x, y);
}

void MouseWheel(float spin) { camera.Wheel(spin, Shift()); }

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "3D-Mesh");
	cube.Read(string(meshFile), string(textureFile));
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	printf("mouse-drag:  rotate\nwith shift:  translate xy\nmouse-wheel: translate z");
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
}
