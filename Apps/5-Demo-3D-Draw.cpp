// Demo-5-Draw.cpp - 3D line-drawing

#include <glad.h>               // access to OpenGL routines
#include <glfw3.h>				// framework for application
#include "Camera.h"				// create view transformations on mouse input
#include "Draw.h"				// line-drawing
#include "GLXtras.h"            // GLSL convenience routines
#include "Misc.h"               // Shift

// window and camera
int winWidth = 700, winHeight = 700;
Camera camera(0, 0, winWidth, winHeight, vec3(0,0,0), vec3(0,0,-10));

// cube
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1;					// left, right, bottom, top, near, far
vec3 points[] = {{l, b, n}, {l, b, f}, {l, t, n}, {l, t, f},
				 {r, b, n}, {r, b, f}, {r, t, n}, {r, t, f}};		// 8 cube corners
vec3 colors[] = {{1,1,1}, {0,0,0}, {1,0,0}, {0,1,0},				// white, black, red, green
				 {0,0,1}, {1,1,0}, {1,0,1}, {0,1,1}};				// blue, yellow, magenta, cyan

// Display

void DrawQuad(int i1, int i2, int i3, int i4, float w = 7) {
	Line(points[i1], points[i2], w, colors[i1], colors[i2]);
	Line(points[i2], points[i3], w, colors[i2], colors[i3]);
	Line(points[i3], points[i4], w, colors[i3], colors[i4]);
	Line(points[i4], points[i1], w, colors[i4], colors[i1]);
}

void Display(GLFWwindow *w) {
	glClearColor(.5f, .5f, .5f, 1);                                 // set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);             // clear background and z-buffer
	glEnable(GL_DEPTH_TEST);										// view only nearest surface
	enum { lbn=0, lbf, ltn, ltf, rbn, rbf, rtn, rtf };
	UseDrawShader(camera.fullview);
	DrawQuad(lbf, ltf, ltn, lbn);									// left face
	DrawQuad(rtn, rtf, rbf, rbn);									// right
	DrawQuad(rbn, rbf, lbf, lbn);									// bottom
	DrawQuad(ltf, rtf, rtn, ltn);									// top
	DrawQuad(ltn, rtn, rbn, lbn);									// near
	DrawQuad(rbf, rtf, ltf, lbf);									// far
	glFlush();														// finish
}

// Mouse Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) camera.Down(x, y, Shift()); else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) camera.Drag(x, y);
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Application

void Resize(int width, int height) {
	camera.Resize(winWidth = width, winHeight = height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	GLFWwindow *w = InitGLFW(100, 400, winWidth, winHeight, "3D-ShaderFree");
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
