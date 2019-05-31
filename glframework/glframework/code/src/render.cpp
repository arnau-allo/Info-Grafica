#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <ctime>
#include <time.h>
#include <fstream>
#include <string>
#include <sstream>

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
	glm::vec3 lightColor = { 0.3f, 0.04f, 0.12f };
	float Velocity = 0.1;
	float Diffuse, Specular, Ambient, LightPower;
	int exercise1, exercise2;
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
	bool firstTime, secondTime, init;
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
GLuint compileShader(GLenum shaderType, std::string shaderName, const char* name = "") {

	std::stringstream stream;
	std::string string;

	std::ifstream myFile(shaderName);
	if (myFile.is_open())
	{
		stream << myFile.rdbuf();
		myFile.close();
		string = stream.str();
	}

	const GLchar * shaderStr = string.c_str();

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
	const char* Axis_vertShader;
	const char* Axis_fragShader;

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

		AxisShader[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "AxisVert");
		AxisShader[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "AxisFrag");

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


namespace Luz {

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	glm::vec3 position;
	glm::vec4 lightColor = { 1.f, 1.f, 0.f, 0.f };
	float angle;
	float rotationX;
	bool goingRight;
	GLuint luzVao;
	GLuint luzVbo[3];
	GLuint luzShaders[2];
	GLuint luzProgram;

	glm::mat4 luzMat = glm::mat4(1.f);
	glm::vec4 luzColor = { 1.f, 1.f, 0.f, 0.f };

	char* luz_vertShader;
	char* luz_fragShader;

	void setupLuz() {
		glBufferData(GL_ARRAY_BUFFER, Luz::vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &luzVao);
		glBindVertexArray(luzVao);
		glGenBuffers(2, luzVbo);

		glBindBuffer(GL_ARRAY_BUFFER, luzVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, luzVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		luzShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
		luzShaders[1] = compileShader( GL_FRAGMENT_SHADER,"BasicFrag.txt", "cubeFrag");

		luzProgram = glCreateProgram();
		glAttachShader(luzProgram, luzShaders[0]);
		glAttachShader(luzProgram, luzShaders[1]);
		glBindAttribLocation(luzProgram, 0, "in_Position");
		glBindAttribLocation(luzProgram, 1, "in_Normal");
		linkProgram(luzProgram);
		if (RV::init)
		{
			luzMat = glm::translate(luzMat, glm::vec3(5.55f, 6.3f, .0f));
			position.x = 5.55f;
			position.y = 6.3f;
			position.z = 0.0f;
			angle = 0.0f;
			rotationX = 0.0f;
			goingRight = true;
		}
	}

	void cleanupLuz() {
		glDeleteBuffers(2, luzVbo);
		glDeleteVertexArrays(1, &luzVao);

		glDeleteProgram(luzProgram);
		glDeleteShader(luzShaders[0]);
		glDeleteShader(luzShaders[1]);
	}

	void drawLuz(float currentTime) {

		glBindVertexArray(luzVao);
		glUseProgram(luzProgram);

		glUniformMatrix4fv(glGetUniformLocation(luzProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(luzMat));
		glUniformMatrix4fv(glGetUniformLocation(luzProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(luzProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform3f(glGetUniformLocation(luzProgram, "color"), luzColor[0], luzColor[1], luzColor[2]);
		glDrawArrays(GL_TRIANGLES, 0, Luz::vertices.size());

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

	char* gallina_vertShader;
	char* gallina_fragShader;

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
		if (ImGui::exercise1 == 1)
		{
			gallinaShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
			gallinaShaders[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "cubeFrag");
		}
		else if (ImGui::exercise2 == 1)
		{
			gallinaShaders[0] = compileShader(GL_VERTEX_SHADER, "GallinaVert.txt", "cubeVert");
			gallinaShaders[1] = compileShader(GL_FRAGMENT_SHADER, "GallinaFrag.txt", "cubeFrag");
		}
		

		gallinaProgram = glCreateProgram();
		glAttachShader(gallinaProgram, gallinaShaders[0]);
		glAttachShader(gallinaProgram, gallinaShaders[1]);
		glBindAttribLocation(gallinaProgram, 0, "in_Position");
		glBindAttribLocation(gallinaProgram, 1, "in_Normal");
		linkProgram(gallinaProgram);
		if (RV::init)
		{
			gallinaMat = glm::rotate(gallinaMat, (float)glm::radians(90.0f), glm::vec3(0, 1, 0));
			gallinaMat = glm::translate(gallinaMat, glm::vec3(-0.2f, 5.8f, 5.4f));//-0.15, 11.35
			lastX = 5.4f;
			lastY = 5.8;
			angle = 0.0f;
		}
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
		glUniform3f(glGetUniformLocation(gallinaProgram, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(gallinaProgram, "light_Position2"), Luz::position[0] + Luz::rotationX/100, Luz::position[1], Luz::position[2]);
		glUniform3f(glGetUniformLocation(gallinaProgram, "color"), gallinaColor[0], gallinaColor[1], gallinaColor[2]);
		glUniform3f(glGetUniformLocation(gallinaProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniform3f(glGetUniformLocation(gallinaProgram, "light_Color2"), Luz::lightColor[0], Luz::lightColor[1], Luz::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(gallinaProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(gallinaProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(gallinaProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(gallinaProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(gallinaProgram, "light_Power"), ImGui::LightPower);
		//glUniform4f(glGetUniformLocation(gallinaProgram, "Color"), gallinaColor[0], gallinaColor[1], gallinaColor[2], gallinaColor[3]);
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
	glm::vec4 trumpColor = { 1.0f, 0.f, 0.f, 0.f };

	char* trump_vertShader;
	char* trump_fragShader;

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

		if (ImGui::exercise1 == 1)
		{
			trumpShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
			trumpShaders[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "cubeFrag");
		}
		else if (ImGui::exercise2 == 1)
		{
			trumpShaders[0] = compileShader(GL_VERTEX_SHADER, "TrumpVert.txt", "cubeVert");
			trumpShaders[1] = compileShader(GL_FRAGMENT_SHADER, "TrumpFrag.txt", "cubeFrag");
		}

		trumpProgram = glCreateProgram();
		glAttachShader(trumpProgram, trumpShaders[0]);
		glAttachShader(trumpProgram, trumpShaders[1]);
		glBindAttribLocation(trumpProgram, 0, "in_Position");
		glBindAttribLocation(trumpProgram, 1, "in_Normal");
		linkProgram(trumpProgram);
		if (RV::init)
		{
			trumpMat = glm::rotate(trumpMat, (float)glm::radians(180.0f), glm::vec3(0, 1, 0));
			trumpMat = glm::translate(trumpMat, glm::vec3(-5.7f, 5.8f, -0.2f));
			lastX = -5.7f;
			lastY = 5.8;
		}
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
		glUniform3f(glGetUniformLocation(trumpProgram, "light_Position"), ImGui::lightPosition[0], ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(trumpProgram, "light_Position2"), Luz::position[0] + Luz::rotationX / 100, Luz::position[1], Luz::position[2]);
		glUniform3f(glGetUniformLocation(trumpProgram, "light_Position2"), Luz::position[0], Luz::position[1], Luz::position[2]);
		glUniform3f(glGetUniformLocation(trumpProgram, "color"), trumpColor[0], trumpColor[1], trumpColor[2]);
		glUniform3f(glGetUniformLocation(trumpProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniform3f(glGetUniformLocation(trumpProgram, "light_Color2"), Luz::lightColor[0], Luz::lightColor[1], Luz::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(trumpProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(trumpProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(trumpProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(trumpProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(trumpProgram, "light_Power"), ImGui::LightPower);
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
	glm::vec4 cabinaColor = { 0.0f, 1.f, 0.1f, 0.f };

	char* cabina_vertShader;
	char* cabina_fragShader;

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

		cabinaShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
		cabinaShaders[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "cubeFrag");


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
		glUniform3f(glGetUniformLocation(cabinaProgram, "light_Position"), ImGui::lightPosition[0] + Luz::rotationX / 100, ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(cabinaProgram, "color"), cabinaColor[0], cabinaColor[1], cabinaColor[2]);
		glUniform3f(glGetUniformLocation(cabinaProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(cabinaProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(cabinaProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(cabinaProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(cabinaProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(cabinaProgram, "light_Power"), ImGui::LightPower);
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
	glm::vec4 radiosColor = { 0.1f, 0.1f, 1.f, 0.f };

	char* radios_vertShader;
	char* radios_fragShader;

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

		radiosShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
		radiosShaders[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "cubeFrag");
		

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
		glUniform3f(glGetUniformLocation(radiosProgram, "light_Position"), ImGui::lightPosition[0] + Luz::rotationX / 100, ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(radiosProgram, "color"), radiosColor[0], radiosColor[1], radiosColor[2]);
		glUniform3f(glGetUniformLocation(radiosProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(radiosProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(radiosProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(radiosProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(radiosProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(radiosProgram, "light_Power"), ImGui::LightPower);
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
	glm::vec4 soporteColor = { 0.1f, 0.1f, 1.0f, 0.f };

	char* soporte_vertShader;
	char* soporte_fragShader;

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

		soporteShaders[0] = compileShader(GL_VERTEX_SHADER, "BasicVert.txt", "cubeVert");
		soporteShaders[1] = compileShader(GL_FRAGMENT_SHADER, "BasicFrag.txt", "cubeFrag");


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
		glUniform3f(glGetUniformLocation(soporteProgram, "light_Position"), ImGui::lightPosition[0] + Luz::rotationX / 100, ImGui::lightPosition[1], ImGui::lightPosition[2]);
		glUniform3f(glGetUniformLocation(soporteProgram, "color"), soporteColor[0], soporteColor[1], soporteColor[2]);
		glUniform3f(glGetUniformLocation(soporteProgram, "light_Color"), ImGui::lightColor[0], ImGui::lightColor[1], ImGui::lightColor[2]);
		glUniformMatrix4fv(glGetUniformLocation(soporteProgram, "camera_Point"), 1, GL_FALSE, glm::value_ptr(RenderVars::_cameraPoint));
		glUniform1f(glGetUniformLocation(soporteProgram, "kd"), ImGui::Diffuse);
		glUniform1f(glGetUniformLocation(soporteProgram, "ka"), ImGui::Ambient);
		glUniform1f(glGetUniformLocation(soporteProgram, "ks"), ImGui::Specular);
		glUniform1f(glGetUniformLocation(soporteProgram, "light_Power"), ImGui::LightPower);
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

	bool res = loadOBJ("box.obj", Luz::vertices, Luz::uvs, Luz::normals);
	bool res1 = loadOBJ("Gallina.obj", Gallina::vertices, Gallina::uvs, Gallina::normals);
	bool res2 = loadOBJ("Trump.obj", Trump::vertices, Trump::uvs, Trump::normals);
	bool res3 = loadOBJ("Cabina.obj", Cabina::vertices, Cabina::uvs, Cabina::normals);
	bool res4 = loadOBJ("Radios.obj", Radios::vertices, Radios::uvs, Radios::normals);
	bool res5 = loadOBJ("Soporte.obj", Soporte::vertices, Soporte::uvs, Soporte::normals);
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
	RV::secondTime = false;
	RV::init = true;
	ImGui::exercise1 = 1;
	ImGui::exercise2 = 0;

	//carregar shaders


	// Setup shaders & geometry

	Axis::setupAxis();

	Luz::setupLuz();

	Gallina::setupGallina();

	Trump::setupTrump();
	
	Cabina::setupCabina();

	Radios::setupRadios();

	Soporte::setupSoporte();

	RV::init = false;

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

	Luz::cleanupLuz();

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

	double a = CLOCKS_PER_SEC; 

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (ImGui::exercise1 & 1)
	{
		Gallina::cleanupGallina();

		Trump::cleanupTrump();

		Gallina::setupGallina();

		Trump::setupTrump();

		ImGui::exercise1--;
		ImGui::exercise2 = 0;
	}
	else if(ImGui::exercise2 & 1)
	{
		Gallina::cleanupGallina();

		Trump::cleanupTrump();

		Gallina::setupGallina();

		Trump::setupTrump();

		ImGui::exercise2--;
		ImGui::exercise1 = 0;
	}

	if (RV::firstTime && ((std::clock() - RV::start) / a) > 1)
	{
		RV::firstTime = false;
		RV::secondTime = true;
		RV::start = std::clock();
	}
	else if (RV::secondTime && ((std::clock() - RV::start) / a) > 1)
	{
		RV::secondTime = false;
		RV::start = std::clock();
	}
	else
	{
		if (((std::clock() - RV::start) / (double)CLOCKS_PER_SEC) > 1 && !RV::lookingTrump)
		{
			RV::lookingTrump = true;
			RV::start = std::clock();
		}
		else if (((std::clock() - RV::start) / (double)CLOCKS_PER_SEC) > 1)
		{
			RV::lookingTrump = false;
			RV::start = std::clock();
		}
	}

	Cabina::mainX = Cabina::lastX;
	Cabina::mainY = Cabina::lastY;

	Axis::drawAxis();

	/////////////////////////////////////////////////////TODO

	// Do your render code here

	// ...


	 ///////////////////////////////////////////////////////////////////////////CUBS

	//Cube::drawCube();

	// ...

	// ...

	/////////////////////////////////////////////////////////

	Luz::luzMat = glm::translate(Luz::luzMat, glm::vec3(((cos(glm::radians(Luz::angle)) * 5.55)) - Luz::position.x, ((sin(glm::radians(Luz::angle)) * 5.55) + 6.2) - Luz::position.y, 0.0f));
	Luz::position.x = (cos(glm::radians(Luz::angle)) * 5.55);
	Luz::position.y = (sin(glm::radians(Luz::angle)) * 5.55) + 6.2;

	if (Luz::goingRight)
	{
		Luz::luzMat = glm::translate(Luz::luzMat, glm::vec3(0.01f, 0.0f, 0.0f));
		Luz::rotationX++;
		if (Luz::rotationX > 20) Luz::goingRight = false;
	}
	else
	{
		Luz::luzMat = glm::translate(Luz::luzMat, glm::vec3(-0.01f, 0.0f, 0.0f));
		Luz::rotationX--;
		if (Luz::rotationX < -20) Luz::goingRight = true;
	}

	Luz::drawLuz(currentTime);

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

	Radios::radiosMat = glm::rotate(Radios::radiosMat, (float)glm::radians(ImGui::Velocity), glm::vec3(0, 0, 1));
	Radios::drawRadios(currentTime);
	Luz::angle += ImGui::Velocity;
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
	if (!RV::firstTime && !RV::secondTime)
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
		ImGui::DragFloat("Diffuse", &ImGui::Diffuse, 0.02f, 0.0f, 1.0f);
		ImGui::DragFloat("Specular", &ImGui::Specular, 0.02f, 0.0f, 1.0f);
		ImGui::DragFloat("Ambient", &ImGui::Ambient, 0.02f, 0.0, 1.0f);
		ImGui::DragFloat("Light Power", &ImGui::LightPower, 0.02f, 0.0f, 1.0f);
		if (ImGui::Button("Exercise 1"))
		{
			ImGui::exercise1++;
		}
		if (ImGui::Button("Exercise 2"))
		{
			ImGui::exercise2++;
		}
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