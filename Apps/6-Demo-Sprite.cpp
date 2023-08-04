// Sprite.cpp - 2D display and manipulate texture-mapped quad

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include "Sprite.h"

Sprite background, actor;

// Mouse

void MouseWheel(float spin) { actor.Wheel(spin, false); }

void MouseButton(float x, float y, bool left, bool down) { if (left && down) actor.Down(x, y); }

void MouseMove(float x, float y, bool leftDown, bool rightDown) { if (leftDown) actor.Drag(x, y); }

// Application

void Resize(int width, int height) { glViewport(0, 0, width, height); }

int main(int ac, char **av) {
	GLFWwindow *w = InitGLFW(100, 100, 600, 600, "Sprite Demo");
	// read background, foreground, and mat textures
	background.Initialize("C:/Users/Jules/Code/Assets/Images/Earth.tga");
	actor.Initialize("C:/Users/Jules/Code/Assets/Images/MattedNumber1.png");
	// callbacks
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	printf("mouse drag to move sprite\n");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		background.Display();
		actor.Display();
		glFlush();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
