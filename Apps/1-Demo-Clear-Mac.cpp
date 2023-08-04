// 1-Demo-Clear-Mac.cpp - OpenGL test program for Apple Silicon architecture

#include <glad.h>														// OpenGL routines
#include <glfw3.h>														// OS negotiation 
#include "GLXtras.h"													// convenience routines

GLuint	VBO = 0, VAO = 0;												// vertex array ID, GPU buffer ID
GLuint	program = 0;													// shader program ID, valid if > 0
int4	vp(150, 50, 400, 400);											// app window viewport (x, y, width, height)

const char *vertexShader = R"(											// operations before the rasterizer
	#version 330														// minimum version
	in vec2 point;														// 2D input from GPU memory expected 
	void main() {
		gl_Position = vec4(point, 0, 1);								// promote 2D point to 4D, set built-in output
	}
)";

const char *pixelShader = R"(											// operations after the rasterizer
	#version 330														// minimum version
	out vec4 pColor;													// 4D color output expected
	void main() {
		pColor = vec4(0, 1, 0, 1);										// set (r,g,b,a) pixel to opaque green
	}
)";

void BufferVertices() {
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {-1,-1}, {1,1}, {1,-1} };	// 2 triangles (6 points) define app window
	glGenVertexArrays(1, &VAO);											// create vertex array
	glGenBuffers(1, &VBO);												// create vertex buffer on GPU
	glBindVertexArray(VAO);												// bind (make active) the vertex array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);									// then bind vertex buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);	// copy six vertices to GPU buffer
}

void Display() {
	glUseProgram(program);												// ensure correct program
	glBindBuffer(GL_ARRAY_BUFFER, VBO);									// ensure correct buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);			// establish GPU memory transfer to vertex shader
	glDrawArrays(GL_TRIANGLES, 0, 6);									// send 6 vertices from GPU to vertex shader
	glFlush();															// complete graphics operations 
}

int main() {															// application entry
	GLFWwindow *w = InitGLFW(vp[0], vp[1], vp[2], vp[3], "Clear");		// create titled window
	program = LinkProgramViaCode(&vertexShader, &pixelShader);			// build shader program
	BufferVertices();													// store the corner points in GPU memory
	while (!glfwWindowShouldClose(w)) {									// event loop, check for user kill window
		Display();                                     
		glfwSwapBuffers(w);												// exchange render buffer with display buffer
		glfwPollEvents();												// check for user input
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
