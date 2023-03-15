// Grafika Komputerowa i Wizualizacja 2022
// Akwarium
// Agnieszka Grzymska 148295, Łukasz Jankowski 148081

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "./include/myCube.h"
#include "./include/constants.h"
#include "./include/allmodels.h"
#include "./include/lodepng.h"
#include "./include/shaderprogram.h"
#include "./include/ObjLoader.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "./include/myTeapot.h"

#define FISH 0
#define TANK 1
#define ROCK1 2
#define ROCK2 3
#define ROCK3 4
#define ROCK4 5
#define ROCK5 6
#define ROCK6 7
#define AKWAR 8
#define FISH1 9
#define NEMO 10
#define PLANTS 11

#define TEX_FISH 0
#define TEX_TANK 1
#define TEX_ROCK1 2 
#define TEX_ROCK2 3 
#define TEX_ROCK3 4 
#define TEX_ROCK4 5 
#define TEX_ROCK5 6 
#define TEX_ROCK6 7 
#define TEX_ROCK7 8 
#define TEX_BOTTOM 9
#define TEX_FISH2 10
#define TEX_FISH3 11
#define TEX_PLANT 12
#define TEX_FISH4 13

glm::vec4 light1 = glm::vec4(-2.5, 6, 0, 1);
glm::vec4 light2 = glm::vec4(2.5, 6, 0, 1);


float speed = 1.5;

struct MyVertex {
	std::vector<glm::vec4> Vertices;
	std::vector<glm::vec4> Normals;
	std::vector<glm::vec2> TexCoords;
	std::vector<unsigned int> Indices;
};

std::vector<MyVertex> models;
std::vector<GLuint> texs;

ShaderProgram* waterShader;
ShaderProgram* phongShader;
ShaderProgram* glassShader;


void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod) {
	const float cameraSpeed = 0.13f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraUp;
}

bool firstMouse;
float lastX = 400, lastY = 300;
float yaw = -90.0f;
float pitch;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

void loadModel(std::string filename, int model_i) {
	models.push_back(MyVertex());
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

	std::cout << importer.GetErrorString() << std::endl;

	aiMesh* mesh = scene->mMeshes[0];
	for (int i = 0; i < mesh->mNumVertices; i++) {

		aiVector3D vertex = mesh->mVertices[i];
		models[model_i].Vertices.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));

		aiVector3D normal = mesh->mNormals[i];
		models[model_i].Normals.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));

		unsigned int liczba_zest = mesh->GetNumUVChannels();
		unsigned int wymiar_wsp_tex = mesh->mNumUVComponents[0];

		aiVector3D texCoord = mesh->mTextureCoords[0][i];
		models[model_i].TexCoords.push_back(glm::vec2(texCoord.x, texCoord.y));
	}

	for (int i = 0; i < mesh->mNumFaces; i++) {
		aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++) {
			models[model_i].Indices.push_back(face.mIndices[j]);
		}
	}
}

void readTexture(const char* filename, int tex_i) {
	texs.push_back(0);
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image;   //Alokuj wektor do wczytania obrazka
	unsigned width, height;   //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	texs[tex_i] = tex;
}

void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
	glClearColor(0.85, 0.85, 0.85, 1); 
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);


	readTexture("./img/fish.png", TEX_FISH);
	readTexture("./img/tank2.png", TEX_TANK);
	readTexture("./img/rock0.png", TEX_ROCK1);
	readTexture("./img/rock1.png", TEX_ROCK2);
	readTexture("./img/rock2.png", TEX_ROCK3);
	readTexture("./img/rock3.png", TEX_ROCK4);
	readTexture("./img/rock4.png", TEX_ROCK5);
	readTexture("./img/rock5.png", TEX_ROCK6);
	readTexture("./img/rock6.png", TEX_ROCK7);
	readTexture("./img/sand.png", TEX_BOTTOM);
	readTexture("./img/fish2.png", TEX_FISH2);
	readTexture("./img/fish3.png", TEX_FISH3);
	readTexture("./img/lisc.png", TEX_PLANT);
	readTexture("./img/Helena.png", TEX_FISH4);

	loadModel(std::string("models/fish.fbx"), FISH);
	loadModel(std::string("models/tank.fbx"), TANK);
	loadModel(std::string("models/Rock0.fbx"), ROCK1);
	loadModel(std::string("models/Rock1.fbx"), ROCK2);
	loadModel(std::string("models/Rock2.fbx"), ROCK3);
	loadModel(std::string("models/Rock3.fbx"), ROCK4);
	loadModel(std::string("models/Rock4.fbx"), ROCK5);
	loadModel(std::string("models/Rock5.fbx"), ROCK6);
	loadModel(std::string("models/akwarium.obj"), AKWAR);
	loadModel(std::string("models/fish_h.obj"), FISH1);
	loadModel(std::string("models/nemo.obj"), NEMO);
	loadModel(std::string("models/plants.obj"), PLANTS);

	waterShader = new ShaderProgram("v_water.glsl", NULL, "f_water.glsl");
	phongShader = new ShaderProgram("v_phong.glsl", NULL, "f_phong.glsl");
	glassShader = new ShaderProgram("v_glass.glsl", NULL, "f_glass.glsl");
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();

	glDeleteTextures(1, &texs[TEX_TANK]);
	glDeleteTextures(1, &texs[TEX_ROCK1]);
	glDeleteTextures(1, &texs[TEX_ROCK2]);
	glDeleteTextures(1, &texs[TEX_ROCK3]);
	glDeleteTextures(1, &texs[TEX_ROCK4]);
	glDeleteTextures(1, &texs[TEX_ROCK5]);
	glDeleteTextures(1, &texs[TEX_ROCK6]);
	glDeleteTextures(1, &texs[TEX_ROCK7]);
	glDeleteTextures(1, &texs[TEX_BOTTOM]);
}

void drawGlass(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	glassShader->use();

	glUniformMatrix4fv(glassShader->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(glassShader->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(glassShader->u("M"), 1, false, glm::value_ptr(M)); 

	glEnableVertexAttribArray(glassShader->a("vertex"));
	glVertexAttribPointer(glassShader->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

	glEnableVertexAttribArray(glassShader->a("normal"));
	glVertexAttribPointer(glassShader->a("normal"), 4, GL_FLOAT, false, 0, myCubeVertexNormals);

	glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

	glDisableVertexAttribArray(glassShader->a("vertex"));
	glDisableVertexAttribArray(glassShader->a("normal"));
}

void drawButtom(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	float bottomTexCoords[] = {
		4.0f, 4.0f,	  0.0f, 0.0f,    0.0f, 4.0f,
		4.0f, 4.0f,   4.0f, 0.0f,    0.0f, 0.0f,
	};

	float bottomVertices[] = {
		8.0f,-6.0f, 6.0f,1.0f,
		8.0f, 6.0f,-6.0f,1.0f,
		8.0f,-6.0f,-6.0f,1.0f,

		8.0f,-6.0f, 6.0f,1.0f,
		8.0f, 6.0f, 6.0f,1.0f,
		8.0f, 6.0f,-6.0f,1.0f,
	};

	float bottomVertexNormals[] = {
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 0.0f,

		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f,
	};

	int bottomVertexCount = 6;

	waterShader->use();

	glUniformMatrix4fv(waterShader->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(waterShader->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(waterShader->u("M"), 1, false, glm::value_ptr(M));

	glEnableVertexAttribArray(waterShader->a("vertex"));
	glVertexAttribPointer(waterShader->a("vertex"), 4, GL_FLOAT, false, 0, bottomVertices);

	glEnableVertexAttribArray(waterShader->a("normal"));
	glVertexAttribPointer(waterShader->a("normal"), 4, GL_FLOAT, false, 0, bottomVertexNormals);

	glEnableVertexAttribArray(waterShader->a("texCoord"));
	glVertexAttribPointer(waterShader->a("texCoord"), 2, GL_FLOAT, false, 0, bottomTexCoords);

	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_S,
		GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_T,
		GL_REPEAT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[TEX_BOTTOM]);
	glUniform1i(waterShader->u("tex"), 0);

	glDrawArrays(GL_TRIANGLES, 0, bottomVertexCount);

	glDisableVertexAttribArray(waterShader->a("vertex"));
	glDisableVertexAttribArray(waterShader->a("normal"));
	glDisableVertexAttribArray(waterShader->a("texCoord"));
}

void drawModel(glm::mat4 P, glm::mat4 V, glm::mat4 M, int model_i, int texture) {
	waterShader->use();

	glUniformMatrix4fv(waterShader->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(waterShader->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(waterShader->u("M"), 1, false, glm::value_ptr(M));
	glUniform4fv(waterShader->u("light1"), 1, glm::value_ptr(light1));
	glUniform4fv(waterShader->u("light2"), 1, glm::value_ptr(light2));

	glEnableVertexAttribArray(waterShader->a("vertex"));
	glVertexAttribPointer(waterShader->a("vertex"), 4, GL_FLOAT, false, 0, models[model_i].Vertices.data()); 

	glEnableVertexAttribArray(waterShader->a("texCoord"));
	glVertexAttribPointer(waterShader->a("texCoord"), 2, GL_FLOAT, false, 0, models[model_i].TexCoords.data()); 
	 
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[texture]);
	glUniform1i(waterShader->u("tex"), 0);

	glDrawElements(GL_TRIANGLES, models[model_i].Indices.size(), GL_UNSIGNED_INT, models[model_i].Indices.data());

	glDisableVertexAttribArray(waterShader->a("vertex"));
	glDisableVertexAttribArray(waterShader->a("normal"));
	glDisableVertexAttribArray(waterShader->a("color"));
}

void drawRocks(glm::mat4 P, glm::mat4 V) {
	glm::mat4 rocks = glm::mat4(1.0f);
	rocks = glm::translate(rocks, glm::vec3(0.0f, -7.85f, 0.0f));
	rocks = glm::scale(rocks, glm::vec3(0.005f, 0.005f, 0.005f));
	rocks = glm::scale(rocks, glm::vec3(0.50f, 0.5f, 0.5f));
	float wsp = 400;

	glm::mat4 rock1 = glm::translate(rocks, glm::vec3(0.0f, 0.0f, 0.0f));
	drawModel(P, V, rock1, ROCK1, TEX_ROCK1);

	glm::mat4 rock2 = glm::translate(rocks, glm::vec3(wsp * 3.0f, 0.0f, 0.0f));
	drawModel(P, V, rock2, ROCK2, TEX_ROCK2);

	glm::mat4 rock3 = glm::translate(rocks, glm::vec3(-wsp * 3.0f, wsp * 0.0f, -wsp * 4.0f));
	drawModel(P, V, rock3, ROCK3, TEX_ROCK3);

	glm::mat4 rock4 = glm::translate(rocks, glm::vec3(-wsp * 1.0f, wsp * 0.0f, wsp * 2.4f));
	drawModel(P, V, rock4, ROCK3, TEX_ROCK4);

	glm::mat4 rock5 = glm::translate(rocks, glm::vec3(wsp * 3.0f, wsp * 0.0f, wsp * 4.7f));
	drawModel(P, V, rock5, ROCK3, TEX_ROCK5);

	glm::mat4 rock6 = glm::translate(rocks, glm::vec3(-wsp * 3.0f, wsp * 0.0f, -wsp * 4.0f));
	drawModel(P, V, rock6, ROCK2, TEX_ROCK2);

	glm::mat4 rock7 = glm::translate(rocks, glm::vec3(-wsp * 3.0f, wsp * 0.0f, wsp * 4.0f));
	drawModel(P, V, rock7, ROCK1, TEX_ROCK7);

	glm::mat4 rock8 = glm::translate(rocks, glm::vec3(wsp * 3.0f, wsp * 0.0f, -wsp * 4.0f));
	drawModel(P, V, rock8, ROCK3, TEX_ROCK4);

	glm::mat4 rock9 = glm::translate(rocks, glm::vec3(0.0f, 0.0f, 0.0f));
	drawModel(P, V, rock9, ROCK1, TEX_ROCK1);

	glm::mat4 rock10 = glm::translate(rocks, glm::vec3(wsp * 3.0f, 0.0f, 0.0f));
	drawModel(P, V, rock10, ROCK2, TEX_ROCK2);

	glm::mat4 rock11 = glm::translate(rocks, glm::vec3(-wsp * 2.0f, wsp * 0.0f, -wsp * 2.0f));
	drawModel(P, V, rock11, ROCK3, TEX_ROCK3);

	glm::mat4 rock12 = glm::translate(rocks, glm::vec3(-wsp * 5.0f, wsp * 0.0f, wsp * 3.4f));
	drawModel(P, V, rock12, ROCK3, TEX_ROCK1);

	glm::mat4 rock13 = glm::translate(rocks, glm::vec3(wsp * 1.0f, wsp * 0.0f, wsp * 1.7f));
	drawModel(P, V, rock13, ROCK3, TEX_ROCK1);

	glm::mat4 rock14 = glm::translate(rocks, glm::vec3(-wsp * 2.0f, wsp * 0.0f, -wsp * 4.0f));
	drawModel(P, V, rock14, ROCK2, TEX_ROCK2);

	glm::mat4 rock15 = glm::translate(rocks, glm::vec3(-wsp * 4.0f, wsp * 0.0f, wsp * 1.0f));
	drawModel(P, V, rock15, ROCK1, TEX_ROCK7);

	glm::mat4 rock16 = glm::translate(rocks, glm::vec3(wsp * 5.1f, wsp * 0.0f, -wsp * 2.0f));
	drawModel(P, V, rock16, ROCK3, TEX_ROCK4);
}

void drawLight(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	Models::Sphere light(0.25, 36, 36);

	spConstant->use();

	glUniformMatrix4fv(spConstant->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(spConstant->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M)); 

	glUniform4f(spConstant->u("color"), 1, 1, 0.8, 1);

	light.drawSolid();
}

void drawTank(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	phongShader->use(); 

	glUniformMatrix4fv(phongShader->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(phongShader->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(phongShader->u("M"), 1, false, glm::value_ptr(M));
	glUniform4fv(phongShader->u("light1"), 1, glm::value_ptr(light1));
	glUniform4fv(phongShader->u("light2"), 1, glm::value_ptr(light2));

	glEnableVertexAttribArray(phongShader->a("vertex"));
	glVertexAttribPointer(phongShader->a("vertex"), 4, GL_FLOAT, false, 0, models[TANK].Vertices.data()); 

	glEnableVertexAttribArray(phongShader->a("texCoord"));
	glVertexAttribPointer(phongShader->a("texCoord"), 2, GL_FLOAT, false, 0, models[TANK].TexCoords.data()); 

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[TANK]);
	glUniform1i(phongShader->u("tex"), 0);

	glDrawElements(GL_TRIANGLES, models[TANK].Indices.size(), GL_UNSIGNED_INT, models[TANK].Indices.data());

	glDisableVertexAttribArray(phongShader->a("vertex"));
	glDisableVertexAttribArray(phongShader->a("normal"));
	glDisableVertexAttribArray(phongShader->a("color"));
}

void drawPlants(glm::mat4 P, glm::mat4 V) {
	float wsp_plant = 1.5f;

	glm::mat4 M = glm::mat4(1.0f);

	glm::mat4 plant1 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant1 = glm::translate(plant1, glm::vec3(0.0f, -7.8f / wsp_plant, 0.0f));
	drawModel(P, V, plant1, PLANTS, TEX_PLANT);

	glm::mat4 plant2 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant2 = glm::translate(plant2, glm::vec3(3.0f / wsp_plant, -7.8f / wsp_plant, -4.0f / wsp_plant));
	drawModel(P, V, plant2, PLANTS, TEX_PLANT);

	glm::mat4 plant3 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant3 = glm::translate(plant3, glm::vec3(5.0f / wsp_plant, -7.8f / wsp_plant, 3.0f / wsp_plant));
	drawModel(P, V, plant3, PLANTS, TEX_PLANT);

	glm::mat4 plant4 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant4 = glm::translate(plant4, glm::vec3(2.0f / wsp_plant, -7.8f / wsp_plant, 5.0f / wsp_plant));
	drawModel(P, V, plant4, PLANTS, TEX_PLANT);

	glm::mat4 plant5 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant5 = glm::translate(plant5, glm::vec3(1.0f / wsp_plant, -7.8f / wsp_plant, 3.0f / wsp_plant));
	drawModel(P, V, plant5, PLANTS, TEX_PLANT);

	glm::mat4 plant6 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant6 = glm::translate(plant6, glm::vec3(-4.0f / wsp_plant, -7.8f / wsp_plant, -3.0f / wsp_plant));
	drawModel(P, V, plant6, PLANTS, TEX_PLANT);

	glm::mat4 plant7 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant7 = glm::translate(plant7, glm::vec3(-3.0f / wsp_plant, -7.8f / wsp_plant, 2.0f / wsp_plant));
	drawModel(P, V, plant7, PLANTS, TEX_PLANT);

	glm::mat4 plant8 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant8 = glm::translate(plant8, glm::vec3(-5.0f / wsp_plant, -7.8f / wsp_plant, -4.0f / wsp_plant));
	drawModel(P, V, plant8, PLANTS, TEX_PLANT);

	glm::mat4 plant9 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant9 = glm::translate(plant9, glm::vec3(3.0f / wsp_plant, -7.8f / wsp_plant, -5.0f / wsp_plant));
	drawModel(P, V, plant9, PLANTS, TEX_PLANT);

	glm::mat4 plant10 = glm::scale(M, glm::vec3(wsp_plant, wsp_plant, wsp_plant));
	plant10 = glm::translate(plant10, glm::vec3(-1.0f / wsp_plant, -7.8f / wsp_plant, -3.0f / wsp_plant));
	drawModel(P, V, plant10, PLANTS, TEX_PLANT);
}

void drawScene(GLFWwindow* window,float angle) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); 
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f); 
	
	glm::mat4 Ml1 = glm::translate(M, glm::vec3(light1[0], light1[1], light1[2]));
	drawLight(P, V, Ml1);
	glm::mat4 Ml2 = glm::translate(M, glm::vec3(light2[0], light2[1], light2[2]));
	drawLight(P, V, Ml2);

	glm::mat4 Mb = glm::translate(Mb, glm::vec3(-5.0f, 0.0f, 0.0f));
	M = glm::rotate(M, -PI/2, glm::vec3(0.0f, 0.0f, 1.0f));
	drawButtom(P, V, M);

	float wsp_nemo = 10.0f;
	float wsp_fish_h = 6.0f;

	glm::mat4 FishMatrix = glm::mat4(1.0f);
	glm::mat4 NemoMatrix = glm::scale(FishMatrix, glm::vec3(wsp_nemo, wsp_nemo, wsp_nemo));
	glm::mat4 FishMatrix_h = glm::scale(FishMatrix, glm::vec3(wsp_fish_h, wsp_fish_h, wsp_fish_h));


	glm::mat4 Mf = glm::rotate(NemoMatrix, angle * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
	Mf = glm::translate(Mf, glm::vec3(4.0f / wsp_nemo, 0.0f, 0.0f));
	drawModel(P, V, Mf, NEMO, TEX_FISH2);

	glm::mat4 Mf1 = glm::rotate(FishMatrix_h, -angle * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
	Mf1 = glm::translate(Mf1, glm::vec3(2.0f / wsp_fish_h, 2.0f / wsp_fish_h, 0.0f));
	drawModel(P, V, Mf1, FISH1, TEX_FISH4);

	glm::mat4 Mf2 = glm::rotate(NemoMatrix, angle * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f));
	Mf2 = glm::translate(Mf2, glm::vec3(1.0f / wsp_nemo, -3.0f / wsp_nemo, 0.0f));
	drawModel(P, V, Mf2, NEMO, TEX_FISH2);

	glm::mat4 Mf3 = glm::rotate(FishMatrix_h, -angle * 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));
	Mf3 = glm::translate(Mf3, glm::vec3(4.0f / wsp_fish_h, 3.0f / wsp_fish_h, 0.0f));
	drawModel(P, V, Mf3, FISH1, TEX_FISH4);

	glm::mat4 Mf4 = glm::rotate(NemoMatrix, angle * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
	Mf4 = glm::translate(Mf4, glm::vec3(6.0f / wsp_nemo, 2.0f / wsp_nemo, 0.0f));
	drawModel(P, V, Mf4, NEMO, TEX_FISH2);

	glm::mat4 Mf5 = glm::rotate(NemoMatrix, angle * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));  // to jest w prawo
	Mf5 = glm::translate(Mf5, glm::vec3(3.0f / wsp_nemo, -2.0f / wsp_nemo, 0.0f));
	drawModel(P, V, Mf5, NEMO, TEX_FISH2);

	glm::mat4 Mf7 = glm::rotate(M, angle * 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));  // to jest w prawo
	Mf7 = glm::translate(Mf7, glm::vec3(3.0f, 0.0f, 2.7f));
	drawModel(P, V, Mf7, FISH, TEX_FISH);

	glm::mat4 Mf8 = glm::rotate(M, -0.2f * angle, glm::vec3(1.0f, 0.0f, 0.0f));  // to jest  w lewo
	Mf8 = glm::translate(Mf8, glm::vec3(1.0f, 0.0f, 2.5f));
	Mf8 = glm::rotate(Mf8, PI, glm::vec3(1.0f, 0.0f, 0.0f));
	drawModel(P, V, Mf8, FISH, TEX_FISH);

	glm::mat4 Mf9 = glm::rotate(M, -angle * 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));  // to jest w prawo
	Mf9 = glm::translate(Mf9, glm::vec3(7.0f, 0.0f, -5.7f));
	drawModel(P, V, Mf9, FISH, TEX_FISH);

	glm::mat4 Mf10 = glm::rotate(M, -angle * 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));  // to jest w prawo
	Mf10 = glm::translate(Mf10, glm::vec3(4.0f, 0.0f, -2.7f));
	drawModel(P, V, Mf10, FISH, TEX_FISH);

	drawRocks(P, V);
	drawPlants(P, V);


	glm::mat4 Mt = glm::rotate(M, PI / 2, glm::vec3(0.0f, 1.0f, 0.0f));
	Mt = glm::scale(Mt, glm::vec3(6.0f, 6.0f, 6.0f));
	drawTank(P, V, Mt);

	glm::mat4 Mg = glm::scale(M, glm::vec3(8.0f, 6.0f, 6.0f));
	drawGlass(P, V, Mg);

	glfwSwapBuffers(window); 

}


int main(void)
{
	GLFWwindow* window; 

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) { 
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1600, 1000, "OpenGL", NULL, NULL); 

	if (!window) 
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); 
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) { 
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	
	float angle = 0; 
	glfwSetTime(0); 
	while (!glfwWindowShouldClose(window)) 
	{
		angle += speed * glfwGetTime(); 
		glfwSetTime(0); 
		drawScene(window,angle);
		glfwPollEvents(); 
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); 
	glfwTerminate(); 
	exit(EXIT_SUCCESS);

}
