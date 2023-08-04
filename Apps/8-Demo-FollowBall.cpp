// FollowBall.cpp: rotate/translate textured sphere on textured quad, jump on 'j'

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "IO.h"
#include "GLXtras.h"
#include "Sphere.h"
#include "VecMat.h"
#include "Widgets.h"

typedef SphereVertex Vertex;

const int		RADIUS = 1;
const float		KEY_DIST = .05f;

// shading ids for program, vertices, textures
GLuint			program = 0;
GLuint			vBuffer[] = { 0, 0 };
GLuint			textureUnit[] = { 0, 1 }, textureName[] = { 0, 0 };

// interactive lighting
vec3			light(3.25f, 1.5f, 4.7f);
Mover			mover;
void		   *picked = NULL;

// ground parallel to xy-plane
float			grndSize = 10, grndHght = 0.5f;
Vertex			p1 = Vertex(vec3(-grndSize, -grndSize, grndHght), vec3(0, 0, 1), vec2(0, 0));
Vertex			p2 = Vertex(vec3( grndSize, -grndSize, grndHght), vec3(0, 0, 1), vec2(1, 0));
Vertex			p3 = Vertex(vec3( grndSize,  grndSize, grndHght), vec3(0, 0, 1), vec2(1, 1));
Vertex			p4 = Vertex(vec3(-grndSize,  grndSize, grndHght), vec3(0, 0, 1), vec2(0, 1));
Vertex			quadVertices[4] = { p1, p2, p3, p4 };

// display
int				winX = 100, winY = 100, winWidth = 800, winHeight = 800;
Camera			camera(winX, winY, winWidth, winHeight, vec3(-60,0,0), vec3(0,0,-20));
bool			followBall = false;

// unit sphere
float			ballZ = 1, ballRadius = .5;
vec3			ballPosition = vec3(0, 0, ballZ);
float			dAnglePerSec = 360;
Quaternion		ballOrientation(0, 0, 0, 1);
vector<Vertex>	ballVertices;
float			ballRotX = 0, ballRotY = 0; // **** BOGUS - don't use Eulers; use matrix muls or quaternions
float			degRotate = 5;

// animation
time_t prevTime = clock(), jumpTime = prevTime;
bool directionRight = true, jumpEnable = false, autoRoll = false;
float jumpDuration = .4f, jumpHeight = 1.5f;

// vertex shader
const char *vertexShader = R"(
	#version 130
	in vec3 point, normal;
	in vec2 uv;
	out vec3 vPoint, vNormal;
	out vec2 vUv;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vNormal = (modelview*vec4(normal, 0)).xyz;
		vUv = uv;
	}
)";

// pixel shader with Phong shading
const char *fragmentShader = R"(
	#version 130
	struct Ray {vec3 b, v;};
	in vec3 vPoint, vNormal;
	in vec2 vUv;
	out vec4 pColor;
	uniform vec3 light;
	uniform bool infiniteLight = false, doShadow = false;
	uniform vec4 ball;
	uniform sampler2D textureImage;
	uniform float textureScale = 1;
	float RaySphere(Ray r, vec4 s) {
		// return least pos alpha of ray and sphere (or -1 if none)
		vec3 q = r.b-s.xyz;
		float vDot = dot(r.v, q);
		float sq = vDot*vDot-dot(q, q)+s.w*s.w;
		if (sq < 0) return -1;
		float root = sqrt(sq), a = -vDot-root;
		return a > 0? a : -vDot+root;
	}
	void main() {
		vec3 N = normalize(vNormal);								// surface normal
		vec3 L = normalize(infiniteLight? light : light-vPoint);	// light vector
		vec3 E = normalize(vPoint);									// eye vector
		vec3 R = reflect(L, N);										// highlight vector
		float d = abs(dot(N, L));									// two-sided diffuse
		float s = abs(dot(R, E));									// two-sided specular
		float intensity = clamp(d+pow(s, 50), 0, 1);
		vec2 uvScaled = vec2(textureScale*vUv);
		if (doShadow) {
			Ray r = Ray(vPoint, L);
			float a = RaySphere(r, ball);
			if (a >= 0) intensity = .5*intensity;
		}
		pColor = vec4(intensity*texture(textureImage, uvScaled).rgb, 1);
	}
)";

// Display

void DrawBall() {
	// load transformation matrix
	mat4 ballXform = camera.modelview*Translate(ballPosition)*Scale(.75f)*RotateX(ballRotX)*RotateY(ballRotY);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelview"), 1, true, (float *) &ballXform[0][0]);
	// set texture image
	glActiveTexture(GL_TEXTURE0+textureUnit[0]);    // active texture corresponds with textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName[0]);
	SetUniform(program, "textureImage", (int) textureUnit[0]);
	SetUniform(program, "textureScale", 1.f);
	SetUniform(program, "doShadow", false);
	SetUniform(program, "infiniteLight", false);
	// connect ball vertex buffer to vertex shader inputs
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer[0]);
	int pointId = EnableVertexAttribute(program, "point");
	int normalId = EnableVertexAttribute(program, "normal");
	int uvId = EnableVertexAttribute(program, "uv");
	glVertexAttribPointer(pointId,  3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
	glVertexAttribPointer(normalId, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(vec3)));
	glVertexAttribPointer(uvId,     2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (2*sizeof(vec3)));
	glDrawArrays(GL_TRIANGLES, 0, ballVertices.size());
}

void DrawGround() {
	// load transformation matrix
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// set texture image and scale
	glActiveTexture(GL_TEXTURE0+textureUnit[1]);    // active texture corresponds with textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName[1]);
	SetUniform(program, "textureImage", (int) textureUnit[1]);
	SetUniform(program, "textureScale", 1.f);
	// enable shadow testing (needs transformed ball position)
	SetUniform(program, "doShadow", true);
	vec4 xBallR(Vec3(camera.modelview*vec4(ballPosition, 1)), ballRadius);
	SetUniform(program, "ball", xBallR);
	// connect ground vertex buffer to vertex shader inputs
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer[1]);
	VertexAttribPointer(program, "point", 3, sizeof(Vertex), (void *) 0);
	VertexAttribPointer(program, "normal", 3, sizeof(Vertex), (void *) sizeof(vec3));
	VertexAttribPointer(program, "uv", 2, sizeof(Vertex), (void *) (2*sizeof(vec3)));
	glDrawArrays(GL_QUADS, 0, 4);
}

void Display(GLFWwindow *w) {
	glUseProgram(program);
	// update matrices and send perspective matrix to vertex shader
	glUniformMatrix4fv(glGetUniformLocation(program, "persp"), 1, true, camera.persp); // (float *) &persp[0][0]);
	// transform light and send to pixel shader
	vec3 xLight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(program, "light", xLight);
	// set screen grey, enable transparency, use z-buffer
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT |  GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// draw objects and finish
	DrawGround();
	DrawBall();
	UseDrawShader(camera.fullview);
	if (IsVisible(light, camera.fullview))
		Disk(light, 12, vec3(1,0,0));
   glFlush();
}

// Animation

vec3 SetBall(unsigned char key) {
	vec3 move;
	float lim = grndSize+RADIUS;
	if (key == 'W' && (ballPosition.y+KEY_DIST) < lim) {
		// up arrow: negative x-rotation
		ballRotX += -degRotate;
		move = vec3(0, KEY_DIST, 0);
	}
	if (key == 'S' && (ballPosition.y-KEY_DIST) > -lim) {
		ballRotX += degRotate;
		move = vec3(0, -KEY_DIST, 0);
	}
	if (key == 'A' && (ballPosition.x+KEY_DIST) < lim) {
		ballRotY += -degRotate;
		move = vec3(-KEY_DIST, 0, 0);
	}
	if (key == 'D' && (ballPosition.x-KEY_DIST) > -lim) {
		ballRotY += degRotate;
		move = vec3(KEY_DIST, 0, 0);
	}
	ballPosition += move;
	return move;
}

void AnimateBall() {
	time_t now = clock();
	float dt = (float)(now-prevTime)/CLOCKS_PER_SEC;
	float dj = (float)(now-jumpTime)/CLOCKS_PER_SEC;
	prevTime = now;
	if (jumpEnable && now > jumpTime && dj < jumpDuration) {
		float dr = dj/jumpDuration, d = 1-2*abs(.5f-dr);
		ballPosition.z = ballZ+d*d*jumpHeight;
	}
	else ballPosition.z = ballZ;
	ballRotY += (directionRight? 1 : -1) * 20 * degRotate*dt;
	ballPosition.x += (directionRight? 1 : -1) * 20 * KEY_DIST*dt;
	if (ballPosition.x > 2 && directionRight || ballPosition.x < -2 && !directionRight)
		directionRight = !directionRight;
}

// Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		if (MouseOver((float) x, (float) y, light, camera.fullview)) {
			picked = &mover;
			mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
		}
		else {
			picked = &camera;
			camera.Down((int) x, (int) y, Shift());
		}
	}
	if (!down) camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == &mover) mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera) camera.Drag((int) x, (int) y);
	}
}

void MouseWheel(float spin) { camera.Wheel(spin, Shift()); }

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press) {
		if (key == 'R') autoRoll = !autoRoll;
		if (key == 'J') { jumpEnable = true; jumpTime = clock(); }
		if (key == 'F') followBall = !followBall;
		if (key == 'P') printf("light = (%3.2f, %3.2f, %3.2f)\n", light.x, light.y, light.z);
	}
	vec3 move = SetBall(key);
	if (followBall) camera.Move(move);
}

void Resize(int width, int height) { glViewport(0, 0, width, height); }

// Application

void BuildObjects() {
	// build unit sphere
	UnitSphere(20, &ballVertices);
	// create two GPU vertex buffers
	glGenBuffers(2, vBuffer);
	// allocate and download ball
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, ballVertices.size()*sizeof(SphereVertex), &ballVertices[0], GL_STATIC_DRAW);
	// allocate and download ground
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
}

int main(int argc, char **argv) {
	GLFWwindow *w = InitGLFW(winX, winY, winWidth, winHeight, "Follow Ball");
	// build shader program
	program = LinkProgramViaCode(&vertexShader, &fragmentShader);
	if (!program) {
		printf("Can't link shaderId program\n");
		getchar();
		return 0;
	}
	BuildObjects();
	// read texture images, create mipmaps, link to pixel shader
	textureName[0] = ReadTexture("C:/Users/Jules/Code/Assets/Images/Lily.tga");
	textureName[1] = ReadTexture("C:/Users/Jules/Code/Assets/Images/Ground.tga");
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf("usage:\n\twasd: move\n\tr: auto-roll\n\tj: jump\n\tf: follow\n\tp: print light\n");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		if (autoRoll)
			AnimateBall();
		Display(w);
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(2, vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
	return 0;
}
