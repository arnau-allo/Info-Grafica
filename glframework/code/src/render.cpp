#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

#include "objloader.h"

///////// fw decl
namespace ImGui {
	glm::vec3 lightPosition = { 10.f, 10.0f, 10.0f };
	glm::vec3 lightColor = { 1.f, 1.f, 1.f };
	float Diffuse, Specular, Ambient, LightPower;
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

////////////////////////////////////////////////// CUBE
namespace Cube {
	GLuint cubeVao;
	GLuint cubeVbo[3];
	GLuint cubeShaders[2];
	GLuint cubeProgram;
	glm::mat4 objMat = glm::mat4(1.f);
	glm::vec3 objColor = { 0.1f, 1.f, 1.f};

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
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
}";
	const char* cube_fragShader =
		"#version 330\n\
in vec4 vert_Normal;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 color;\n\
void main() {\n\
	out_Color = vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0)) + color.xyz * 0.3, 1.0 );\n\
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
		objMat = glm::translate(objMat, glm::vec3(0.0f, 5.0f, 0.0f));
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
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

////////////////////////////////////////////////// OBJECT
namespace Object {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint objectVao;
	GLuint objectVbo[3];
	GLuint objectShaders[2];
	GLuint objectProgram;

	glm::mat4 objMat = glm::mat4(1.f);
	glm::vec4 objColor = { 0.1f, 1.f, 1.f, 0.f };

	const char* object_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
out vec3 vec_light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
uniform vec3 light_Position;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
	vec_light = vec3(mv_Mat * objMat * vec4(light_Position, 0.0));\n\
}";

	const char* object_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
in vec3 vec_light;\n\
vec3 e;\n\
vec3 ambient_light;\n\
vec3 diffuse_color;\n\
vec3 specular_light;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 color;\n\
uniform vec3 light_Color;\n\
uniform float kd;\n\
uniform float ka;\n\
uniform float ks;\n\
uniform float light_Power;\n\
float cosTheta;\n\
float cosAlpha;\n\
float distance;\n\
vec3 r;\n\
uniform vec3 camera_Point;\n\
void main() {\n\
	ambient_light = ka * color.xyz;\n\
	cosTheta = (clamp(dot(normalize(vert_Normal), normalize(vec_light)), 0, 1));\n\
	diffuse_color =  kd * (color.xyz * light_Color * light_Power * cosTheta);\n\
	r = reflect(-vec_light, vert_Normal);\n\
	e = normalize(-fragPos);\n\
	cosAlpha = dot(e, r);\n\
	specular_light = ks * color.xyz * light_Color *  pow(max(cosAlpha, 0.0), 0.2f);\n\
	out_Color = vec4(ambient_light + diffuse_color + specular_light, 0);\n\
}";

	void setupObject() {
		glBufferData(GL_ARRAY_BUFFER, Object::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &objectVao);
		glBindVertexArray(objectVao);
		glGenBuffers(2, objectVbo);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		objectShaders[0] = compileShader(object_vertShader, GL_VERTEX_SHADER, "cubeVert");
		objectShaders[1] = compileShader(object_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		objectProgram = glCreateProgram();
		glAttachShader(objectProgram, objectShaders[0]);
		glAttachShader(objectProgram, objectShaders[1]);
		glBindAttribLocation(objectProgram, 0, "in_Position");
		glBindAttribLocation(objectProgram, 1, "in_Normal");
		linkProgram(objectProgram);
	}

	void cleanupObject() {
		glDeleteBuffers(2, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}

	void drawObject(float currentTime) {

		glBindVertexArray(objectVao);
		glUseProgram(objectProgram);

		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform3f(glGetUniformLocation(objectProgram, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(objectProgram, "color"), objColor[0], objColor[1], objColor[2]);
		glUniform3f(glGetUniformLocation(objectProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(objectProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(objectProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(objectProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(objectProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(objectProgram, "light_Power"), ImGui::LightPower);
		glDrawArrays(GL_TRIANGLES, 0, Object::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);

	}

}

////////////////////////////////////////////////// OBJECT1
namespace Object1 {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint object1Vao;
	GLuint object1Vbo[3];
	GLuint object1Shaders[2];
	GLuint object1Program;

	glm::mat4 obj1Mat = glm::mat4(1.f);
	glm::vec4 obj1Color = { 1.0f, 0.1f, 1.f, 0.f };

	const char* object1_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
out vec3 vec_light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
uniform vec3 light_Position;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
	vec_light = vec3(mv_Mat * objMat * vec4(light_Position, 0.0));\n\
}";

	const char* object1_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
in vec3 vec_light;\n\
vec3 e;\n\
vec3 ambient_light;\n\
vec3 diffuse_color;\n\
vec3 specular_light;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 color;\n\
uniform vec3 light_Color;\n\
uniform float kd;\n\
uniform float ka;\n\
uniform float ks;\n\
uniform float light_Power;\n\
float cosTheta;\n\
float cosAlpha;\n\
float distance;\n\
vec3 r;\n\
uniform vec3 camera_Point;\n\
void main() {\n\
	ambient_light = ka * color.xyz;\n\
	cosTheta = (clamp(dot(normalize(vert_Normal), normalize(vec_light)), 0, 1));\n\
	diffuse_color =  kd * (color.xyz * light_Color * light_Power * cosTheta);\n\
	r = reflect(-vec_light, vert_Normal);\n\
	e = normalize(-fragPos);\n\
	cosAlpha = dot(e, r);\n\
	specular_light = ks * color.xyz * light_Color * pow(max(cosAlpha, 0.0), 0.2f);\n\
	out_Color = vec4(ambient_light + diffuse_color + specular_light, 0);\n\
}";

	void setupObject1() {
		glBufferData(GL_ARRAY_BUFFER, Object1::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &object1Vao);
		glBindVertexArray(object1Vao);
		glGenBuffers(2, object1Vbo);

		glBindBuffer(GL_ARRAY_BUFFER, object1Vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, object1Vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		object1Shaders[0] = compileShader(object1_vertShader, GL_VERTEX_SHADER, "cubeVert");
		object1Shaders[1] = compileShader(object1_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		object1Program = glCreateProgram();
		glAttachShader(object1Program, object1Shaders[0]);
		glAttachShader(object1Program, object1Shaders[1]);
		glBindAttribLocation(object1Program, 0, "in_Position");
		glBindAttribLocation(object1Program, 1, "in_Normal");
		linkProgram(object1Program);

		obj1Mat = glm::translate(obj1Mat, glm::vec3(5.0f, -5.0f, -20.0f));
		obj1Mat = glm::scale(obj1Mat, glm::vec3(5.0f, 5.0f, 5.0f));
	}

	void cleanupObject1() {
		glDeleteBuffers(2, object1Vbo);
		glDeleteVertexArrays(1, &object1Vao);

		glDeleteProgram(object1Program);
		glDeleteShader(object1Shaders[0]);
		glDeleteShader(object1Shaders[1]);
	}

	void drawObject1(float currentTime) {

		glBindVertexArray(object1Vao);
		glUseProgram(object1Program);

		glUniformMatrix4fv(glGetUniformLocation(object1Program, "objMat"), 1, GL_FALSE, glm::value_ptr(obj1Mat));
		glUniformMatrix4fv(glGetUniformLocation(object1Program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(object1Program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform3f(glGetUniformLocation(object1Program, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(object1Program, "color"), obj1Color[0], obj1Color[1], obj1Color[2]);
		glUniform3f(glGetUniformLocation(object1Program, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(object1Program, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(object1Program, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(object1Program, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(object1Program, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(object1Program, "light_Power"), ImGui::LightPower);
		glDrawArrays(GL_TRIANGLES, 0, Object1::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);

	}

}

////////////////////////////////////////////////// OBJECT2
namespace Object2 {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint object2Vao;
	GLuint object2Vbo[3];
	GLuint object2Shaders[2];
	GLuint object2Program;

	glm::mat4 obj2Mat = glm::mat4(1.f);
	glm::vec4 obj2Color = { 1.0f, 1.f, 0.1f, 0.f };

	const char* object2_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
out vec3 vec_light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
uniform vec3 light_Position;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
	vec_light = vec3(mv_Mat * objMat * vec4(light_Position, 0.0));\n\
}";

	const char* object2_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
in vec3 vec_light;\n\
vec3 e;\n\
vec3 ambient_light;\n\
vec3 diffuse_color;\n\
vec3 specular_light;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 color;\n\
uniform vec3 light_Color;\n\
uniform float kd;\n\
uniform float ka;\n\
uniform float ks;\n\
uniform float light_Power;\n\
float cosTheta;\n\
float cosAlpha;\n\
float distance;\n\
vec3 r;\n\
uniform vec3 camera_Point;\n\
void main() {\n\
	ambient_light = ka * color.xyz;\n\
	cosTheta = (clamp(dot(normalize(vert_Normal), normalize(vec_light)), 0, 1));\n\
	diffuse_color =  kd * (color.xyz * light_Color * light_Power * cosTheta);\n\
	r = reflect(-vec_light, vert_Normal);\n\
	e = normalize(-fragPos);\n\
	cosAlpha = dot(e, r);\n\
	specular_light = ks * color.xyz * light_Color * pow(max(cosAlpha, 0.0), 0.2f);\n\
	out_Color = vec4(ambient_light + diffuse_color + specular_light, 0);\n\
}";

	void setupObject2() {
		glBufferData(GL_ARRAY_BUFFER, Object2::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &object2Vao);
		glBindVertexArray(object2Vao);
		glGenBuffers(2, object2Vbo);

		glBindBuffer(GL_ARRAY_BUFFER, object2Vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, object2Vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		object2Shaders[0] = compileShader(object2_vertShader, GL_VERTEX_SHADER, "cubeVert");
		object2Shaders[1] = compileShader(object2_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		object2Program = glCreateProgram();
		glAttachShader(object2Program, object2Shaders[0]);
		glAttachShader(object2Program, object2Shaders[1]);
		glBindAttribLocation(object2Program, 0, "in_Position");
		glBindAttribLocation(object2Program, 1, "in_Normal");
		linkProgram(object2Program);

		obj2Mat = glm::translate(obj2Mat, glm::vec3(-25.0f, -5.0f, -20.0f));
		obj2Mat = glm::scale(obj2Mat, glm::vec3(5.0f, 5.0f, 5.0f));
	}

	void cleanupObject2() {
		glDeleteBuffers(2, object2Vbo);
		glDeleteVertexArrays(1, &object2Vao);

		glDeleteProgram(object2Program);
		glDeleteShader(object2Shaders[0]);
		glDeleteShader(object2Shaders[1]);
	}

	void drawObject2(float currentTime) {

		glBindVertexArray(object2Vao);
		glUseProgram(object2Program);

		glUniformMatrix4fv(glGetUniformLocation(object2Program, "objMat"), 1, GL_FALSE, glm::value_ptr(obj2Mat));
		glUniformMatrix4fv(glGetUniformLocation(object2Program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(object2Program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform3f(glGetUniformLocation(object2Program, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(object2Program, "color"), obj2Color[0], obj2Color[1], obj2Color[2]);
		glUniform3f(glGetUniformLocation(object2Program, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(object2Program, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(object2Program, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(object2Program, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(object2Program, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(object2Program, "light_Power"), ImGui::LightPower);
		glDrawArrays(GL_TRIANGLES, 0, Object2::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);

	}

}

////////////////////////////////////////////////// OBJECT3
namespace Object3 {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint object3Vao;
	GLuint object3Vbo[3];
	GLuint object3Shaders[2];
	GLuint object3Program;

	glm::mat4 obj3Mat = glm::mat4(1.f);
	glm::vec4 obj3Color = { 0.5f, 0.5f, 0.5f, 0.f };

	const char* object3_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
out vec3 vec_light;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
uniform vec3 light_Position;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
	vec_light = vec3(mv_Mat * objMat * vec4(light_Position, 0.0));\n\
}";

	const char* object3_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
in vec3 vec_light;\n\
vec3 e;\n\
vec3 ambient_light;\n\
vec3 diffuse_color;\n\
vec3 specular_light;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec3 color;\n\
uniform vec3 light_Color;\n\
uniform float kd;\n\
uniform float ka;\n\
uniform float ks;\n\
uniform float light_Power;\n\
float cosTheta;\n\
float cosAlpha;\n\
float distance;\n\
vec3 r;\n\
uniform vec3 camera_Point;\n\
void main() {\n\
	ambient_light = ka * color.xyz;\n\
	cosTheta = (clamp(dot(normalize(vert_Normal), normalize(vec_light)), 0, 1));\n\
	diffuse_color =  kd * (color.xyz * light_Color * light_Power * cosTheta);\n\
	r = reflect(-vec_light, vert_Normal);\n\
	e = normalize(-fragPos);\n\
	cosAlpha = dot(e, r);\n\
	specular_light = ks * color.xyz * light_Color * pow(max(cosAlpha, 0.0), 0.2f);\n\
	out_Color = vec4(ambient_light + diffuse_color + specular_light, 0);\n\
}";

	void setupObject3() {
		glBufferData(GL_ARRAY_BUFFER, Object3::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &object3Vao);
		glBindVertexArray(object3Vao);
		glGenBuffers(2, object3Vbo);

		glBindBuffer(GL_ARRAY_BUFFER, object3Vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, object3Vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		object3Shaders[0] = compileShader(object3_vertShader, GL_VERTEX_SHADER, "cubeVert");
		object3Shaders[1] = compileShader(object3_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		object3Program = glCreateProgram();
		glAttachShader(object3Program, object3Shaders[0]);
		glAttachShader(object3Program, object3Shaders[1]);
		glBindAttribLocation(object3Program, 0, "in_Position");
		glBindAttribLocation(object3Program, 1, "in_Normal");
		linkProgram(object3Program);

		obj3Mat = glm::translate(obj3Mat, glm::vec3(-10.0f, 5.0f, -30.0f));
		obj3Mat = glm::scale(obj3Mat, glm::vec3(5.0f, 5.0f, 5.0f));
	}

	void cleanupObject3() {
		glDeleteBuffers(2, object3Vbo);
		glDeleteVertexArrays(1, &object3Vao);

		glDeleteProgram(object3Program);
		glDeleteShader(object3Shaders[0]);
		glDeleteShader(object3Shaders[1]);
	}

	void drawObject3(float currentTime) {

		glBindVertexArray(object3Vao);
		glUseProgram(object3Program);

		glUniformMatrix4fv(glGetUniformLocation(object3Program, "objMat"), 1, GL_FALSE, glm::value_ptr(obj3Mat));
		glUniformMatrix4fv(glGetUniformLocation(object3Program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(object3Program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform3f(glGetUniformLocation(object3Program, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(object3Program, "color"), obj3Color[0], obj3Color[1], obj3Color[2]);
		glUniform3f(glGetUniformLocation(object3Program, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(object3Program, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(object3Program, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(object3Program, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(object3Program, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(object3Program, "light_Power"), ImGui::LightPower);
		glDrawArrays(GL_TRIANGLES, 0, Object3::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);

	}

}

//My first point, and my first triangle:

static const GLchar * vertex_shader_source[] =
{
			  "#version 330 \n\
			  void main() \n\
			  { \n\
							const vec4 vertices[3] = vec4[3](\n\
							vec4(0.25, -0.25, 0.5, 1.0),\n\
							vec4(0.25, 0.25, 0.5, 1.0),\n\
							vec4(-0.25, -0.25, 0.5, 1.0)); \n\
							gl_Position = \n\
							vertices[gl_VertexID]; \n\
			  }"

};



static const GLchar * fragment_shader_source[] =

{

			  "#version 330\n\
			  \n\
			  out vec4 color; \n\
			  \n\
			  void main() {\n\
 color = vec4(0.0,0.8,1.0,1.0); \n\
}"
};



GLuint compile_shaders(void)

{
	GLuint vertex_shader; //direccion de memoria del vertex shader
	GLuint fragment_shader; //direccion de memoria del fragment shader
	GLuint program;



	vertex_shader = glCreateShader(GL_VERTEX_SHADER); //crea shader tipo vertex
	glShaderSource(vertex_shader, 1, vertex_shader_source, NULL); //Tiene este codigo source
	glCompileShader(vertex_shader); //compila



	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER); //mismo porceso que el vertex pero tipo fragment
	glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);

	glCompileShader(fragment_shader);



	program = glCreateProgram();



	glAttachShader(program, vertex_shader);

	glAttachShader(program, fragment_shader);



	glLinkProgram(program);



	glDeleteShader(vertex_shader);

	glDeleteShader(fragment_shader);



	return program;

}





GLuint myRenderProgram;

GLuint myVao; //vertex array



void GLinit(int width, int height) {

	bool res = loadOBJ("box.obj", Object::vertices, Object::uvs, Object::normals);
	bool res1 = loadOBJ("Lampara.obj", Object1::vertices, Object1::uvs, Object1::normals);
	bool res2 = loadOBJ("Lampara.obj", Object2::vertices, Object2::uvs, Object2::normals);
	bool res3 = loadOBJ("Lampara.obj", Object3::vertices, Object3::uvs, Object3::normals);
	glViewport(0, 0, width, height);

	glClearColor(0.2f, 0.2f, 0.2f, 1.f);

	glClearDepth(1.f);

	glDepthFunc(GL_LEQUAL);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);



	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);



	// Setup shaders & geometry

	Axis::setupAxis();

	Cube::setupCube();

	Object::setupObject();

	Object1::setupObject1();
	
	Object2::setupObject2();

	Object3::setupObject3();

	/////////////////////////////////////////////////////TODO

	// Do your init code here

	// ...

	// ...

	// ...

	/////////////////////////////////////////////////////////

	glGenVertexArrays(1, &myVao);

	myRenderProgram = compile_shaders();

}



void GLcleanup() {

	Axis::cleanupAxis();

	Cube::cleanupCube();

	Object::cleanupObject();

	Object1::cleanupObject1();

	Object2::cleanupObject2();

	Object3::cleanupObject3();

	/////////////////////////////////////////////////////TODO

	// Do your cleanup code here

	// ...

	// ...

	// ...

	/////////////////////////////////////////////////////////

}

float currentTime = 0;

void GLrender(float dt) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));
	RV::_MVP = RV::_projection * RV::_modelView;

	Axis::drawAxis();

	/////////////////////////////////////////////////////TODO

	// Do your render code here

	// ...


	 ///////////////////////////////////////////////////////////////////////////CUBS

	//Cube::drawCube();

	// ...

	// ...

	/////////////////////////////////////////////////////////

	Object::drawObject(currentTime);

	Object1::drawObject1(currentTime);

	Object2::drawObject2(currentTime);

	Object3::drawObject3(currentTime);

	//EX1:
	//glPointSize(40.0f);

	glBindVertexArray(myVao);
	glUseProgram(myRenderProgram);

	//EX1:
	//glDrawArrays(GL_POINTS, 0, 1);

	//EX2:
	//glDrawArrays(GL_TRIANGLES, 0, 3);



	ImGui::Render();

}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		// ...
		// ...
		// ...
		/////////////////////////////////////////////////////////
		ImGui::DragFloat("Diffuse", &ImGui::Diffuse, 0.02f, 0.0f, 1.0f);
		ImGui::DragFloat("Specular", &ImGui::Specular, 0.02f, 0.0f, 1.0f);
		ImGui::DragFloat("Ambient", &ImGui::Ambient, 0.02f, 0.0, 1.0f);
		ImGui::DragFloat("Light Power", &ImGui::LightPower, 0.02f, 0.0f, 1.0f);
		ImGui::DragFloat("Light X", &ImGui::lightPosition[0], 0.02f, -10.0f, 10.0f);
		ImGui::DragFloat("Light Y", &ImGui::lightPosition[1], 0.02f, -10.0f, 10.0f);
		ImGui::DragFloat("Light Z", &ImGui::lightPosition[2], 0.02f, -10.0f, 10.0f);

	}
	// .........................

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}