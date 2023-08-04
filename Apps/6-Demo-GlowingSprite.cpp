// GlowingSprite.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "Sprite.h"

GLuint glowSpriteShader = 0;

namespace Glow {
	void BuildShader() {
		const char *vShader = R"(
			#version 330
			uniform mat4 view;
			uniform float z = 0;
			out vec2 uv;
			void main() {
				vec2 pts[] = vec2[4](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(1,-1));
				uv = (vec2(1,1)+pts[gl_VertexID])/2;
				gl_Position = view*vec4(pts[gl_VertexID], z, 1);
			}
		)";
		const char *pShader = R"(
			#version 330
			in vec2 uv;
			out vec4 pColor;
			uniform float glow = 0;
			uniform mat4 uvTransform;
			uniform sampler2D textureImage;
			uniform sampler2D textureMat;
			uniform bool useMat;
			uniform int nTexChannels = 3;
			void main() {
				vec2 st = (uvTransform*vec4(uv, 0, 1)).xy;
				if (nTexChannels == 4)
					pColor = texture(textureImage, st);
				else {
					pColor.rgb = texture(textureImage, st).rgb;
					pColor.a = useMat? texture(textureMat, st).r : 1;
				}
				if (pColor.a < .02) // if nearly full matte,
					discard;		// don't tag z-buffer
				pColor.rgb *= (1+glow);
			}
		)";
		glowSpriteShader = LinkProgramViaCode(&vShader, &pShader);
	}
} // end namespace

class GlowSprite: public Sprite {
public:
	float glow = 0;
	void Display(mat4 *view = NULL, int textureUnit = 0) {
		if (!glowSpriteShader)
			Glow::BuildShader();
		glUseProgram(glowSpriteShader);
		glActiveTexture(GL_TEXTURE0+textureUnit);
		if (nFrames) { // animation
			time_t now = clock();
			if (now > change) {
				frame = (frame+1)%nFrames;
				change = now+(time_t)(frameDuration*CLOCKS_PER_SEC);
			}
			glBindTexture(GL_TEXTURE_2D, textureNames[frame]);
		}
		else glBindTexture(GL_TEXTURE_2D, textureName);
		SetUniform(glowSpriteShader, "textureImage", (int) textureUnit);
		SetUniform(glowSpriteShader, "useMat", matName > 0);
		SetUniform(glowSpriteShader, "nTexChannels", nTexChannels);
		SetUniform(glowSpriteShader, "z", z);
		SetUniform(glowSpriteShader, "glow", glow);
		if (matName > 0) {
			glActiveTexture(GL_TEXTURE0+textureUnit+1);
			glBindTexture(GL_TEXTURE_2D, matName);
			SetUniform(glowSpriteShader, "textureMat", (int) textureUnit+1);
		}
		SetUniform(glowSpriteShader, "view", view? *view*ptTransform : ptTransform);
		SetUniform(glowSpriteShader, "uvTransform", uvTransform);
		glDrawArrays(GL_QUADS, 0, 4);
	}
};

Sprite background;
GlowSprite actor;
time_t start = clock();

void SetGlow() {
	float glowDuration = 4, pause = 2; // in seconds
	float amplitude = 2;
	float dt = (float)(clock()-start)/CLOCKS_PER_SEC;
	if (dt > pause) {
		float t = .75f+(dt-pause)/glowDuration, angle = 2*3.141592f*t;
		actor.glow = amplitude*(1+sin(angle))/2;
	}
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
	GLFWwindow *w = InitGLFW(100, 100, 600, 600, "Glowing Sprite");
	RegisterResize(Resize);
	// read background, actor sprites
	background.Initialize("C:/Users/Jules/Code/Assets/Images/Earth.tga");
	actor.Initialize("C:/Users/Jules/Code/Assets/Images/MattedNumber1.png");
	// event loop
	while (!glfwWindowShouldClose(w)) {
		SetGlow();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
}
