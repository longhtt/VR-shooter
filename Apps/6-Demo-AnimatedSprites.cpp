// AnimatedSprites.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Sprite.h"

Sprite backgroundSprite, numberSprite(vec2(-.3f, -.2f), .2f), letterSprite(vec2(.35f, 0.f), .3f);
Sprite *selectedSprite = NULL;
time_t start = clock();

// Mouse

void MouseWheel(float spin) { if (selectedSprite) selectedSprite->Wheel(spin, false); }

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) {
		selectedSprite = numberSprite.Hit(x, y)? &numberSprite : letterSprite.Hit(x, y)? &letterSprite : NULL;
		if (selectedSprite)
			selectedSprite->Down(x, y);
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown)
		if (selectedSprite)
			selectedSprite->Drag(x, y);
}

// Sprite Intersection

void GetPts(Sprite s, vec2 *pts) {
	vec4 x1 = s.ptTransform*vec4(-1,-1,0,1), x2 = s.ptTransform*vec4(-1,+1,0,1),
		 x3 = s.ptTransform*vec4(+1,+1,0,1), x4 = s.ptTransform*vec4(+1,-1,0,1);
	pts[0] = vec2(x1.x, x1.y); pts[1] = vec2(x2.x, x2.y);
	pts[2] = vec2(x3.x, x3.y); pts[3] = vec2(x4.x, x4.y);
}

void Outline(Sprite s) {
	vec2 pts[4];
	GetPts(s, pts);
	for (int i = 0; i < 4; i++) Line(pts[i], pts[(i+1)%4], 3, vec3(1, 1, 0));
}

vec2 Min(vec2 a, vec2 b) { return vec2(a.x < b.x? a.x : b.x, a.y < b.y? a.y : b.y); }
vec2 Max(vec2 a, vec2 b) { return vec2(a.x > b.x? a.x : b.x, a.y > b.y? a.y : b.y); }

bool SpritesIntersect(Sprite &s1, Sprite &s2) {
	vec2 pts1[4], pts2[4];
	GetPts(s1, pts1);
	GetPts(s2, pts2);
	vec2 min1(FLT_MAX), max1(-FLT_MAX), min2(FLT_MAX), max2(-FLT_MAX);
	for (int i = 0; i < 4; i++) {
		vec2 p1 = pts1[i], p2 = pts2[i];
		min1 = Min(min1, p1); min2 = Min(min2, p2);
		max1 = Max(max1, p1); max2 = Max(max2, p2);
	}
	bool xmiss = min1.x > max2.x || max1.x < min2.x;
	bool ymiss = min1.y > max2.y || max1.y < min2.y;
	return !xmiss && !ymiss;
}

// Display

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	backgroundSprite.Display();
	numberSprite.Display();
	float dt = (float)(clock()-start)/CLOCKS_PER_SEC;
	int f = ((int) (dt/.15)) % 4;
	letterSprite.SetUvTransform(Translate((float)f/4, 0, 0)*Scale(.25, 1, 1));
	letterSprite.Display();
	if (SpritesIntersect(letterSprite, numberSprite)) {
		Outline(letterSprite);
		Outline(numberSprite);
	}
	glFlush();
}
	
// Application

void Resize(int width, int height) { glViewport(0, 0, width, height); }

int main(int ac, char **av) {
	GLFWwindow *w = InitGLFW(100, 100, 600, 600, "Animated Sprites");
	// read background, foreground, and mat textures
	std::string dir = "C:/Users/Jules/Code/Assets/Images/", background = dir+"Lily.jpg";
	std::string mat = dir+"Circle-Mat.jpg", numbers[] = {"00", "01", "02", "03", "04"}, letters = dir+"A-D.jpg";
	std::vector<std::string> numberNames(5);
	for (int i = 0; i < 5; i++)
		numberNames[i] = dir+numbers[i]+".jpg";
	backgroundSprite.Initialize(background.c_str());
	numberSprite.Initialize(numberNames, mat.c_str());
	letterSprite.Initialize(letters.c_str(), mat.c_str());
	printf("Usage:\n  mouse drag: move sprite\n  mouse wheel: scale sprite\n");
	// callbacks
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
