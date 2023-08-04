// StepbackSprite.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "Sprite.h"

int winWidth = 600, winHeight = 600;
Sprite background, actor;

// step back
bool released = false;
time_t releaseTime = 0;
vec2 startLoc, releaseLoc;

void Stepback() {
	if (!released) return;
	float stepPause = 1.5f, stepDuration = 1; // in seconds
	float dtRelease = (float)(clock()-releaseTime)/CLOCKS_PER_SEC;
	if (dtRelease < stepPause) return;
	float dtStep = dtRelease-stepPause;
	if (dtStep > stepDuration) return;
	float t = dtStep/stepDuration;
	vec2 newLoc = releaseLoc+t*(startLoc-releaseLoc);
	actor.SetPosition(newLoc);
}

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	background.Display();
	actor.Display();
	glFlush();
}

void MouseButton(float x, float y, bool left, bool down) {
	if (down) {
		startLoc = actor.GetPosition();
		actor.Down(x, y);
	}
	else {
		released = true;
		releaseTime = clock();
		releaseLoc = actor.GetPosition();
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) { if (leftDown) actor.Drag(x, y); }

void Resize(int width, int height) { glViewport(0, 0, winWidth = width, winHeight = height); }

int main() {
	// init app window, GL context, callbacks
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Sprite Stepback");
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterResize(Resize);
	// read background, foreground sprites
	background.Initialize("C:/Users/longt/Code/Assets/Images/Earth.tga");
	actor.Initialize("C:/Users/longt/Code/Assets/Images/Matted-32bit.png");
	actor.SetScale(vec2(.5f, .5f));
	printf("move and the sprite returns\n");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Stepback();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
