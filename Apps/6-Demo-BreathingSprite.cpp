// BreathingSprite.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "Sprite.h"

Sprite background, actor;
time_t start = clock();

void Breathe() {
	float breathDuration = 2; // in seconds
	float bias = 1, amplitude = .5f;
	float dt = (float)(clock()-start)/CLOCKS_PER_SEC;
	float angle = 2*3.141592f*dt/breathDuration;
	float vscale = bias+amplitude*(1+sin(angle))/2;
	actor.SetScale(vec2(.7f, .7f*vscale));
}

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	background.Display();
	actor.Display();
	glFlush();
}

void Resize(int width, int height) { glViewport(0, 0, width, height); }

int main() {
	GLFWwindow *w = InitGLFW(100, 100, 600, 600, "Breathing Sprite");
	RegisterResize(Resize);
	// read background, actor sprites
	background.Initialize("C:/Users/Jules/Code/Assets/Images/Earth.tga");
	actor.Initialize("C:/Users/Jules/Code/Assets/Images/MattedNumber1.png");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Breathe();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
