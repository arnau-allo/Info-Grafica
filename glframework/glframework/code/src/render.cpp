#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <ctime>
#include <time.h>

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
	float Velocity = 0.1;
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
	const float zNear = 0.01f;
	const float zFar = 50.f;

	bool lookingTrump = false;
	bool firstTime;
	double start;

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



////////////////////////////////////////////////// GALLINA
namespace Gallina {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	float lastX;
	float lastY;
	float angle;
	GLuint gallinaVao;
	GLuint gallinaVbo[3];
	GLuint gallinaShaders[2];
	GLuint gallinaProgram;

	glm::mat4 gallinaMat = glm::mat4(1.f);
	glm::vec4 gallinaColor = { 0.1f, 1.f, 1.f, 0.f };

	const char* gallina_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
}";
	const char* gallina_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 Color;\n\
void main() {\n\
	out_Color = Color;\n\
}";

	void setupGallina() {
		glBufferData(GL_ARRAY_BUFFER, Gallina::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &gallinaVao);
		glBindVertexArray(gallinaVao);
		glGenBuffers(2, gallinaVbo);

		glBindBuffer(GL_ARRAY_BUFFER, gallinaVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, gallinaVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		gallinaShaders[0] = compileShader(gallina_vertShader, GL_VERTEX_SHADER, "cubeVert");
		gallinaShaders[1] = compileShader(gallina_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		gallinaProgram = glCreateProgram();
		glAttachShader(gallinaProgram, gallinaShaders[0]);
		glAttachShader(gallinaProgram, gallinaShaders[1]);
		glBindAttribLocation(gallinaProgram, 0, "in_Position");
		glBindAttribLocation(gallinaProgram, 1, "in_Normal");
		linkProgram(gallinaProgram);

		gallinaMat = glm::rotate(gallinaMat, (float)glm::radians(90.0f), glm::vec3(0, 1, 0));
		gallinaMat = glm::translate(gallinaMat, glm::vec3(-0.2f, 5.8f, 5.4f));//-0.15, 11.35
		lastX = 5.4f;
		lastY = 5.8;
		angle = 0.0f;
	}

	void cleanupGallina() {
		glDeleteBuffers(2, gallinaVbo);
		glDeleteVertexArrays(1, &gallinaVao);

		glDeleteProgram(gallinaProgram);
		glDeleteShader(gallinaShaders[0]);
		glDeleteShader(gallinaShaders[1]);
	}

	void drawGallina(float currentTime) {

		glBindVertexArray(gallinaVao);
		glUseProgram(gallinaProgram);

		glUniformMatrix4fv(glGetUniformLocation(gallinaProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(gallinaMat));
		glUniformMatrix4fv(glGetUniformLocation(gallinaProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(gallinaProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(gallinaProgram, "Color"), gallinaColor[0], gallinaColor[1], gallinaColor[2], gallinaColor[3]);
		glDrawArrays(GL_TRIANGLES, 0, Gallina::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}

}

////////////////////////////////////////////////// TRUMP
namespace Trump {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	float lastX;
	float lastY;
	float angle;
	GLuint trumpVao;
	GLuint trumpVbo[3];
	GLuint trumpShaders[2];
	GLuint trumpProgram;

	glm::mat4 trumpMat = glm::mat4(1.f);
	glm::vec4 trumpColor = { 1.0f, 0.1f, 1.f, 0.f };

	const char* trump_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
}";
	const char* trump_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 Color;\n\
void main() {\n\
	out_Color = Color;\n\
}";

	void setupTrump() {
		glBufferData(GL_ARRAY_BUFFER, Trump::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &trumpVao);
		glBindVertexArray(trumpVao);
		glGenBuffers(2, trumpVbo);

		glBindBuffer(GL_ARRAY_BUFFER, trumpVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, trumpVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		trumpShaders[0] = compileShader(trump_vertShader, GL_VERTEX_SHADER, "cubeVert");
		trumpShaders[1] = compileShader(trump_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		trumpProgram = glCreateProgram();
		glAttachShader(trumpProgram, trumpShaders[0]);
		glAttachShader(trumpProgram, trumpShaders[1]);
		glBindAttribLocation(trumpProgram, 0, "in_Position");
		glBindAttribLocation(trumpProgram, 1, "in_Normal");
		linkProgram(trumpProgram);
		trumpMat = glm::rotate(trumpMat, (float)glm::radians(180.0f), glm::vec3(0, 1, 0));
		trumpMat = glm::translate(trumpMat, glm::vec3(-5.7f, 5.8f, -0.2f));
		lastX = -5.7f;
		lastY = 5.8;
	}

	void cleanupTrump() {
		glDeleteBuffers(2, trumpVbo);
		glDeleteVertexArrays(1, &trumpVao);

		glDeleteProgram(trumpProgram);
		glDeleteShader(trumpShaders[0]);
		glDeleteShader(trumpShaders[1]);
	}

	void drawTrump(float currentTime) {

		glBindVertexArray(trumpVao);
		glUseProgram(trumpProgram);

		glUniformMatrix4fv(glGetUniformLocation(trumpProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(trumpMat));
		glUniformMatrix4fv(glGetUniformLocation(trumpProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(trumpProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(trumpProgram, "Color"), trumpColor[0], trumpColor[1], trumpColor[2], trumpColor[3]);
		glDrawArrays(GL_TRIANGLES, 0, Trump::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);

	}

}

////////////////////////////////////////////////// CABINA
namespace Cabina {
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	float lastX;
	float lastY;
	float mainX;
	float mainY;
	float angle;
	GLuint cabinaVao;
	GLuint cabinaVbo[3];
	GLuint cabinaShaders[2];
	GLuint cabinaProgram;

	glm::mat4 cabinaMat = glm::mat4(1.f);
	glm::vec4 cabinaColor = { 1.0f, 1.f, 0.1f, 0.f };

	const char* cabina_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
}";
	const char* cabina_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 Color;\n\
void main() {\n\
	out_Color = Color;\n\
}";

	void setupCabina() {
		glBufferData(GL_ARRAY_BUFFER, Cabina::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &cabinaVao);
		glBindVertexArray(cabinaVao);
		glGenBuffers(2, cabinaVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cabinaVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, cabinaVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cabinaShaders[0] = compileShader(cabina_vertShader, GL_VERTEX_SHADER, "cubeVert");
		cabinaShaders[1] = compileShader(cabina_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		cabinaProgram = glCreateProgram();
		glAttachShader(cabinaProgram, cabinaShaders[0]);
		glAttachShader(cabinaProgram, cabinaShaders[1]);
		glBindAttribLocation(cabinaProgram, 0, "in_Position");
		glBindAttribLocation(cabinaProgram, 1, "in_Normal");
		linkProgram(cabinaProgram);
		cabinaMat = glm::translate(cabinaMat, glm::vec3(5.55f, 6.3f, .0f));
		lastX = 5.55f;
		lastY = 6.3f;
		mainX = lastX;
		mainY = lastY;
		angle = 0.0f;
	}

	void cleanupCabina() {
		glDeleteBuffers(2, cabinaVbo);
		glDeleteVertexArrays(1, &cabinaVao);

		glDeleteProgram(cabinaProgram);
		glDeleteShader(cabinaShaders[0]);
		glDeleteShader(cabinaShaders[1]);
	}

	void drawCabina(float currentTime) {

		glBindVertexArray(cabinaVao);
		glUseProgram(cabinaProgram);

		glUniformMatrix4fv(glGetUniformLocation(cabinaProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(cabinaMat));
		glUniformMatrix4fv(glGetUniformLocation(cabinaProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cabinaProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(cabinaProgram, "Color"), cabinaColor[0], cabinaColor[1], cabinaColor[2], cabinaColor[3]);
		glDrawArrays(GL_TRIANGLES, 0, Cabina::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}

}

////////////////////////////////////////////////// RADIOS
namespace Radios {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint radiosVao;
	GLuint radiosVbo[3];
	GLuint radiosShaders[2];
	GLuint radiosProgram;

	glm::mat4 radiosMat = glm::mat4(1.f);
	glm::vec4 radiosColor = { 0.5f, 0.5f, 0.5f, 0.f };

	const char* radios_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
}";
	const char* radios_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 Color;\n\
void main() {\n\
	out_Color = Color;\n\
}";

	void setupRadios() {
		glBufferData(GL_ARRAY_BUFFER, Radios::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &radiosVao);
		glBindVertexArray(radiosVao);
		glGenBuffers(2, radiosVbo);

		glBindBuffer(GL_ARRAY_BUFFER, radiosVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, radiosVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		radiosShaders[0] = compileShader(radios_vertShader, GL_VERTEX_SHADER, "cubeVert");
		radiosShaders[1] = compileShader(radios_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		radiosProgram = glCreateProgram();
		glAttachShader(radiosProgram, radiosShaders[0]);
		glAttachShader(radiosProgram, radiosShaders[1]);
		glBindAttribLocation(radiosProgram, 0, "in_Position");
		glBindAttribLocation(radiosProgram, 1, "in_Normal");
		linkProgram(radiosProgram);
		radiosMat = glm::translate(radiosMat, glm::vec3(.0f, 6.3f, .0f));
	}

	void cleanupRadios() {
		glDeleteBuffers(2, radiosVbo);
		glDeleteVertexArrays(1, &radiosVao);

		glDeleteProgram(radiosProgram);
		glDeleteShader(radiosShaders[0]);
		glDeleteShader(radiosShaders[1]);
	}

	void drawRadios(float currentTime) {

		glBindVertexArray(radiosVao);
		glUseProgram(radiosProgram);

		glUniformMatrix4fv(glGetUniformLocation(radiosProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(radiosMat));
		glUniformMatrix4fv(glGetUniformLocation(radiosProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(radiosProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(radiosProgram, "Color"), radiosColor[0], radiosColor[1], radiosColor[2], radiosColor[3]);
		glDrawArrays(GL_TRIANGLES, 0, Radios::vertices.size());

		glUseProgram(0);
		glBindVertexArray(0);
	}

}

////////////////////////////////////////////////// SOPORTE
namespace Soporte {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	GLuint soporteVao;
	GLuint soporteVbo[3];
	GLuint soporteShaders[2];
	GLuint soporteProgram;

	glm::mat4 soporteMat = glm::mat4(1.f);
	glm::vec4 soporteColor = { 0.5f, 0.5f, 0.5f, 0.f };

	const char* soporte_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec3 in_Normal;\n\
out vec3 vert_Normal;\n\
out vec3 fragPos;\n\
uniform mat4 objMat;\n\
uniform mat4 mv_Mat;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
	fragPos = vec3(mv_Mat * objMat * vec4(in_Position, 1.0));\n\
	vert_Normal = vec3(mv_Mat * objMat * vec4(in_Normal, 0.0));\n\
}";
	const char* soporte_fragShader =
		"#version 330\n\
in vec3 vert_Normal;\n\
in vec3 fragPos;\n\
out vec4 out_Color;\n\
uniform mat4 mv_Mat;\n\
uniform vec4 Color;\n\
void main() {\n\
	out_Color = Color;\n\
}";

	void setupSoporte() {
		glBufferData(GL_ARRAY_BUFFER, Soporte::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &soporteVao);
		glBindVertexArray(soporteVao);
		glGenBuffers(2, soporteVbo);

		glBindBuffer(GL_ARRAY_BUFFER, soporteVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, soporteVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		soporteShaders[0] = compileShader(soporte_vertShader, GL_VERTEX_SHADER, "cubeVert");
		soporteShaders[1] = compileShader(soporte_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		soporteProgram = glCreateProgram();
		glAttachShader(soporteProgram, soporteShaders[0]);
		glAttachShader(soporteProgram, soporteShaders[1]);
		glBindAttribLocation(soporteProgram, 0, "in_Position");
		glBindAttribLocation(soporteProgram, 1, "in_Normal");
		linkProgram(soporteProgram);
	}

	void cleanupSoporte() {
		glDeleteBuffers(2, soporteVbo);
		glDeleteVertexArrays(1, &soporteVao);

		glDeleteProgram(soporteProgram);
		glDeleteShader(soporteShaders[0]);
		glDeleteShader(soporteShaders[1]);
	}

	void drawSoporte(float currentTime) {

		glBindVertexArray(soporteVao);
		glUseProgram(soporteProgram);

		glUniformMatrix4fv(glGetUniformLocation(soporteProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(soporteMat));
		glUniformMatrix4fv(glGetUniformLocation(soporteProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(soporteProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(soporteProgram, "Color"), soporteColor[0], soporteColor[1], soporteColor[2], soporteColor[3]);
		glDrawArrays(GL_TRIANGLES, 0, Soporte::vertices.size());

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

	bool res = loadOBJ("Gallina.obj", Gallina::vertices, Gallina::uvs, Gallina::normals);
	bool res1 = loadOBJ("Trump.obj", Trump::vertices, Trump::uvs, Trump::normals);
	bool res2 = loadOBJ("Cabina.obj", Cabina::vertices, Cabina::uvs, Cabina::normals);
	bool res3 = loadOBJ("Radios.obj", Radios::vertices, Radios::uvs, Radios::normals);
	bool res4 = loadOBJ("Soporte.obj", Soporte::vertices, Soporte::uvs, Soporte::normals);
	glViewport(0, 0, width, height);

	glClearColor(0.2f, 0.2f, 0.2f, 1.f);

	glClearDepth(1.f);

	glDepthFunc(GL_LEQUAL);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);



	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));
	RV::_MVP = RV::_projection * RV::_modelView;
	RV::start = std::clock();
	RV::firstTime = true;

	// Setup shaders & geometry

	Axis::setupAxis();

	Cube::setupCube();

	Gallina::setupGallina();

	Trump::setupTrump();
	
	Cabina::setupCabina();

	Radios::setupRadios();

	Soporte::setupSoporte();

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

	Gallina::cleanupGallina();

	Trump::cleanupTrump();

	Cabina::cleanupCabina();

	Radios::cleanupRadios();

	Soporte::cleanupSoporte();

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

	if (((std::clock() - RV::start) / (double) CLOCKS_PER_SEC) > 1 && !RV::lookingTrump)
	{
		if (RV::firstTime) RV::firstTime = false;
		printf("LOOKING AT TRUMP");
		RV::lookingTrump = true;
		RV::start = std::clock();
	}
	else if (((std::clock() - RV::start) / (double)CLOCKS_PER_SEC) > 1)
	{
		printf("LOOKING AT CHICKEN");
		RV::lookingTrump = false;
		RV::start = std::clock();
	}	

	Axis::drawAxis();

	/////////////////////////////////////////////////////TODO

	// Do your render code here

	// ...


	 ///////////////////////////////////////////////////////////////////////////CUBS

	//Cube::drawCube();

	// ...

	// ...

	/////////////////////////////////////////////////////////

	Cabina::mainX = Cabina::lastX;
	Cabina::mainY = Cabina::lastY;

	Gallina::gallinaMat = glm::translate(Gallina::gallinaMat, glm::vec3(0.0f, ((sin(glm::radians(Gallina::angle)) * 5.55) + 5.8) - Gallina::lastY, ((cos(glm::radians(Gallina::angle)) * 5.55) - 0.15) - Gallina::lastX));
	Gallina::lastX = (cos(glm::radians(Gallina::angle)) * 5.55) - 0.15;
	Gallina::lastY = (sin(glm::radians(Gallina::angle)) * 5.55) + 5.8;
	Gallina::drawGallina(currentTime);

	Trump::trumpMat = glm::translate(Trump::trumpMat, glm::vec3(-((cos(glm::radians(Trump::angle)) * 5.55) + 0.15) - Trump::lastX, ((sin(glm::radians(Trump::angle)) * 5.55) + 5.8) - Trump::lastY, 0.0f));
	Trump::lastX = -((cos(glm::radians(Trump::angle)) * 5.55) + 0.15);
	Trump::lastY = (sin(glm::radians(Trump::angle)) * 5.55) + 5.8;
	Trump::drawTrump(currentTime);

	for (int i = 0; i < 20; i++)
	{
		Cabina::cabinaMat = glm::translate(Cabina::cabinaMat, glm::vec3((cos(glm::radians(Cabina::angle)) * 5.55) - Cabina::lastX, ((sin(glm::radians(Cabina::angle)) * 5.55) + 6.3) - Cabina::lastY, .0f));
		Cabina::drawCabina(currentTime);
		Cabina::lastX = (cos(glm::radians(Cabina::angle)) * 5.55);
		Cabina::lastY = (sin(glm::radians(Cabina::angle)) * 5.55) + 6.3;
		Cabina::angle += 18;
		if (Cabina::angle >= 360) Cabina::angle -= 360;
	}

	//Cabina::drawCabina(currentTime);

	Radios::radiosMat = glm::rotate(Radios::radiosMat, (float)glm::radians(ImGui::Velocity), glm::vec3(0, 0, 1));
	Radios::drawRadios(currentTime);
	Gallina::angle += ImGui::Velocity;
	Trump::angle += ImGui::Velocity;
	Cabina::angle += ImGui::Velocity;


	Soporte::drawSoporte(currentTime);

	//EX1:
	//glPointSize(40.0f);

	glBindVertexArray(myVao);
	glUseProgram(myRenderProgram);

	//EX1:
	//glDrawArrays(GL_POINTS, 0, 1);

	//EX2:
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	if (!RV::firstTime)
	{
		if (!RV::lookingTrump)
		{
			RV::panv[0] = -Trump::lastX + 0.08f;
			RV::panv[1] = Trump::lastY + 0.4f;
			RV::panv[2] = 0.4;
			RV::_modelView = glm::lookAt(glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]), glm::vec3(Gallina::lastX, Gallina::lastY + 0.2f, 0.2f), glm::vec3(0, 1, 0));
		}
		else
		{
			RV::panv[0] = Gallina::lastX - 0.26f;
			RV::panv[1] = Gallina::lastY + 0.4f;
			RV::panv[2] = 0.4;
			RV::_modelView = glm::lookAt(glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]), glm::vec3(-Trump::lastX, Trump::lastY + 0.2f, 0.2f), glm::vec3(0, 1, 0));
		}
		RV::_MVP = RV::_projection * RV::_modelView;
	}

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
		ImGui::DragFloat("Velocity", &ImGui::Velocity, 0.01f, 0.1f, 0.5f);
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