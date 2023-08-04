// VR-Game-Stub.cpp

#include <glad.h>
#include <glfw3.h>
#include <openvr.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Mesh.h"
#include "Misc.h"
#include "VRXtras.h"

// HMD res, app size
bool		hmdActive = false;
int			bufW = 1024, bufH = 768;
float		aspectRatio = (float) bufW/bufH;
int			eyeH = 330, eyeW = (int) (eyeH*aspectRatio), winW = 2*eyeW, winH = 3*eyeH;

// GL indices
GLuint		displayProgram = 0, fbTextureUnits[] = { 2, 3 }, fbTextureNames[] = { 0, 0 };

// VR initialization, frame buffer for eyes
VROOM		vroom;

// cameras, light
Quaternion	initialOrientation(-.17f, .42f, .09f, .88f);
Camera		cameraScene(0, 0, winW, winH-eyeH, initialOrientation, vec3(0, 0, -5));
Camera		cameraUser(0, 0, bufW, bufH);
vec3		light(-.2f, .4f, .3f), lookAt(-.35f, -.15f, .55f);

// interaction
Framer		framer;	// move or orient head
Mover		mover;	// move lookAt, light, left-hand or right-hand
void	   *picked = NULL;
time_t		mouseEvent = clock();

// meshes
Mesh		bench, ground, head, leftHand, rightHand;
int			meshTextureUnit = 5;

// buttons
bool		annotate = true, stereopsis = true, fixGaze = false;
Toggler		annotateToggler(&annotate, "Annotate", 20, 13, 14);
Toggler		stereopsisToggler(&stereopsis, "Stereopsis", 160, 13, 14);
Toggler		fixGazeToggler(&fixGaze, "Fix Gaze", 320, 13, 14);
Toggler	   *buttons[] = { &annotateToggler, &stereopsisToggler, &fixGazeToggler };
int			nbuttons = sizeof(buttons)/sizeof(Toggler *);

enum		Eye { Left = 0, Right };

vec3 XAxis(mat4 m)  { return vec3(m[0][0], m[1][0], m[2][0]); }
vec3 YAxis(mat4 m)  { return vec3(m[0][1], m[1][1], m[2][1]); }
vec3 ZAxis(mat4 m)  { return vec3(m[0][2], m[1][2], m[2][2]); }
vec3 Origin(mat4 m) { return vec3(m[0][3], m[1][3], m[2][3]); }

vec3 EyeOffset(Eye e) {
	// head faces +Z axis, left face is towards +X axis
	vec3 headX = XAxis(head.toWorld), headZ = ZAxis(head.toWorld);
	return e == Left? vec3(.23f*headX+.55f*headZ) : vec3(-.23f*headX+.55f*headZ);
}

void RenderScene(Camera &camera, bool showHead) {
	GLuint s = UseMeshShader();
	vec3 xlight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(s, "defaultLight", xlight);
	glEnable(GL_DEPTH_TEST);
	bench.Display(camera, meshTextureUnit);
	ground.Display(camera, meshTextureUnit);
	leftHand.Display(camera);
	rightHand.Display(camera);
	if (showHead) head.Display(camera);
}

void RenderEye(Eye e) {
	// compute view matrices for left and right eyes
	vec3 headP = Origin(head.toWorld), offset = EyeOffset(e);
	mat4 eyeView = LookAt(headP+offset, stereopsis? lookAt : lookAt+offset, vec3(0, 1, 0));
	if (e == Left) glClearColor(0, 1, 0, 1);
	if (e == Right) glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, bufW, bufH);
	cameraUser.SetModelview(eyeView);
	RenderScene(cameraUser, false); // don't show head
	if (annotate) {
		// center crosshair
		glDisable(GL_DEPTH_TEST);
		UseDrawShader(ScreenMode());
		Line(vec2(bufW/2-20, bufH/2), vec2(bufW/2+20, bufH/2), 3.7f, vec3(1, 1, 0));
		Line(vec2(bufW/2, bufH/2-20), vec2(bufW/2, bufH/2+20), 3.7f, vec3(1, 1, 0));
	}
	vroom.CopyFramebufferToEyeTexture(fbTextureNames[e], fbTextureUnits[e]);
}

void Display() {
	// clear, enable zbuffer, smooth lines, multi-sample
	glClearColor(.5f, .5f, .5f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_MULTISAMPLE); // *** working for RenderEye?
	glEnable(GL_DEPTH_TEST);
	// enable custom frame buffer, render eye textures, present to HMD
	glBindFramebuffer(GL_FRAMEBUFFER, vroom.framebuffer);
	RenderEye(Left);
	RenderEye(Right);
	if (vroom.hmdPresent) vroom.SubmitOpenGLFrames(fbTextureUnits[1], fbTextureUnits[0]);
//	if (vroom.hmdPresent) vroom.SubmitOpenGLFrames(fbTextureNames[1], fbTextureNames[0]);
	// display eye textures on default frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(.7f, .7f, .7f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(displayProgram);
	for (int k = 0; k < 2; k++) {
		SetUniform(displayProgram, "textureImage", (int) fbTextureUnits[k]);
		glViewport(k*eyeW, winH-eyeH, eyeW, eyeH);
		glDrawArrays(GL_QUADS, 0, 4);
	}
	// display global scene
	glViewport(0, 0, winW, winH-eyeH);
	RenderScene(cameraScene, true);
	// miscellany
	UseDrawShader(cameraScene.fullview);
	glDisable(GL_DEPTH_TEST);
	if (annotate) {
		ArrowV(Origin(head.toWorld), 3*ZAxis(head.toWorld), cameraScene.modelview, cameraScene.persp, vec3(1,0,0), 2, 6);
		// Disk(Origin(head.toWorld)+EyeOffset(Left), 6, vec3(1, 0, 0));
		// Disk(Origin(head.toWorld)+EyeOffset(Right), 6, vec3(1, 0, 0));
		glDisable(GL_DEPTH_TEST);
		Disk(Origin(head.toWorld), 8, vec3(1, 1, 1));
		Disk(Origin(leftHand.toWorld), 8, vec3(1, 1, 1));
		Disk(Origin(rightHand.toWorld), 8, vec3(1, 1, 1));
		Disk(lookAt, 11, vec3(1, 0, 0));
		Disk(light, 11, vec3(1, 1, 0));
	}
	if (picked == &framer) framer.Draw(cameraScene.fullview);
	if ((float) (clock()-mouseEvent)/CLOCKS_PER_SEC < .5f) cameraScene.arcball.Draw();
	UseDrawShader(ScreenMode());
	Quad(vec3(1, 1), vec3(1, 29), vec3(431, 29), vec3(431, 1), true, vec3(.5), .5);
	for (int i = 0; i < nbuttons; i++) buttons[i]->Draw(NULL, 11);
	glFlush();
}

void OrientHead() {
	vec3 x = XAxis(head.toWorld), y = YAxis(head.toWorld), z = ZAxis(head.toWorld);
	float xlen = length(x), ylen = length(y), zlen = length(z);
	vec3 zNew = zlen*normalize(lookAt-Origin(head.toWorld));
	vec3 xNew = xlen*normalize(cross(vec3(0, 1, 0), zNew)); // cross with up (presumed 0,1,0)
	vec3 yNew = ylen*normalize(cross(zNew, xNew));
	head.toWorld[0][0] = xNew.x; head.toWorld[1][0] = xNew.y; head.toWorld[2][0] = xNew.z;
	head.toWorld[0][1] = yNew.x; head.toWorld[1][1] = yNew.y; head.toWorld[2][1] = yNew.z;
	head.toWorld[0][2] = zNew.x; head.toWorld[1][2] = zNew.y; head.toWorld[2][2] = zNew.z;
}

void TestFramerHit(float x, float y, Mesh &m, void *oldPicked) {
	int ix = (int) x, iy = (int) y;
	// test for base hit
	if (!picked && MouseOver(x, y, Origin(m.toWorld), cameraScene.fullview)) {
		picked = &framer;
		framer.Set(&m.toWorld, 100, cameraScene.fullview);
		framer.Down(ix, iy, cameraScene.modelview, cameraScene.persp);
	}
	// test for arcball hit
	if (!picked && oldPicked == &framer && framer.Hit(ix, iy)) {
		picked = &framer;
		framer.Down(ix, iy, cameraScene.modelview, cameraScene.persp);
	}
}

void TestMeshHit(float x, float y, Mesh &m) {
	if (!picked && MouseOver(x, y, Origin(m.toWorld), cameraScene.fullview)) {
		picked = &m;
		mover.Down(&m.toWorld, (int) x, (int) y, cameraScene.modelview, cameraScene.persp);
	}
}

void TestPointHit(float x, float y, vec3 &p) {
	if (!picked && MouseOver(x, y, p, cameraScene.fullview)) {
		picked = &p;
		mover.Down(&p, (int) x, (int) y, cameraScene.modelview, cameraScene.persp);
	}
}

void TestButtonHit(float x, float y) {
	for (int i = 0; i < nbuttons; i++)
		if (buttons[i]->DownHit(x, y)) picked = buttons;
}

void MouseButton(float x, float y, bool left, bool down) {
	if (y > winH-eyeH) return;
	mouseEvent = clock();
	if (left && down) {
		glViewport(0, 0, winW, winH-eyeH); // needed for subsequent calls to ScreenLine, ScreenPoint, etc.
		void *oldPicked = picked;
		picked = NULL;
		TestButtonHit(x, y);
		TestFramerHit(x, y, head, oldPicked);
		TestFramerHit(x, y, leftHand, oldPicked);
		TestFramerHit(x, y, rightHand, oldPicked);
		TestPointHit(x, y, lookAt);
		TestPointHit(x, y, light);
		if (!picked) {
			picked = &cameraScene;
			cameraScene.Down(x, y, Shift());
		}
	}
	if (!down && picked == &cameraScene) cameraScene.Up();
	if (!down && picked == &framer) framer.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (y > winH-eyeH) return;
	mouseEvent = clock();
	if (leftDown) {
		if (picked == &cameraScene)
			cameraScene.Drag(x, y);
		if (picked == &lookAt) {
			mover.Drag((int) x, (int) y, cameraScene.modelview, cameraScene.persp);
			OrientHead();
		}
		if (picked == &light || picked == &leftHand || picked == &rightHand)
			mover.Drag((int) x, (int) y, cameraScene.modelview, cameraScene.persp);
		if (picked == &framer) {
			float f = length(lookAt-Origin(head.toWorld))/length(ZAxis(head.toWorld));
			framer.Drag((int) x, (int) y, cameraScene.modelview, cameraScene.persp);
			if (!fixGaze) lookAt = Origin(head.toWorld)+f*ZAxis(head.toWorld);
			OrientHead();
		}
	}
}

void MouseWheel(float spin) {
	if (picked == &framer) framer.Wheel(spin, false);
	if (picked == &lookAt || picked == &light) {
		mover.Wheel(spin);
		if (picked == &lookAt) OrientHead();
	}
	if (picked == &cameraScene) cameraScene.Wheel(spin, Shift());
}

GLuint MakeTextureDisplayProgram() {
	const char *vertexDisplayShader = R"(
		#version 130
		out vec2 uv;
		void main() {
			vec2 pts[] = vec2[4](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(1,-1));
			vec2 uvs[] = vec2[4](vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1,0));
			gl_Position = vec4(pts[gl_VertexID], 0, 1);
			uv = uvs[gl_VertexID];
		}
	)";
	const char *pixelDisplayShader = R"(
		#version 330
		in vec2 uv;
		out vec4 color;
		uniform sampler2D textureImage;
		void main() { color = texture(textureImage, uv); }
	)";
	return LinkProgramViaCode(&vertexDisplayShader, &pixelDisplayShader);
}

void MakeScene(string objdir, string imgdir) {
	if (!bench.Read(objdir+"Bench.obj", imgdir+"Bench.tga")) printf("can't read bench\n");
	if (!ground.Read(objdir+"Floor.obj", imgdir+"Floor.tga")) printf("can't read floor\n");
	if (!head.Read(objdir+"Head.obj")) printf("can't read head\n");
	if (!leftHand.Read(objdir+"Hand.obj") || !rightHand.Read(objdir+"Hand.obj")) printf("can't read hand\n");
	bench.toWorld = Scale(.5f)*Translate(0, -.2f, .7f);
	ground.toWorld = Scale(2)*Translate(0, -.3f, 0);
	head.toWorld = RotateX(20)*Translate(.5f, .6f, -.85f)*Scale(.17f);
	OrientHead();
	leftHand.toWorld = Translate(.7f, .4f, -.4f)*RotateX(-90)*Scale(-.15f, .15f, .15f);
	rightHand.toWorld = Translate(.4f, .4f, -.4f)*RotateX(-90)*Scale(.15f);
}

void Resize(int width, int height) {
	winW = width;
	winH = height;
	int maxEyeH = (int) (.5*winW/aspectRatio);
	eyeH = height/3 > maxEyeH? maxEyeH : height/3;
	eyeW = (int) (eyeH*aspectRatio);
	cameraScene.Resize(winW, winH-eyeH);
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (!press) return;
	if (key == 'A') annotate = !annotate;
	if (key == 'L') stereopsis = true;
	if (key == 'I') stereopsis = false;
	if (key == 'F') fixGaze = !fixGaze;
}

const char *usage = R"(
  A: annotations on/off
  L: local focus
  I: infinite focus
  F: fix gaze on/off
)";

int main() {
	// initialize VR, app window, OpenGL
	bool initVR = vroom.InitOpenVR();
	const char *title = initVR? "VR-Test" : "VR-Test (NO HEAD MOUNTED DISPLAY)";
	GLFWwindow *w = InitGLFW(100, 50, winW, winH, title);
	// make app display program and VR render targets
	if (!(displayProgram = MakeTextureDisplayProgram())) printf("can't link shader program");
	if (!vroom.InitFrameBuffer(bufW, bufH)) printf("can't make frame buffer");
	glGenTextures(2, fbTextureNames);
	// read meshes, position/orient characters
	MakeScene("C:/Users/longt/Code/Assets/Models/", "C:/Users/longt/Code/Assets/Images/");
	// callbacks, usage
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf("Usage: %s", usage);
	hmdActive = vroom.StartTransformHMD(&head.toWorld);
	printf("hmd active = %s\n", hmdActive ? "true" : "false");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		if (hmdActive) vroom.UpdateTransformHMD();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// finish
	vr::VR_Shutdown();
	glfwDestroyWindow(w);
	glfwTerminate();
}
