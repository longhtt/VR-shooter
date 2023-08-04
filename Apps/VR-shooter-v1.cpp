// VR-Demo.cpp: app and HMD display with HMD tracking
// see HelloOpenVR_GLFW.cpp

// TODO
// 1) eye separation (check ipd), see also 5)
// 2) hand tracking: location/orientation/button
// 3) resolve version 1/2
// 4) rename 'hmd' --> 'ivr'
// 5) display of eyes (green dots) wrong


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

// VR access
VROOM		vroom;

// headset display
enum		Side { Left = 0, Right };
int			hmdW = 1024, hmdH = 768;			// Vive Cosmos, per eye: 1440 wide, 1700 high
float		hmdAspectRatio = (float) hmdW/hmdH;
GLuint		fbTextureUnits[] = { 2, 3 };		// left and right eye texture memory
GLuint		fbTextureNames[] = { 0, 0 };		// left and right eye texture name
vec3		lookAt(-.35f, -.15f, .55f);
bool		hmdPresent = false;

// app display
GLuint		hmdToAppProgram = 0;
int			appEyeH = 330, appEyeW = (int) (appEyeH*hmdAspectRatio);
int			winW = 2*appEyeW, winH = 3*appEyeH;

// colors
vec3		wht(1, 1, 1), red(1, 0, 0), grn(0, 1, 0), blu(0, 0, 1), yel(1, 1, 0);

// cameras
Quaternion	initialSceneOrientation(-.17f, .42f, .09f, .88f);
Camera		cameraScene(0, 0, winW, winH-appEyeH, initialSceneOrientation, vec3(0, 0, -5));
Camera		cameraUser(0, 0, hmdW, hmdH);

// lighting
vec3		light(-.2f, .4f, .3f);

// interaction
Framer		framer;											// move/orient head or hands
Mover		mover;											// move lookAt or light
void	   *picked = NULL;
time_t		mouseEvent = clock();

// meshes
Mesh		bench, ground, head, leftHand, rightHand, bill2, bill3, pistol;
int			meshTextureUnit = 5;

// buttons
Toggler		annotate("Annotate", true, 20, 13, 14);
Toggler		stereopsis("Stereopsis", false, 140, 13, 14);	// *** remove this option?
Toggler		fixGaze("Fix Gaze", false, 280, 13, 14);
Toggler		hmdTrack("HMD Track", false, 400, 13, 14);
Toggler	   *buttons[] = { &annotate, &stereopsis, &fixGaze }; // , &hmdTrack };
int			nbuttons = sizeof(buttons)/sizeof(Toggler *);

// Geometry

vec3 XAxis(mat4 m)  { return vec3(m[0][0], m[1][0], m[2][0]); }
vec3 YAxis(mat4 m)  { return vec3(m[0][1], m[1][1], m[2][1]); }
vec3 ZAxis(mat4 m)  { return vec3(m[0][2], m[1][2], m[2][2]); }
vec3 Origin(mat4 m) { return vec3(m[0][3], m[1][3], m[2][3]); }

vec3 EyeOffset(Side e) {
	float pd = .05f;
	// head faces +Z axis, left face is towards +X axis
	vec3 headX = normalize(XAxis(head.toWorld));
	vec3 headZ = normalize(ZAxis(head.toWorld));
	return e == Left? vec3(-pd*headX) : vec3(pd*headX);
//	return e == Left? vec3(.23f*headX+.55f*headZ) : vec3(-.23f*headX+.55f*headZ);
}

vec3 FingerTip(Side s) {
	mat4 m = s == Left? leftHand.toWorld : rightHand.toWorld;
	return Origin(m)-.15f*XAxis(m)-.67f*YAxis(m)+.6f*ZAxis(m);
}

void OrientHead() {
	if (hmdPresent)
		return;
	// adjust head to look at lookAt
	vec3 x = XAxis(head.toWorld), y = YAxis(head.toWorld), z = ZAxis(head.toWorld);
	float xlen = length(x), ylen = length(y), zlen = length(z);
	vec3 zNew = zlen*normalize(lookAt-Origin(head.toWorld));
	vec3 xNew = xlen*normalize(cross(vec3(0, 1, 0), zNew));	// up presumed (0,1,0)
	vec3 yNew = ylen*normalize(cross(zNew, xNew));
	head.toWorld[0][0] = xNew.x; head.toWorld[1][0] = xNew.y; head.toWorld[2][0] = xNew.z;
	head.toWorld[0][1] = yNew.x; head.toWorld[1][1] = yNew.y; head.toWorld[2][1] = yNew.z;
	head.toWorld[0][2] = zNew.x; head.toWorld[1][2] = zNew.y; head.toWorld[2][2] = zNew.z;
}

// Display

void RenderScaled(Mesh &m, float s, Camera &camera) {
	mat4 save = m.toWorld;
	m.toWorld = m.toWorld*Scale(s);
	m.Display(camera);
	m.toWorld = save;
}

void RenderScene(Camera &camera, bool showHead) {
	GLuint s = UseMeshShader();
	vec3 xlight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(s, "defaultLight", xlight);
	glEnable(GL_DEPTH_TEST);
	SetUniform(s, "useLight", false);
	bench.Display(camera, meshTextureUnit);
	bill2.Display(camera, meshTextureUnit);
	bill3.Display(camera, meshTextureUnit);
	SetUniform(s, "useLight", true);
	pistol.Display(camera, meshTextureUnit);
	ground.Display(camera, meshTextureUnit);
	RenderScaled(leftHand, .15f, camera);
	RenderScaled(rightHand, .15f, camera);
	if (showHead)
		RenderScaled(head, .17f, camera);
//	mat4 headSave = head.toWorld, lHandSave = leftHand.toWorld, rHandSave = rightHand.toWorld;
//	leftHand.toWorld = leftHand.toWorld*Scale(.15f);
//	leftHand.Display(camera);
//	leftHand.toWorld = lHandSave;
//	rightHand.toWorld = rightHand.toWorld*Scale(.15f);
//	rightHand.Display(camera);
//	rightHand.toWorld = rHandSave;
//	if (showHead) {
//		head.toWorld = head.toWorld*Scale(.17f);
		// *** TODO: this presumes mid-eye is origin, but in head.obj, origin likely
		//           base of head, so appropriate translation should be added here
//		head.Display(camera);
//		head.toWorld = headSave;
//	}
}

void RenderEye(Side e, vec3 backgrnd) {
	// compute view matrices for left and right eyes
	vec3 headP = Origin(head.toWorld), offset = EyeOffset(e);
//	mat4 eyeView = LookAt(headP+offset, lookAt+offset, vec3(0, 1, 0));
	mat4 eyeView = LookAt(headP+offset, stereopsis.on? lookAt : lookAt+offset, vec3(0, 1, 0));
	glClearColor(backgrnd.x, backgrnd.y, backgrnd.z, 1);
	glViewport(0, 0, hmdW, hmdH);
	cameraUser.SetModelview(eyeView);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderScene(cameraUser, false);
	if (annotate.on) {
		// center crosshair
		glDisable(GL_DEPTH_TEST);
		UseDrawShader(ScreenMode());
		Line(vec2(hmdW/2-20, hmdH/2), vec2(hmdW/2+20, hmdH/2), 3.7f, yel);
		Line(vec2(hmdW/2, hmdH/2-20), vec2(hmdW/2, hmdH/2+20), 3.7f, yel);
	}
	vroom.CopyFramebufferToEyeTexture(fbTextureNames[e], fbTextureUnits[e]);
}

void Display() {
	// smooth lines, multi-sample
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_MULTISAMPLE);
	// use custom framebuffer to render eye textures, submit to HMD
	glBindFramebuffer(GL_FRAMEBUFFER, vroom.framebuffer);
	RenderEye(Left, wht);			// white background
	RenderEye(Right, wht);			// red background
	if (vroom.HmdPresent())
		vroom.SubmitOpenGLFrames(fbTextureUnits[0], fbTextureUnits[1]);
	// use default framebuffer for app display
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(.7f, .7f, .7f, 1);	// grey background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// display eye textures
	glUseProgram(hmdToAppProgram);
	for (int k = 0; k < 2; k++) {
		SetUniform(hmdToAppProgram, "textureImage", (int) fbTextureUnits[k]);
		glViewport(k*appEyeW, winH-appEyeH, appEyeW, appEyeH);
		glDrawArrays(GL_QUADS, 0, 4);
	}
	// display global scene
	glViewport(0, 0, winW, winH-appEyeH);
	RenderScene(cameraScene, true);
	// annotations, arcball, buttons
	UseDrawShader(cameraScene.fullview);
	glDisable(GL_DEPTH_TEST);
	if (annotate.on) {
		ArrowV(Origin(head.toWorld), 3*ZAxis(head.toWorld), cameraScene.modelview, cameraScene.persp, vec3(1,0,0), 2, 6);
		 Disk(Origin(head.toWorld)+EyeOffset(Left), 6, red);
		 Disk(Origin(head.toWorld)+EyeOffset(Right), 6, red);
		glDisable(GL_DEPTH_TEST);
		Disk(Origin(head.toWorld), 8, wht);
		Disk(Origin(leftHand.toWorld), 8, wht);
		Disk(FingerTip(Left), 6, grn);
		Disk(FingerTip(Right), 6, grn);
		Disk(Origin(rightHand.toWorld), 8, wht);
		Disk(lookAt, 11, red);
		Star(light, 9, yel, blu);
		if (picked == &framer) framer.Draw(cameraScene.fullview);
	}
	if ((float) (clock()-mouseEvent)/CLOCKS_PER_SEC < .5f)
		cameraScene.arcball.Draw();
	UseDrawShader(ScreenMode());
	Quad(vec3(1, 1), vec3(1, 29), vec3(520, 29), vec3(520, 1), true, vec3(.5), .5);
	for (int i = 0; i < nbuttons; i++)
		buttons[i]->Draw(NULL, 11);
	glFlush();
}

// Hit Testing

void TestFramer(float x, float y, Mesh &m, void *oldPicked) {
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

void TestMesh(float x, float y, Mesh &m) {
	if (!picked && MouseOver(x, y, Origin(m.toWorld), cameraScene.fullview)) {
		picked = &m;
		mover.Down(&m.toWorld, (int) x, (int) y, cameraScene.modelview, cameraScene.persp);
	}
}

void TestPoint(float x, float y, vec3 &p) {
	if (!picked && MouseOver(x, y, p, cameraScene.fullview)) {
		picked = &p;
		mover.Down(&p, (int) x, (int) y, cameraScene.modelview, cameraScene.persp);
	}
}

void TestButtons(float x, float y) {
	for (int i = 0; i < nbuttons; i++)
		if (buttons[i]->DownHit(x, y)) {
			picked = buttons;
		//	if (buttons[i] == &hmdTrack && hmdTrack.on && !vroom.StartTransformHMD(&head.toWorld)) {
		//		hmdTrack.Set(false);
		//		printf("can't start HMD\n");
		//	}
		}
}

// Mouse Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	if (y > winH-appEyeH) return;
	mouseEvent = clock();
	if (left && down) {
		glViewport(0, 0, winW, winH-appEyeH); // needed for calls to ScreenLine, ScreenPoint
		void *oldPicked = picked;
		picked = NULL;
		TestButtons(x, y);
		TestFramer(x, y, head, oldPicked);
		TestFramer(x, y, leftHand, oldPicked);
		TestFramer(x, y, rightHand, oldPicked);
		TestPoint(x, y, lookAt);
		TestPoint(x, y, light);
		if (!picked) {
			picked = &cameraScene;
			cameraScene.Down(x, y, Shift());
		}
	}
	if (!down && picked == &cameraScene)
		cameraScene.Up();
	if (!down && picked == &framer)
		framer.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (y > winH-appEyeH)
		return;
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
			if (!fixGaze.on)
				lookAt = Origin(head.toWorld)+f*ZAxis(head.toWorld);
			OrientHead();
		}
	}
}

void MouseWheel(float spin) {
	if (picked == &framer) framer.Wheel(spin, Shift());
	if (picked == &lookAt || picked == &light) {
		mover.Wheel(spin);
		if (picked == &lookAt)
			OrientHead();
	}
	if (picked == &cameraScene)
		cameraScene.Wheel(spin, Shift());
}

// Copy VR Framebuffer to App Display

GLuint MakeTextureDisplayProgram() {
	const char *vertexDisplayShader = R"(
		#version 330
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

// Application

/* void MakeScene() {
	string objDir("C:/Users/longt/Code/Assets/Models/"), imgDir("C:/Users/longt/Code/Assets/Images/");
	if (!bench.Read(objDir+"Bench.obj", imgDir+"Bench.tga")) printf("can't read bench\n");
	if (!ground.Read(objDir+"Floor.obj", imgDir+"Lily.tga")) printf("can't read floor\n");
	if (!head.Read(objDir+"Head.obj")) printf("can't read head\n");
	if (!leftHand.Read(objDir+"Hand.obj") || !rightHand.Read(objDir+"Hand.obj")) printf("can't read hand\n");
	ground.toWorld = RotateX(90);
	bench.toWorld = Scale(.5f)*Translate(0, -.2f, .7f);
	ground.toWorld = Scale(2)*Translate(0, -.3f, 0);
	leftHand.toWorld = Translate(.7f, .4f, -.4f)*RotateX(-90)*Scale(-1, 1, 1);
	rightHand.toWorld = Translate(.4f, .4f, -.4f)*RotateX(-90); // *Scale(.15f);
	head.toWorld = RotateX(20)*Translate(.5f, .6f, -.85f); // *Scale(.17f);
	OrientHead();
}*/

void MakeScene() {
	string objDir("C:/Users/longt/Code/Assets/Models/"), imgDir("C:/Users/longt/Code/Assets/Images/");
	if (!bench.Read(objDir + "Screen1_Test.obj", imgDir + "Test_Start_S1.jpg")) printf("can't read bench\n");
	if (!bill2.Read(objDir + "Screen1_Test.obj", imgDir + "Test_Start_S1.jpg")) printf("can't read bench\n");
	if (!bill3.Read(objDir + "Screen1_Test.obj", imgDir + "Test_Start_S1.jpg")) printf("can't read bench\n");
	if (!ground.Read(objDir + "Test_ground.obj", imgDir + "marble_floor.jpg")) printf("can't read floor\n");
	if (!head.Read(objDir + "Head.obj")) printf("can't read head\n");
	if (!leftHand.Read(objDir + "Hand.obj") || !rightHand.Read(objDir + "Hand.obj")) printf("can't read hand\n");
	if (!pistol.Read(objDir + "Pistol2.obj", imgDir + "pistol2.png")) printf("cant read pistol\n");
	bench.toWorld = Scale(.5f) * Translate(0, -.2f, .7f) * RotateZ(90) * RotateX(0) * RotateY(90);
	bill2.toWorld = Scale(.5f) * Translate(2.3f, -.2f, .1f) * RotateZ(90) * RotateX(45) * RotateY(90);
	bill3.toWorld = Scale(.5f) * Translate(-2.3f, -.2f, .1f) * RotateZ(90) * RotateX(-45) * RotateY(90);
	ground.toWorld = Scale(2) * Translate(0, -.3f, 0);
	pistol.toWorld = Translate(.55f, .4f, -.2f) * RotateX(-90) * RotateZ(-90) * Scale(.15f);
	leftHand.toWorld = Translate(.7f, .4f, -.4f) * RotateX(-90) * Scale(-1, 1, 1);
	rightHand.toWorld = Translate(.4f, .4f, -.4f) * RotateX(-90); // *Scale(.15f);
	head.toWorld = RotateX(20) * Translate(.5f, .6f, -.85f); // *Scale(.17f);
	OrientHead();
}

void Resize(int width, int height) {
	winW = width;
	winH = height;
	int maxappEyeH = (int) (.5*winW/hmdAspectRatio);
	appEyeH = height/3 > maxappEyeH? maxappEyeH : height/3;
	appEyeW = (int) (appEyeH*hmdAspectRatio);
	cameraScene.Resize(winW, winH-appEyeH);
}

#ifdef VROOM_V1
int version = 1;
#else
int version = 2;
#endif

int main() {
	try {
		// initialize VR, app window, OpenGL
		bool runtime = vroom.InitOpenVR();
			// this should list all connected devices, which can be one of:
			// Invalid, HMD, Controller, GenericTracker, TrackingReference, DisplayRedirect
		if (!runtime)
			printf("no VR runtime\n");
		hmdPresent = runtime && vroom.HmdPresent();
		const char *title = hmdPresent? "VR-Test" : "VR-Test (NO HEAD MOUNTED DISPLAY)";
		GLFWwindow *w = InitGLFW(100, 50, winW, winH, title);
		// make app eye display program
		if (!(hmdToAppProgram = MakeTextureDisplayProgram()))
			printf("can't link shader program");
		// make VR render targets
		if (hmdPresent) {
			hmdW = vroom.RecommendedWidth();
			hmdH = vroom.RecommendedWidth();
		}
		if (!vroom.InitFrameBuffer(hmdW, hmdH))
			printf("can't make frame buffer");
		glGenTextures(2, fbTextureNames);
		if (hmdPresent)
			printf("headset present\n");
#ifdef VROOM_V1
		// initialize HMD orientation
		if (hmdPresent) {
			printf("headset present\n");
			if (!vroom.StartTransformHMD(&head.toWorld))
				printf("can't start HMD transform\n");
		}
		// else {
		//	printf("can't start HMD\n");
		//	hmdTrack.Set(false);
		// }
#endif
		// read meshes, position/orient characters
		MakeScene();
		// callbacks
		RegisterMouseMove(MouseMove);
		RegisterMouseButton(MouseButton);
		RegisterMouseWheel(MouseWheel);
		RegisterResize(Resize);
		printf("version %i\n", version);
		// event loop
		while (!glfwWindowShouldClose(w)) {
		//	if (hmdTrack.on && !fixGaze.on) {
			if (hmdPresent) {
				float f = length(lookAt-Origin(head.toWorld))/length(ZAxis(head.toWorld));
#ifdef VROOM_V1
				vroom.UpdateTransformHMD();
#else
				vroom.GetTransforms(head.toWorld, leftHand.toWorld, rightHand.toWorld);
#endif
				lookAt = Origin(head.toWorld)+f*ZAxis(head.toWorld);
			}
			Display();
			glfwSwapBuffers(w);
			glfwPollEvents();
		}
		// finish
		vr::VR_Shutdown();
		glfwDestroyWindow(w);
		glfwTerminate();
	}
	catch (const std::runtime_error &err) {
		printf("error: %s\n", err.what());
	}
	return 0;
}
