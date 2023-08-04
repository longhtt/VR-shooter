// CollidingSprites.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Sprite.h"
#include "Text.h"

string dir("C:/Users/longt/Code/Assets/Images/");
string bgrnd = dir+"marble_floor.jpg";
string pix1 = dir+"MattedAlienSlime.png";
string pix2 = dir+"MattedNumber1.png";
string pix3 = dir+"MushZoom.png";

Sprite background, sprite1(vec2(-.3f, -.2f), .2f), sprite2(vec2(.35f, 0.f), .2f), sprite3(vec2(0.f, .2f), .3f);
vector<Sprite *> sprites = { &sprite1, &sprite2, &sprite3 };
Sprite *selectedSprite = NULL;
bool showOutlines = false;

// Display

void Outline(Sprite &s, float width = 2, vec3 color = vec3(1,1,0)) {// if(true)return;
	UseDrawShader(mat4());
	vec2 pts[] = {s.PtTransform({-1,-1}), s.PtTransform({-1,1}), s.PtTransform({1,1}), s.PtTransform({1,-1})};
	for (int i = 0; i < 4; i++)
		Line(pts[i], pts[(i+1)%4], width, color);
}

void ShowCollide(Sprite &s) {
	char buf[500] = {0};
	for (int c : s.collided)
		if (c >= 0)
			sprintf(buf+strlen(buf), "%s%i", buf[0]? "," : "", c);
	vec2 p = ScreenPoint(vec3(s.PtTransform({-1,-1})), mat4());
	Text(p.x, p.y, vec3(1,1,0), 16, buf);
}

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// background
	background.Display();
	// display foreground sprites, testing for collision
	int nHitPixels = TestCollisions(sprites);
	if (showOutlines)
		for (Sprite *s : sprites) {
			bool hit = false;
			for (int i = 0; i < (int) s->collided.size(); i++)
				if (s->collided[i] > -1) {
					hit = true;
					Outline(*sprites[i], 2, vec3(1,1,1));
				}
			if (hit)
				Outline(*s, 2, vec3(1,0,1));
		}
	for (Sprite *s : sprites)
		ShowCollide(*s);
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(ScreenMode());
	int w = 145+16*((int)log10(nHitPixels)), y = VPh()-40;
	Quad(0, y, 0, y+40, w, y+40, w, y, true, vec3(0,0,1), .4f);
	Text(10, y+10, vec3(1), 16, "%i hit pixels", nHitPixels);
	// depth test needed for mouse selection
	glEnable(GL_DEPTH_TEST);
	glFlush();
}
	
// Mouse

void MouseButton(float x, float y, bool left, bool down) {
	if (down) {
		selectedSprite = NULL;
		for (Sprite *s : sprites)
			if (s->Hit(x, y))
				selectedSprite = s;
		if (selectedSprite)
			selectedSprite->Down(x, y);
	}
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown)
		if (selectedSprite)
			selectedSprite->Drag(x, y);
}

void MouseWheel(float spin) {
	if (selectedSprite)
		selectedSprite->Wheel(spin, false);
}

// Application

void Resize(int width, int height) { glViewport(0, 0, width, height); }

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'O') showOutlines = !showOutlines;
}

const char *usage = R"(Usage:
	mouse drag: move sprite
	mouse wheel: scale sprite
	O: outlines on/off
)";

int main(int ac, char **av) {
	// init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, 600, 600, "Colliding Sprites");
	// read sprites
	background.Initialize(bgrnd, .9f);
	sprite1.Initialize(pix1, .5f); sprite1.SetScale(.3f);
	sprite2.Initialize(pix2, .6f); sprite2.SetScale(.4f);
	sprite3.Initialize(pix3, .7f); sprite3.SetScale(.5f);
	// callbacks
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf(usage);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
