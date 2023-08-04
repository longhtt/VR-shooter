// 3D-Object-and-Background.cpp: display quad as ground and 3D OBJ model with texture

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Mesh.h"
#include "Sprite.h"
#include "Widgets.h"

// display
int         winW = 600, winH = 600;
Camera	    camera(0, 0, winW, winH, vec3(20, 20, 0), vec3(0, 0, -5));

// interaction
vec3        light(-.2f, .4f, .3f);
vec3		lightAt = light+vec3(.5f, 0, 0);
Framer      framer;								// position/orient mesh
Mover       mover;								// position light
void       *picked = &camera;
time_t		mouseMoved;

// scene
Mesh		mesh;
Sprite		ground;
string		catObj = "C:/Users/longt/Code/Assets/Models/Pistol_obj.obj";
string		catTex = "C:/Users/longt/Code/Assets/Images/Pistol_color.jpg";
const char *groundTex = "C:/Users/longt/Code/Assets/Images/marble_floor.jpg";

// Display

void Display() {
	// clear screen, depth test, blend
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	// update light
	int s = UseMeshShader();
	vec3 xlight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(s, "nLights", 1);
	SetUniform3v(s, "lights", 1, (float *) &xlight);
	// display objects
	ground.Display(&camera.fullview);
	mesh.Display(camera);
	// lights and frames
	if ((clock()-mouseMoved)/CLOCKS_PER_SEC < 1.f) {
		glDisable(GL_DEPTH_TEST);
		UseDrawShader(camera.fullview);
		Disk(light, 9, vec3(1,1,0));
		Disk(lightAt, 9, vec3(1,0,0));
		vec3 endArrow = .5f*normalize(lightAt-light);
		ArrowV(light, endArrow, camera.modelview, camera.persp, vec3(1,1,0), 1, 6);
		mat4 &f = mesh.toWorld;
		vec3 base(f[0][3], f[1][3], f[2][3]);
		Disk(base, 9, vec3(1, 1, 1));
		if (picked == &framer)
			framer.Draw(camera.fullview);
		if (picked == &camera)
			camera.arcball.Draw();
		vec4 p1(light, .5f), p2(lightAt, .1f);
		Cylinder(light, light+.5f*endArrow, 0, .3f, camera.modelview, camera.persp, vec4(1, 0, 0, .5f));
	}
	glFlush();
}

// Mouse

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) {
		int ix = (int) x, iy = (int) y;
		void *newPicked = NULL;
		if (ScreenD(x, y, light, camera.fullview) < 12) {
			newPicked = &mover;
			mover.Down(&light, ix, iy, camera.modelview, camera.persp);
		}
		if (ScreenD(x, y, lightAt, camera.fullview) < 12) {
			newPicked = &mover;
			mover.Down(&lightAt, ix, iy, camera.modelview, camera.persp);
		}
		if (!newPicked) {
			// test for mesh base hit
			vec3 base(mesh.toWorld[0][3], mesh.toWorld[1][3], mesh.toWorld[2][3]);
			if (MouseOver(x, y, base, camera.fullview)) {
				newPicked = &framer;
				framer.Set(&mesh.toWorld, 100, camera.fullview);
				framer.Down(ix, iy, camera.modelview, camera.persp);
			}
			}
		if (!newPicked && picked == &framer && framer.Hit(ix, iy)) {
			framer.Down(ix, iy, camera.modelview, camera.persp);
			newPicked = &framer;
		}
		picked = newPicked;
		if (!picked) {
			picked = &camera;
			camera.Down(x, y);
		}
	}
	if (!down) {
		if (picked == &camera)
			camera.Up();
		if (picked == &framer)
			framer.Up();
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	mouseMoved = clock();
	if (leftDown) {
		int ix = (int) x, iy = (int) y;
		if (picked == &mover)
			mover.Drag(ix, iy, camera.modelview, camera.persp);
		if  (picked == &framer)
			framer.Drag(ix, iy, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	if (picked == &framer)
		framer.Wheel(spin, Shift());
	if (picked == &camera)
		camera.Wheel(spin, Shift());
}

// Application

void Resize(int width, int height) {
	glViewport(0, 0, winW = width, winH = height);
	camera.Resize(width, height);
}

int main(int ac, char **av) {
	// init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winW, winH, "Mesh And Sprites");
	// read sprite, mesh, texturre
	mesh.Read(catObj, catTex);
	ground.Initialize(groundTex);
	ground.SetPtTransform(Scale(1.5f, 1, 1.5f)*Translate(0, -.55f, 0)*RotateX(-90));
//	Resize(w, winW, winH);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &mesh.vBufferId);
	glfwDestroyWindow(w);
	glfwTerminate();
}
