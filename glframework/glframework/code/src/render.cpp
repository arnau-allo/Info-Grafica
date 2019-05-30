#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <time.h>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
	void setupAxis();
	void cleanupAxis();
	void drawAxis();
}
////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 50.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char *buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

////////////////////////////////////////////////// BOX
namespace Box {
	GLuint cubeVao;
	GLuint cubeVbo[2];
	GLuint cubeShaders[2];
	GLuint cubeProgram;

	float cubeVerts[] = {
		// -5,0,-5 -- 5, 10, 5
		-5.f,  0.f, -5.f,
		5.f,  0.f, -5.f,
		5.f,  0.f,  5.f,
		-5.f,  0.f,  5.f,
		-5.f, 10.f, -5.f,
		5.f, 10.f, -5.f,
		5.f, 10.f,  5.f,
		-5.f, 10.f,  5.f,
	};
	GLubyte cubeIdx[] = {
		1, 0, 2, 3, // Floor - TriangleStrip
		0, 1, 5, 4, // Wall - Lines
		1, 2, 6, 5, // Wall - Lines
		2, 3, 7, 6, // Wall - Lines
		3, 0, 4, 7  // Wall - Lines
	};

	const char* vertShader_xform =
		"#version 330\n\
in vec3 in_Position;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
	const char* fragShader_flatColor =
		"#version 330\n\
out vec4 out_Color;\n\
uniform vec4 color;\n\
void main() {\n\
	out_Color = color;\n\
}";

	void setupCube() {
		glGenVertexArrays(1, &cubeVao);
		glBindVertexArray(cubeVao);
		glGenBuffers(2, cubeVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, cubeVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 20, cubeIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cubeShaders[0] = compileShader(vertShader_xform, GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileShader(fragShader_flatColor, GL_FRAGMENT_SHADER, "cubeFrag");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		glBindAttribLocation(cubeProgram, 0, "in_Position");
		linkProgram(cubeProgram);
	}
	void cleanupCube() {
		glDeleteBuffers(2, cubeVbo);
		glDeleteVertexArrays(1, &cubeVao);

		glDeleteProgram(cubeProgram);
		glDeleteShader(cubeShaders[0]);
		glDeleteShader(cubeShaders[1]);
	}
	void drawCube() {
		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		// FLOOR
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.6f, 0.6f, 0.6f, 1.f);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);
		// WALLS
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.f, 0.f, 0.f, 1.f);
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, (void*)(sizeof(GLubyte) * 4));
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, (void*)(sizeof(GLubyte) * 8));
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, (void*)(sizeof(GLubyte) * 12));
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, (void*)(sizeof(GLubyte) * 16));

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

////////////////////////////////////////////////// AXIS
namespace Axis {
	GLuint AxisVao;
	GLuint AxisVbo[3];
	GLuint AxisShader[2];
	GLuint AxisProgram;

	float AxisVerts[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0
	};
	float AxisColors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};
	GLubyte AxisIdx[] = {
		0, 1,
		2, 3,
		4, 5
	};
	const char* Axis_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
	const char* Axis_fragShader =
		"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

		AxisProgram = glCreateProgram();
		glAttachShader(AxisProgram, AxisShader[0]);
		glAttachShader(AxisProgram, AxisShader[1]);
		glBindAttribLocation(AxisProgram, 0, "in_Position");
		glBindAttribLocation(AxisProgram, 1, "in_Color");
		linkProgram(AxisProgram);
	}
	void cleanupAxis() {
		glDeleteBuffers(3, AxisVbo);
		glDeleteVertexArrays(1, &AxisVao);

		glDeleteProgram(AxisProgram);
		glDeleteShader(AxisShader[0]);
		glDeleteShader(AxisShader[1]);
	}
	void drawAxis() {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

namespace TOct {
	GLuint tOctVao;
	GLuint tOctVbo[3];
	GLuint tOctShaders[2];
	GLuint tOctProgram;
	GLuint geom_shader;
	glm::mat4 objMat = glm::mat4(1.f);

	extern const float a = 0.1f;
	const float h = 3 * a * sqrt(2) / 2;
	const float c = sqrt(pow(a, 2.0f) / 2);
	int numVerts = 20; // 4 vertex/face * 22 faces + 22 PRIMITIVE RESTART
	float velocity = 0.5f;

	glm::vec3 tOctdirections[20];

	glm::vec3 tOctVerts[20];

	glm::vec3 tOctNorms[] = {
		glm::vec3(0.f, 0.f, 1.f)
	};
	GLubyte tOctIdx[] = {
		0
	};

	//glm::vec3 verts[] = {
	//	glm::vec3(0, 1, 1),
	//	glm::vec3(0, -1, 1),
	//	glm::vec3(1, 0, 1),
	//	glm::vec3(-1, 0, 1),
	//	/*glm::vec3(h, 0, 0),
	//	glm::vec3(-h, 0, 0),
	//	glm::vec3(0, h, 0),
	//	glm::vec3(0, -h, 0),
	//	glm::vec3(0, 0, -h)*/
	//};

	//glm::vec3 norms[] = {
	//	glm::vec3(0, 0, h),
	//	/*glm::vec3(h, h, h),
	//	glm::vec3(h, -h, h),
	//	glm::vec3(-h, h, h),
	//	glm::vec3(-h, -h, h),
	//	glm::vec3(h, h, -h),
	//	glm::vec3(h, -h, -h),
	//	glm::vec3(-h, h, -h),
	//	glm::vec3(-h, -h, -h)*/
	//};

	//glm::vec3 tOctVerts[] = {
	//	verts[0], verts[1], verts[3], verts[4]
	//};

	//glm::vec3 tOctNorms[] = {
	//	norms[0], norms[0], norms[0], norms[0]
	//};

	const char* tOct_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
	const char* tOct_fragShader =
		"#version 330\n\
in vec4 vert_Normal;\n\
in vec4 colorFaces;\n\
uniform mat4 mv_Mat; \n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = colorFaces;\n\
}";

	const char* tOct_geomShader[] =
	{
		"#version 330\n\
layout(points) in;\n\
layout(triangle_strip, max_vertices = 100) out; \n\
uniform mat4 mvpMat;\n\
uniform float c;\n\
uniform float h;\n\
out vec4 colorFaces;\n\
void main() {\n\
	vec4 offset = vec4(c, 0, h-c, 0.0);\n\
	vec4 offset1 = vec4(-c, 0, h-c, 0.0);\n\
	vec4 offset2 = vec4(0, c, h-c, 0.0);\n\
	vec4 offset3 = vec4(0, -c, h-c,0.0);\n\
	vec4 offset4 = vec4(c, 0, -(h-c), 0.0);\n\
	vec4 offset5 =  vec4(-c, 0, -(h-c), 0.0);\n\
	vec4 offset6 = vec4(0, c, -(h-c), 0.0);\n\
	vec4 offset7 = vec4(0, -c, -(h-c),0.0);\n\
	vec4 offset8 = vec4(c, h - c, 0, 0.0);\n\
	vec4 offset9 = vec4(-c, h - c, 0, 0.0);\n\
	vec4 offset10 = vec4(0, h - c, c, 0.0);\n\
	vec4 offset11 = vec4(0, h - c, -c, 0.0);\n\
	vec4 offset12 = vec4(c, -(h-c), 0, 0.0);\n\
	vec4 offset13 = vec4(-c, -(h-c), 0, 0.0);\n\
	vec4 offset14 = vec4(0, -(h-c), c, 0.0);\n\
	vec4 offset15 = vec4(0, -(h-c), -c, 0.0);\n\
	vec4 offset16 = vec4(h -c, c, 0, 0.0);\n\
	vec4 offset17 = vec4(h - c, -c, 0, 0.0);\n\
	vec4 offset18 = vec4(h - c, 0, c, 0.0);\n\
	vec4 offset19 = vec4(h - c, 0, -c, 0.0);\n\
	vec4 offset20 = vec4(-(h-c), c, 0, 0.0);\n\
	vec4 offset21 = vec4(-(h-c), -c, 0, 0.0);\n\
	vec4 offset22 = vec4(-(h-c), 0, c, 0.0);\n\
	vec4 offset23 = vec4(-(h-c), 0, -c, 0.0);\n\
	colorFaces = vec4(0.8f, 0.1f, 0.5f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17); \n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
}"/*"#version 330\n\
  layout(points) in;\n\
  layout(triangle_strip, max_vertices = 100) out;\n\
  uniform mat4 mvpMat; \n\

  }" */ };

	void setupTOct() {
		for (int i = 0; i < 20; i++)
			tOctVerts[i] = glm::vec3((rand() % 100) / 10.f - 5, rand() % 100 / 10.f, (rand() % 100) / 10.f - 5);

		for (int i = 0; i < 20; i++)
			tOctdirections[i] = glm::vec3((rand() % 100) / 10.f - 5, rand() % 100 / 10.f, (rand() % 100) / 10.f - 5);

		glGenVertexArrays(1, &tOctVao);
		glBindVertexArray(tOctVao);
		glGenBuffers(2, tOctVbo);

		glBindBuffer(GL_ARRAY_BUFFER, tOctVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tOctVerts), tOctVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, tOctVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tOctNorms), tOctNorms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geom_shader, 1, tOct_geomShader, NULL);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		tOctShaders[0] = compileShader(tOct_vertShader, GL_VERTEX_SHADER, "cubeVert");
		tOctShaders[1] = compileShader(tOct_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		tOctProgram = glCreateProgram();
		glAttachShader(tOctProgram, tOctShaders[0]);
		glAttachShader(tOctProgram, tOctShaders[1]);
		glAttachShader(tOctProgram, geom_shader);
		glBindAttribLocation(tOctProgram, 0, "in_Position");
		glBindAttribLocation(tOctProgram, 1, "in_Normal");
		linkProgram(tOctProgram);
	}
	void cleanupTOct() {
		glDeleteBuffers(3, tOctVbo);
		glDeleteVertexArrays(1, &tOctVao);

		glDeleteProgram(tOctProgram);
		glDeleteShader(tOctShaders[0]);
		glDeleteShader(tOctShaders[1]);
		glDeleteShader(geom_shader);
	}
	void updateTOct() {
		for (int i = 0; i < 20; i++)
		{
			if (tOctVerts[i].x < -5 || tOctVerts[i].x > 5) {
				tOctdirections[i] = glm::vec3(-tOctdirections[i].x, rand() % 10, (rand() % 10) - 5);
				if (tOctVerts[i].x > 5) tOctVerts[i].x = 5;
				else tOctVerts[i].x = -5;
			}
			else if (tOctVerts[i].y < 0 || tOctVerts[i].y > 10) {
				tOctdirections[i] = glm::vec3((rand() % 10) - 5, -tOctdirections[i].y, (rand() % 10) - 5);
				if (tOctVerts[i].y > 10) tOctVerts[i].y = 10;
				else tOctVerts[i].y = 0;
			}
			else if (tOctVerts[i].z < -5 || tOctVerts[i].z > 5) {
				tOctdirections[i] = glm::vec3((rand() % 10) - 5, rand() % 10, -tOctdirections[i].z);
				if (tOctVerts[i].z > 5) tOctVerts[i].z = 5;
				else tOctVerts[i].z = -5;
			}

			tOctVerts[i] += glm::vec3(tOctdirections[i].x * (velocity / 100.f), tOctdirections[i].y * (velocity / 100.f), tOctdirections[i].z * (velocity / 100.f));
		}
	}
	void drawTOct(float dt) {
		glBindVertexArray(tOctVao);
		glUseProgram(tOctProgram);
		glUniformMatrix4fv(glGetUniformLocation(tOctProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(tOctProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(tOctProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform1f(glGetUniformLocation(tOctProgram, "c"), c);
		glUniform1f(glGetUniformLocation(tOctProgram, "h"), h);
		glDrawArrays(GL_POINTS, 0, numVerts);

		updateTOct();
		glBindBuffer(GL_ARRAY_BUFFER, tOctVbo[0]);
		float *buff = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		int j = 0;
		for (int i = 0; i < 60; i += 3) {
			buff[i] = tOctVerts[j].x;
			buff[i + 1] = tOctVerts[j].y;
			buff[i + 2] = tOctVerts[j].z;
			j++;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

float currentTime = 0;

namespace TOct2 {
	GLuint tOct2Vao;
	GLuint tOct2Vbo[3];
	GLuint tOct2Shaders[2];
	GLuint tOct2Program;
	GLuint geom_shader2;
	glm::mat4 objMat2 = glm::mat4(1.f);

	extern const float a = 0.3f;
	const float h = 3 * a * sqrt(2) / 2;
	float c = sqrt(pow(a, 2.0f) / 2); //a * 1.065f;
	int numVerts = 25; // 4 vertex/face * 22 faces + 22 PRIMITIVE RESTART
	float velocity = 0.5f;
	float aa = 0;

	glm::vec3 tOct2directions[20];

	//glm::vec3 tOct2Verts[20];

	glm::vec3 tOct2Verts[]
	{
		glm::vec3(-(4 * a), 5 + (4 * a), -a),
		glm::vec3(-(2 * a), 5 + (4 * a), a),
		glm::vec3(0 , 5 + (4 * a), -a),
		glm::vec3(2 * a, 5 + (4 * a), a),
		glm::vec3(4 * a, 5 + (4 * a), -a),
		glm::vec3(-(4 * a), 5 + (2 * a), a),
		glm::vec3(-(2 * a), 5 + (2 * a), -a),
		glm::vec3(0, 5 + (2 * a), a),
		glm::vec3(2 * a, 5 + (2 * a), -a),
		glm::vec3(4 * a, 5 + (2 * a), a),
		glm::vec3(-(4 * a), 5, -a),
		glm::vec3(-(2 * a), 5, a),
		glm::vec3(0, 5, -a),
		glm::vec3(2 * a, 5, a),
		glm::vec3(4 * a, 5, -a),
		glm::vec3(-(4 * a), 5 - (2 * a), a),
		glm::vec3(-(2 * a), 5 - (2 * a), -a),
		glm::vec3(0, 5 - (2 * a), a),
		glm::vec3(2 * a, 5 - (2 * a), -a),
		glm::vec3(4 * a, 5 - (2 * a), a),
		glm::vec3(-(4 * a), 5 - (4 * a), -a),
		glm::vec3(-(2 * a), 5 - (4 * a), a),
		glm::vec3(0, 5 - (4 * a), -a),
		glm::vec3(2 * a, 5 - (4 * a), a),
		glm::vec3(4 * a, 5 - (4 * a), -a)
	};

	glm::vec3 tOct2Norms[] = {
		glm::vec3(0.f, 0.f, 1.f)
	};
	GLubyte tOct2Idx[] = {
		0
	};

	//glm::vec3 verts[] = {
	//	glm::vec3(0, 1, 1),
	//	glm::vec3(0, -1, 1),
	//	glm::vec3(1, 0, 1),
	//	glm::vec3(-1, 0, 1),
	//	/*glm::vec3(h, 0, 0),
	//	glm::vec3(-h, 0, 0),
	//	glm::vec3(0, h, 0),
	//	glm::vec3(0, -h, 0),
	//	glm::vec3(0, 0, -h)*/
	//};

	//glm::vec3 norms[] = {
	//	glm::vec3(0, 0, h),
	//	/*glm::vec3(h, h, h),
	//	glm::vec3(h, -h, h),
	//	glm::vec3(-h, h, h),
	//	glm::vec3(-h, -h, h),
	//	glm::vec3(h, h, -h),
	//	glm::vec3(h, -h, -h),
	//	glm::vec3(-h, h, -h),
	//	glm::vec3(-h, -h, -h)*/
	//};

	//glm::vec3 tOctVerts[] = {
	//	verts[0], verts[1], verts[3], verts[4]
	//};

	//glm::vec3 tOctNorms[] = {
	//	norms[0], norms[0], norms[0], norms[0]
	//};

	const char* tOct2_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
	const char* tOct2_fragShader =
		"#version 330\n\
in vec4 vert_Normal;\n\
in vec4 colorFaces;\n\
uniform mat4 mv_Mat; \n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = colorFaces;\n\
}";

	const char* tOct2_geomShader[] =
	{
		"#version 330\n\
layout(points) in;\n\
layout(triangle_strip, max_vertices = 100) out; \n\
uniform mat4 mvpMat;\n\
uniform float c;\n\
uniform float h;\n\
uniform float aa;\n\
out vec4 colorFaces;\n\
void main() {\n\
	vec4 offset = vec4(c, -aa, h-c, 0.0);\n\
	vec4 offset1 = vec4(-c, -aa, h-c, 0.0);\n\
	vec4 offset2 = vec4(aa, c, h-c, 0.0);\n\
	vec4 offset3 = vec4(aa, -c, h-c,0.0);\n\
	vec4 offset4 = vec4(c, -aa, -(h-c), 0.0);\n\
	vec4 offset5 =  vec4(-c, -aa, -(h-c), 0.0);\n\
	vec4 offset6 = vec4(-aa, c, -(h-c), 0.0);\n\
	vec4 offset7 = vec4(-aa, -c, -(h-c),0.0);\n\
	vec4 offset8 = vec4(c, h - c, aa, 0.0);\n\
	vec4 offset9 = vec4(-c, h - c, -aa, 0.0);\n\
	vec4 offset10 = vec4(-aa, h - c, c, 0.0);\n\
	vec4 offset11 = vec4(aa, h - c, -c, 0.0);\n\
	vec4 offset12 = vec4(c, -(h-c), -aa, 0.0);\n\
	vec4 offset13 = vec4(-c, -(h-c), aa, 0.0);\n\
	vec4 offset14 = vec4(-aa, -(h-c), c, 0.0);\n\
	vec4 offset15 = vec4(aa, -(h-c), -c, 0.0);\n\
	vec4 offset16 = vec4(h -c, c, -aa, 0.0);\n\
	vec4 offset17 = vec4(h - c, -c, aa, 0.0);\n\
	vec4 offset18 = vec4(h - c, aa, c, 0.0);\n\
	vec4 offset19 = vec4(h - c, aa, -c, 0.0);\n\
	vec4 offset20 = vec4(-(h-c), c, aa, 0.0);\n\
	vec4 offset21 = vec4(-(h-c), -c, -aa, 0.0);\n\
	vec4 offset22 = vec4(-(h-c), aa, c, 0.0);\n\
	vec4 offset23 = vec4(-(h-c), aa, -c, 0.0);\n\
	if(aa == 0){\n\
			colorFaces = vec4(0.8f, 0.1f, 0.5f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset12); \n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset17); \n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
	EmitVertex(); \n\
	gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
	EmitVertex(); \n\
	EndPrimitive(); \n\
	} \n\
	else{\n\
		colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.2, 0.5, 0.2, 1.0);\n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.5f, 0.8f, 0.1f, 1.0f); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		colorFaces = vec4(0.8f, 0.1f, 0.5f, 1.0f);\n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset1);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset22);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset2);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset10);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset20);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset9);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset18);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset8);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset16);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset3);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset14);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset17);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset13);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset12);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset21);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset19);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset4);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset11);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset15);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset6);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset7);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset23);\n\
		EmitVertex(); \n\
		gl_Position = mvpMat * (gl_in[0].gl_Position + offset5);\n\
		EmitVertex(); \n\
		EndPrimitive(); \n\
	} \n\
}"
};

	void setupTOct2() {

		glGenVertexArrays(1, &tOct2Vao);
		glBindVertexArray(tOct2Vao);
		glGenBuffers(2, tOct2Vbo);

		glBindBuffer(GL_ARRAY_BUFFER, tOct2Vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tOct2Verts), tOct2Verts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, tOct2Vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tOct2Norms), tOct2Norms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		geom_shader2 = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geom_shader2, 1, tOct2_geomShader, NULL);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		tOct2Shaders[0] = compileShader(tOct2_vertShader, GL_VERTEX_SHADER, "cubeVert");
		tOct2Shaders[1] = compileShader(tOct2_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		tOct2Program = glCreateProgram();
		glAttachShader(tOct2Program, tOct2Shaders[0]);
		glAttachShader(tOct2Program, tOct2Shaders[1]);
		glAttachShader(tOct2Program, geom_shader2);
		glBindAttribLocation(tOct2Program, 0, "in_Position");
		glBindAttribLocation(tOct2Program, 1, "in_Normal");
		linkProgram(tOct2Program);
	}
	void cleanupTOct2() {
		glDeleteBuffers(3, tOct2Vbo);
		glDeleteVertexArrays(1, &tOct2Vao);

		glDeleteProgram(tOct2Program);
		glDeleteShader(tOct2Shaders[0]);
		glDeleteShader(tOct2Shaders[1]);
		glDeleteShader(geom_shader2);
	}
	void updateTOct2() {

		if (sin(currentTime) > 0)
		{
			c = (1.065f * a) - (((1.065f * a) - (sqrt(pow(a, 2.0f) / 2))) * sin(currentTime));
			aa = 0;
		}
		else
		{
			c = 1.065f * a;
			aa = -sin(currentTime) * 0.33f;
		}
	}
	void drawTOct2(float dt) {
		glBindVertexArray(tOct2Vao);
		glUseProgram(tOct2Program);
		glUniformMatrix4fv(glGetUniformLocation(tOct2Program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat2));
		glUniformMatrix4fv(glGetUniformLocation(tOct2Program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(tOct2Program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform1f(glGetUniformLocation(tOct2Program, "c"), c);
		glUniform1f(glGetUniformLocation(tOct2Program, "h"), h);
		glUniform1f(glGetUniformLocation(tOct2Program, "aa"), aa);
		glDrawArrays(GL_POINTS, 0, numVerts);

		updateTOct2();
		glBindBuffer(GL_ARRAY_BUFFER, tOct2Vbo[0]);
		float *buff2 = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		int j = 0;
		for (int i = 0; i < 3; i += 3) {
			buff2[i] = tOct2Verts[j].x;
			buff2[i + 1] = tOct2Verts[j].y;
			buff2[i + 2] = tOct2Verts[j].z;
			j++;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
	GLuint cubeVao;
	GLuint cubeVbo[3];
	GLuint cubeShaders[2];
	GLuint cubeProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	extern const float halfW = 0.5f;
	int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

						   //   4---------7
						   //  /|        /|
						   // / |       / |
						   //5---------6  |
						   //|  0------|--3
						   //| /       | /
						   //|/        |/
						   //1---------2
	glm::vec3 verts[] = {
		glm::vec3(-halfW, -halfW, -halfW),
		glm::vec3(-halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW, -halfW),
		glm::vec3(-halfW,  halfW, -halfW),
		glm::vec3(-halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW, -halfW)
	};
	glm::vec3 norms[] = {
		glm::vec3(0.f, -1.f,  0.f),
		glm::vec3(0.f,  1.f,  0.f),
		glm::vec3(-1.f,  0.f,  0.f),
		glm::vec3(1.f,  0.f,  0.f),
		glm::vec3(0.f,  0.f, -1.f),
		glm::vec3(0.f,  0.f,  1.f)
	};

	glm::vec3 cubeVerts[] = {
		verts[1], verts[0], verts[2], verts[3],
		verts[5], verts[6], verts[4], verts[7],
		verts[1], verts[5], verts[0], verts[4],
		verts[2], verts[3], verts[6], verts[7],
		verts[0], verts[4], verts[3], verts[7],
		verts[1], verts[2], verts[5], verts[6]
	};
	glm::vec3 cubeNorms[] = {
		norms[0], norms[0], norms[0], norms[0],
		norms[1], norms[1], norms[1], norms[1],
		norms[2], norms[2], norms[2], norms[2],
		norms[3], norms[3], norms[3], norms[3],
		norms[4], norms[4], norms[4], norms[4],
		norms[5], norms[5], norms[5], norms[5]
	};
	GLubyte cubeIdx[] = {
		0, 1, 2, 3, UCHAR_MAX,
		4, 5, 6, 7, UCHAR_MAX,
		8, 9, 10, 11, UCHAR_MAX,
		12, 13, 14, 15, UCHAR_MAX,
		16, 17, 18, 19, UCHAR_MAX,
		20, 21, 22, 23, UCHAR_MAX
	};

	const char* cube_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec4 vert_Normal;\n\
out vec3 vec_light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position + vec3(0, 5, 0), 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
	vec_light = vec3(mv_Mat * objMat * vec4(10.0f, 10.0f, 10.0f, 0.0));\n\
}";
	const char* cube_fragShader =
		"#version 330\n\
in vec4 vert_Normal;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
in vec3 vec_light;\n\
vec3 diffuse_color;\n\
float cosTheta;\n\
uniform vec4 color;\n\
void main() {\n\
	cosTheta = (clamp(dot(normalize(vert_Normal).xyz, normalize(vec_light)), 0, 1));\n\
	diffuse_color = (color.xyz * cosTheta);\n\
	out_Color = vec4(diffuse_color, 1);\n\
}";

	void setupCube() {
		glGenVertexArrays(1, &cubeVao);
		glBindVertexArray(cubeVao);
		glGenBuffers(3, cubeVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glPrimitiveRestartIndex(UCHAR_MAX);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		glBindAttribLocation(cubeProgram, 0, "in_Position");
		glBindAttribLocation(cubeProgram, 1, "in_Normal");
		linkProgram(cubeProgram);
	}
	void cleanupCube() {
		glDeleteBuffers(3, cubeVbo);
		glDeleteVertexArrays(1, &cubeVao);

		glDeleteProgram(cubeProgram);
		glDeleteShader(cubeShaders[0]);
		glDeleteShader(cubeShaders[1]);
	}
	void updateCube(const glm::mat4& transform) {
		objMat = transform;
	}
	void drawCube() {
		glEnable(GL_PRIMITIVE_RESTART);
		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

int exercise = 1;
int lastExercise = 1;

void GLinit(int width, int height) {
	srand(time(NULL));
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	Box::setupCube();
	Axis::setupAxis();
	if(exercise == 1) TOct::setupTOct();
	else TOct2::setupTOct2();
}

void GLcleanup() {
	Box::cleanupCube();
	Axis::cleanupAxis();
	if (exercise == 1) TOct::cleanupTOct();
	else TOct2::cleanupTOct2();
}



void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	currentTime += dt;

	Box::drawCube();
	Axis::drawAxis();
	if (exercise == 1 && lastExercise == 2) TOct::setupTOct();
	else if (exercise == 2 && lastExercise == 1) TOct2::setupTOct2();
	else if (exercise == 1) TOct::drawTOct(dt);
	else if (exercise == 2) TOct2::drawTOct2(dt);
	
	lastExercise = exercise;

	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		if(exercise == 1)ImGui::DragFloat("Velocity", &TOct::velocity, 0.02f, 0.0f, 5.0f);
		if (ImGui::Button("Switch exercise"))
		{
			if(exercise == 1) exercise = 2;
			else if (exercise == 2) exercise = 1;
		}
	}

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}