// VR-Demo.cpp: app and HMD display with HMD tracking
// TODO
// 1) eye separation (check ipd, may be way out of scale)
// 2) hand tracking: location/orientation/button
// 6) if HMD connected, app right eye black


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
int			hmdW = 1024, hmdH = 768;
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
vec3		wht(1, 1, 1), red(1, 0, 0), grn(0, 1, 0), blu(0, 0, 1), yel(1, 1, 0), mag(1, 0, 1);

// cameras
Quaternion	initialSceneOrientation(-.17f, .42f, .09f, .88f);
Camera		cameraScene(0, 0, winW, winH-appEyeH, initialSceneOrientation, vec3(0, 0, -5));
Camera		cameraUser(0, 0, hmdW, hmdH);

// lighting
vec3		light(-.2f, .4f, .3f);

// interaction
Framer		framer;								// move/orient head or hands
Mover		mover;								// move lookAt or light
void	   *picked = NULL, *prevPicked = NULL;	// either: buttons, &cameraScene, &lookAt, &light, &leftHand, &rightHand, &head
time_t		mouseEvent = clock();

// directories
string		objDir("C:/Users/longt/Code/Assets/Models/");
string		imgDir("C:/Users/longt/Code/Assets/Images/");

// meshes
Mesh		bench, ground, head, leftHand, rightHand, button;
int			meshTextureUnit = 5;

// buttons
Toggler		annotate("Annotate", true, 20, 13, 14);
Toggler		stereopsis("Stereopsis", false, 140, 13, 14);
Toggler		fixGaze("Fix Gaze", false, 280, 13, 14);
Toggler		hmdTrack("HMD Track", false, 400, 13, 14);
Toggler	   *buttons[] = { &annotate, &stereopsis, &fixGaze }; // , &hmdTrack };
int			nbuttons = sizeof(buttons)/sizeof(Toggler *);

// gameplay
bool		targeted = false;
vec3		target;
vector<vec3> hits;

// Interaction

bool MoverPicked(void *p = picked) { return p == &lookAt || p == &light; }
bool FramerPicked(void *p = picked) { return p == &head || p == &leftHand || p == &rightHand; }

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
	return Origin(m)-.017f*XAxis(m)-.14f*YAxis(m)+.08f*ZAxis(m);
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


void GetVrTransforms() {
	if (hmdPresent) {
		mat4 headM, leftHandM, rightHandM;
		vroom.GetTransforms(headM, leftHandM, rightHandM);
		head.toWorld = headM*Scale(.17f, .17f, .17f);
		leftHand.toWorld = leftHandM*Scale(.15f, .15f, .15f);
		rightHand.toWorld = rightHandM*Scale(.15f, .15f, .15f);
//		head.toWorld = Scale(.17f, .17f, .17f)*headM;
//		leftHand.toWorld = Scale(.15f, .15f, .15f)*leftHandM;
//		rightHand.toWorld = Scale(.15f, .15f, .15f)*rightHandM;
		float f = length(lookAt-Origin(head.toWorld))/length(ZAxis(head.toWorld));
		lookAt = Origin(head.toWorld)+f*ZAxis(head.toWorld);
	}
}

vec3 Laser1() { return Origin(rightHand.toWorld); }

vec3 Laser2() {
	vec3 p1 = Laser1();
	return p1+5*normalize((FingerTip(Right)-p1));
}

mat4 FromQuatMove(Quaternion q, vec3 t, float scale) {
	mat4 m = q.GetMatrix();
	m[0][3] = t.x; m[1][3] = t.y; m[2][3] = t.z;
	return m*Scale(scale, scale, scale);
}

bool Intersect(Mesh &m, vec3 p1, vec3 p2, vec3 &intersection) {
	// if p1p2 intersects m, set intersection and return true
	// inverse transform p1, p2 back to native mesh space
	mat4 inv = Invert(m.toWorld);
	vec3 xp1 = Vec3(inv*vec4(p1, 1)), xp2 = Vec3(inv*vec4(p2, 1));
	// calculate alpha in native space
	float alpha;
	bool hit = m.IntersectWithSegment(xp1, xp2, &alpha);
	if (hit)
		intersection = p1+alpha*(p2-p1);
	return hit;
}

// Display

void ShowAxes(Mesh &m, float a = .75f) {
	UseDrawShader();
	ArrowV(vec3(0,0,0), vec3(a,0,0), cameraScene.modelview*m.toWorld, cameraScene.persp, red, 2, 8);
	ArrowV(vec3(0,0,0), vec3(0,a,0), cameraScene.modelview*m.toWorld, cameraScene.persp, grn, 2, 8);
	ArrowV(vec3(0,0,0), vec3(0,0,a), cameraScene.modelview*m.toWorld, cameraScene.persp, blu, 2, 8);
}

void RenderMesh(Mesh &m, Camera &camera, vec3 color, bool vrDisplay = false) {
	GLuint s = UseMeshShader();
	SetUniform(s, "color", color);
	m.Display(camera);
//	if (!vrDisplay)
//		ShowAxes(m);
}

void RenderScene(Camera &camera, bool vrDisplay) {
	glEnable(GL_DEPTH_TEST);
	GLuint s = UseMeshShader();
	vec3 xlight = Vec3(camera.modelview*vec4(light, 1));
	SetUniform(s, "defaultLight", xlight);
	SetUniform(s, "useLight", false);
	button.Display(camera, meshTextureUnit);
	SetUniform(s, "useLight", true);
	bench.Display(camera, meshTextureUnit);
	ground.Display(camera, meshTextureUnit);
	RenderMesh(leftHand, camera, grn, vrDisplay);
	RenderMesh(rightHand, camera, red, vrDisplay);
	if (!vrDisplay) {
		SetUniform(s, "twoSidedShading", true);
		RenderMesh(head, camera, grn, false);
		SetUniform(s, "twoSidedShading", false);
	}
}

void RenderEye(Side e, vec3 backgrnd) {
	// compute view matrices for left and right eyes
	vec3 headP = Origin(head.toWorld), offset = EyeOffset(e);
	mat4 eyeView = stereopsis.on?
		LookAt(headP+offset, lookAt+offset, vec3(0, 1, 0)) :
		LookAt(headP, lookAt, vec3(0, 1, 0));
	// mat4 eyeView = LookAt(headP+offset, stereopsis.on? lookAt : lookAt+offset, vec3(0, 1, 0));
	// *** TODO: this assumes mid-eye is origin, but in head.obj, origin likely
	//           base of head, so appropriate translation should be added here
	glClearColor(backgrnd.x, backgrnd.y, backgrnd.z, 1);
	glViewport(0, 0, hmdW, hmdH);
	cameraUser.SetModelview(eyeView);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderScene(cameraUser, true);
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
	RenderEye(Left, wht);						// white background
	RenderEye(Right, wht);						// red background
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
	RenderScene(cameraScene, false);
	// annotations, arcball, buttons
	UseDrawShader(cameraScene.fullview);
	glDisable(GL_DEPTH_TEST);
	if (annotate.on) {
		ArrowV(Origin(head.toWorld), 3*ZAxis(head.toWorld), cameraScene.modelview, cameraScene.persp, vec3(1,0,0), 2, 6);
		// Disk(Origin(head.toWorld)+EyeOffset(Left), 6, red);
		// Disk(Origin(head.toWorld)+EyeOffset(Right), 6, red);
		glDisable(GL_DEPTH_TEST);
		// test the power of the right forefinger, behold!
		Line(Laser1(), Laser2(), 6, red, .5f);
		if (targeted)
			Disk(target, 16, yel, 1, true);
		for (size_t i = 0; i < hits.size(); i++)
			Disk(hits[i], 10, red);
		// draw head and hand controls
		Disk(Origin(head.toWorld), 8, wht);
		Disk(Origin(leftHand.toWorld), 8, wht);
		Disk(FingerTip(Left), 6, mag);
		Disk(FingerTip(Right), 6, mag);
		Disk(Origin(rightHand.toWorld), 8, wht);
		Disk(lookAt, 11, red);
		Star(light, 9, yel, blu);
	}
	else
		Disk(Origin(head.toWorld), 8, wht);
	float dt = (float) (clock()-mouseEvent)/CLOCKS_PER_SEC;
	if (FramerPicked() && dt < 1)
		framer.Draw(cameraScene.fullview);
	if (picked == &cameraScene)
		cameraScene.arcball.Draw();
	UseDrawShader(ScreenMode());
	Quad(vec3(1, 1), vec3(1, 29), vec3(520, 29), vec3(520, 1), true, vec3(.5), .5);
	for (int i = 0; i < nbuttons; i++)
		buttons[i]->Draw(NULL, 11);
	glFlush();
}

// Mouse Callbacks

void TestFramerBaseHit(int x, int y, Mesh &m) {
	if (!picked && MouseOver(x, y, Origin(m.toWorld), cameraScene.fullview)) {
		picked = &m;
		framer.Set(&m.toWorld, 100, cameraScene.fullview);
		framer.Down(x, y, cameraScene.modelview, cameraScene.persp);
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
		if (buttons[i]->DownHit(x, y)) {
			picked = buttons;
			// if (buttons[i] == &hmdTrack && hmdTrack.on && !vroom.StartTransformHMD(&head.toWorld)) {
			//	  hmdTrack.Set(false);
			// 	  printf("can't start HMD\n");
			// }
		}
}

void MouseButton(float x, float y, bool left, bool down) {
	if (y < winH-appEyeH) {
		mouseEvent = clock();
		prevPicked = picked;
		if (left && down) {
			int ix = (int) x, iy = (int) y;
			glViewport(0, 0, winW, winH-appEyeH); // needed by ScreenLine, ScreenPoint
			picked = NULL;
			// test for framer base pick
			TestFramerBaseHit(ix, iy, head);
			TestFramerBaseHit(ix, iy, leftHand);
			TestFramerBaseHit(ix, iy, rightHand);
			// test for framer arcball hit
			if (FramerPicked(prevPicked) && framer.Hit(ix, iy)) {
				framer.Down(ix, iy, cameraScene.modelview, cameraScene.persp);
				picked = prevPicked;
			}
			// test for button pick, lookAt or light pick
			TestButtonHit(x, y);
			TestPointHit(x, y, lookAt);
			TestPointHit(x, y, light);
			if (!picked) {
				picked = &cameraScene;
				cameraScene.Down(x, y, Shift());
			}
		}
		if (!down) {
			if (FramerPicked())
				framer.Up();
			if (picked == &cameraScene) {
				cameraScene.Up();
				picked = NULL;
			}
		}
	}
}
void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (y < winH-appEyeH) {
		mouseEvent = clock();
		if (leftDown) {
			if (picked == &cameraScene)
				cameraScene.Drag(x, y);
			if (MoverPicked(picked))
				mover.Drag((int) x, (int) y, cameraScene.modelview, cameraScene.persp);
			if (FramerPicked(picked))
				framer.Drag((int) x, (int) y, cameraScene.modelview, cameraScene.persp);
			if (picked == &lookAt)
				OrientHead();
			if (picked == &head) {
				if (fixGaze.on)
					OrientHead();
				else {
					float f = length(lookAt-Origin(head.toWorld))/length(ZAxis(head.toWorld));
					lookAt = Origin(head.toWorld)+f*ZAxis(head.toWorld);
				}
			}
			if (picked == &rightHand)
				targeted = Intersect(button, Laser1(), Laser2(), target);
		}
	}
}

void MouseWheel(float spin) {
	if (FramerPicked())
		framer.Wheel(spin, Shift());
	if (MoverPicked()) {
		mover.Wheel(spin);
		if (picked == &lookAt)
			OrientHead();
	}
	if (picked == &cameraScene || (!picked && prevPicked == &cameraScene))
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

// Scenes

void ReadMesh(Mesh &m, string meshName, mat4 t = mat4(1)) {
	if (!m.Read(objDir+meshName, &t))
		printf("can't read %s\n", meshName.c_str());
}

void ReadMesh(Mesh &m, string meshName, string imageName, mat4 t = mat4(1)) {
	if (!m.Read(objDir+meshName, imgDir+imageName, &t))
		printf("can't read %s or %s\n", meshName.c_str(), imageName.c_str());
}

void MakeScene() {
	// read obj files and set toWorld transforms
	ReadMesh(bench, "Bench.obj", "Bench.tga", Scale(.5f)*Translate(0,-.2f,.7f));
	ReadMesh(ground, "Floor.obj", "Lily.tga", Scale(2)*Translate(0,-.3f,0));
	ReadMesh(head, "Head.obj", RotateX(20)*Translate(.5f, .6f, -.85f)*Scale(.17f, .17f, .17f));
	ReadMesh(leftHand, "HandLeft.obj", Translate(.7f, .4f, -.4f)*RotateX(-45)*Scale(.15f));
	ReadMesh(button, "Square.obj", "Push!.png", Translate(.1f, .2f, -.4f)*RotateY(60)*RotateZ(-90)*Scale(.1f, .25f, 1));
	// read right hand, adjust to point at button, test intersection
	ReadMesh(rightHand, "HandRight.obj", FromQuatMove(Quaternion(-.6f, 0, .6f, .6f), vec3(.4f, .2f, -.4f), .15f));
	targeted = Intersect(button, Laser1(), Laser2(), target);
	// if no vr running, orient head towards look-at
	OrientHead();
}

// Application

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == ' ' && targeted)
		hits.push_back(target);
}

void Resize(int width, int height) {
	winW = width;
	winH = height;
	int maxappEyeH = (int) (.5*winW/hmdAspectRatio);
	appEyeH = height/3 > maxappEyeH? maxappEyeH : height/3;
	appEyeW = (int) (appEyeH*hmdAspectRatio);
	cameraScene.Resize(winW, winH-appEyeH);
}

const char *usage = R"(
	<space bar>: fire!
)";

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
		// read meshes, position/orient characters
		MakeScene();
		// callbacks
		RegisterMouseMove(MouseMove);
		RegisterMouseButton(MouseButton);
		RegisterMouseWheel(MouseWheel);
		RegisterResize(Resize);
		RegisterKeyboard(Keyboard);
		printf("Usage: %s", usage);
		glfwSwapInterval(1);
		while (!glfwWindowShouldClose(w)) {
			GetVrTransforms();
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
