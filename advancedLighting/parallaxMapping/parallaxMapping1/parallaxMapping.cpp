#define GLEW_STATIC
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "texture.h"
#include "model.h"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void do_movement();

const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 600;

GLfloat lastX = WINDOW_WIDTH / 2.0f, lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouseMove = true;
bool keyPressedStatus[1024];
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
Camera camera(glm::vec3(0.0f, 1.8f, 4.0f));
glm::vec3 lampPos(-3.0f, 5.0f, -1.2f);
glm::vec3 lampPos2(3.0, 1.0f, -1.2f);
bool bNormalMapping = true;
bool bBlinnPhong = true;
bool bParallaxMapping = true;
GLfloat heightScale = 0.1f;

Model cube;
Model cyborgModel;
Model planet;
Model moon;
Model path;
Model secondPath;

float x = 2;
bool down = true;

GLuint quadVAOId, quadVBOId;

void setupQuadVAO();
int loadModels();

int main(int argc, char** argv)
{

	if (!glfwInit())
	{
		std::cout << "Error::GLFW could not initialize GLFW!" << std::endl;
		return -1;
	}

	std::cout << "Start OpenGL core profile version 3.3" << std::endl;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
		"Advanced Shader Programming - N toggle Normal Mapping - P toggle Parallax Mapping - F toggle Blinn-Phong", NULL, NULL);

	if (!window)
	{
		std::cout << "Error::GLFW could not create winddow!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	glfwSetCursorPosCallback(window, mouse_move_callback);

	glfwSetScrollCallback(window, mouse_scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	glewExperimental = GL_TRUE;
	GLenum status = glewInit();
	if (status != GLEW_OK)
	{
		std::cout << "Error::GLEW glew version:" << glewGetString(GLEW_VERSION)
			<< " error string:" << glewGetErrorString(status) << std::endl;
		glfwTerminate();
		return -1;
	}


	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Section 1 - Set up VAO and load models
	setupQuadVAO();
	loadModels();

	// Section 2 - Load all textures for parallax effect
	GLuint brick_D = TextureHelper::load2DTexture("../../resources/textures/CartoonBrick/bricks2.jpg");
	GLuint brick_N = TextureHelper::load2DTexture("../../resources/textures/CartoonBrick/bricks2_normal.jpg");
	GLuint brick_H = TextureHelper::load2DTexture("../../resources/textures/CartoonBrick/bricks2_disp.jpg");

	GLuint detailedBrick_D = TextureHelper::load2DTexture("../../resources/textures/RealisticBrick/brickwall.jpg");
	GLuint detailedBrick_N = TextureHelper::load2DTexture("../../resources/textures/RealisticBrick/brickwall_normal.jpg");
	GLuint detailedBrick_H = TextureHelper::load2DTexture("../../resources/textures/RealisticBrick/brickwall_disp.jpg");

	GLuint stones_D = TextureHelper::load2DTexture("../../resources/textures/Stones/Stones.jpg");
	GLuint stones_N = TextureHelper::load2DTexture("../../resources/textures/Stones/Stones_normal.jpg");
	GLuint stones_H = TextureHelper::load2DTexture("../../resources/textures/Stones/Stones_disp.jpg");

	GLuint tiles_D = TextureHelper::load2DTexture("../../resources/textures/Tiles/Tiles.jpg");
	GLuint tiles_N = TextureHelper::load2DTexture("../../resources/textures/Tiles/Tiles_normal.jpg");
	GLuint tiles_H = TextureHelper::load2DTexture("../../resources/textures/Tiles/Tiles_disp.jpg");

	// Section 3 - Load shaders
	Shader white("Shaders/white.vertex", "Shaders/white.frag");
	Shader parallaxShader("Shaders/parallax.vertex", "Shaders/parallax.frag");
	Shader normalShader("Shaders/normals.vertex", "Shaders/normals.frag");

	glEnable(GL_DEPTH_TEST);
	
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents();
		do_movement();

		glm::mat4 projection = glm::perspective(camera.mouse_zoom,
			(GLfloat)(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.0f); // near and far camera planes
		glm::mat4 view = camera.getViewMatrix();

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Set up the parallax shader with lighting parameters
		parallaxShader.use();
		GLint lightAmbientLoc = glGetUniformLocation(parallaxShader.programId, "light.ambient");
		GLint lightDiffuseLoc = glGetUniformLocation(parallaxShader.programId, "light.diffuse");
		GLint lightSpecularLoc = glGetUniformLocation(parallaxShader.programId, "light.specular");
		GLint lightPosLoc = glGetUniformLocation(parallaxShader.programId, "light.position");
		glUniform3f(lightAmbientLoc, 0.0f, 0.0f, 0.0f); //ambient
		glUniform3f(lightDiffuseLoc, 0.7f, 0.7f, 0.7f); //light diffusion
		glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f); //reflective-ness
		GLint viewPosLoc = glGetUniformLocation(parallaxShader.programId, "viewPos");
		glUniform3f(viewPosLoc, camera.position.x, camera.position.y, camera.position.z);
		lightPosLoc = glGetUniformLocation(parallaxShader.programId, "lightPos");
		glUniform3f(lightPosLoc, lampPos.x, lampPos.y, lampPos.z);

		lightAmbientLoc = glGetUniformLocation(parallaxShader.programId, "secondLight.ambient");
		lightDiffuseLoc = glGetUniformLocation(parallaxShader.programId, "secondLight.diffuse");
		lightSpecularLoc = glGetUniformLocation(parallaxShader.programId, "secondLight.specular");
		lightPosLoc = glGetUniformLocation(parallaxShader.programId, "secondLight.position");
		glUniform3f(lightAmbientLoc, 0.0f, 0.0f, 0.0f); //ambient
		glUniform3f(lightDiffuseLoc, 0.7f, 0.7f, 0.7f); //light diffusion
		glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f); //reflective-ness
		lightPosLoc = glGetUniformLocation(parallaxShader.programId, "secondLightPos");
		glUniform3f(lightPosLoc, lampPos2.x, lampPos2.y, lampPos2.z);

		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "projection"),
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "view"),
			1, GL_FALSE, glm::value_ptr(view));
		glm::mat4 model;

		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(glGetUniformLocation(parallaxShader.programId, "heightScale"), heightScale);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "bParallaxMapping"), bParallaxMapping);

		//Textures for brick wall
		glBindVertexArray(quadVAOId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brick_D);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "diffuseMap"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, brick_N);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "normalMap"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, brick_H);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "heightMap"), 2);
		//1
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 0, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//2
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(2, 0, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//3
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-2, 0, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//4
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(4, 0, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//5
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-4, 0, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Textures for detailed brick wall
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, detailedBrick_D);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "diffuseMap"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, detailedBrick_N);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "normalMap"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, detailedBrick_H);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "heightMap"), 2);
		//1
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 2, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//2
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(2, 2, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//3
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-2, 2, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//4
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(4, 2, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//5
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-4, 2, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Textures for stones / pebbles
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, stones_D);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "diffuseMap"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, stones_N);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "normalMap"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, stones_H);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "heightMap"), 2);
		//1
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 4, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//2
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(2, 4, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//3
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-2, 4, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//4
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(4, 4, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//5
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-4, 4, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Textures for tiles
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tiles_D);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "diffuseMap"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tiles_N);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "normalMap"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tiles_H);
		glUniform1i(glGetUniformLocation(parallaxShader.programId, "heightMap"), 2);
		//1
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 6, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//2
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(2, 6, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//3
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-2, 6, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//4
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(4, 6, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//5
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-4, 6, -2));
		glUniformMatrix4fv(glGetUniformLocation(parallaxShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Set up normal mapping shader with lighting parameters
		normalShader.use();
		lightAmbientLoc = glGetUniformLocation(normalShader.programId, "light.ambient");
		lightDiffuseLoc = glGetUniformLocation(normalShader.programId, "light.diffuse");
		lightSpecularLoc = glGetUniformLocation(normalShader.programId, "light.specular");
		lightPosLoc = glGetUniformLocation(normalShader.programId, "light.position");
		glUniform3f(lightAmbientLoc, 0.3f, 0.3f, 0.3f);
		glUniform3f(lightDiffuseLoc, 0.6f, 0.6f, 0.6f);
		glUniform3f(lightSpecularLoc, 0.5f, 0.5f, 0.5f);
		viewPosLoc = glGetUniformLocation(normalShader.programId, "viewPos");
		glUniform3f(viewPosLoc, camera.position.x, camera.position.y, camera.position.z);
		lightPosLoc = glGetUniformLocation(normalShader.programId, "lightPos");
		glUniform3f(lightPosLoc, lampPos.x, lampPos.y, lampPos.z);
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "projection"),
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "view"),
			1, GL_FALSE, glm::value_ptr(view));

		model = glm::mat4();

		//animation for cyborg model
		if (down)
		{
			x -= 0.01;
		}
		
		if (x < 0 || !down)
		{
			down = false;
			x += 0.01;

			if (x > 3)
				down = true;
		}

		//Cyborg
		model = glm::translate(model, glm::vec3(0, x, 0));
		model = glm::scale(model, glm::vec3(0.5f));
		model = glm::rotate(model, (GLfloat)glfwGetTime() * -1.0f, glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		glUniform1i(glGetUniformLocation(normalShader.programId, "bNormalMapping"), bNormalMapping);
		glUniform1i(glGetUniformLocation(normalShader.programId, "bBlinnPhong"), bBlinnPhong);
		cyborgModel.draw(normalShader);
		//Planet
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-10, 3, 3));
		model = glm::rotate(model, (GLfloat)glfwGetTime() * -0.2f, glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		planet.draw(normalShader);
		//Moon
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-10, 3, 3));
		model = glm::rotate(model, (GLfloat)glfwGetTime() * -1, glm::normalize(glm::vec3(0.2, 1.0, 0.0)));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		moon.draw(normalShader);
		//First path
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, -1.2, -0.7));
		model = glm::scale(model, glm::vec3(0.25f));
		model = glm::rotate(model, 1.56f, glm::vec3(0, 1.0, 0));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		path.draw(normalShader);
		//Second path
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, -1.2, 2.3));
		model = glm::scale(model, glm::vec3(0.25f));
		model = glm::rotate(model, 1.56f, glm::vec3(0, 1.0, 0));
		glUniformMatrix4fv(glGetUniformLocation(normalShader.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		secondPath.draw(normalShader);

		//Set up 'white shader'
		white.use();
		glUniformMatrix4fv(glGetUniformLocation(white.programId, "projection"),
			1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(white.programId, "view"),
			1, GL_FALSE, glm::value_ptr(view));
		//Draw cube at light positiom
		model = glm::mat4();
		model = glm::translate(model, lampPos);
		model = glm::scale(model, glm::vec3(0.1f));
		glUniformMatrix4fv(glGetUniformLocation(white.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		cube.draw(white);
		//Draw second cube
		model = glm::mat4();
		model = glm::translate(model, lampPos2);
		model = glm::scale(model, glm::vec3(0.1f));
		glUniformMatrix4fv(glGetUniformLocation(white.programId, "model"),
			1, GL_FALSE, glm::value_ptr(model));
		cube.draw(white);

		std::cout << "phong lighting: " << bBlinnPhong << std::endl;
		std::cout << "normal mapping: " << bNormalMapping << std::endl;
		std::cout << "parallax mapping: " << bParallaxMapping << std::endl;

		glBindVertexArray(0);
		glUseProgram(0);
		glfwSwapBuffers(window);
	}
	
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keyPressedStatus[key] = true;
		else if (action == GLFW_RELEASE)
			keyPressedStatus[key] = false;
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{	//N to toggle normal mapping
		bNormalMapping = !bNormalMapping;
	}
	else if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{	//F to toggle blinn phong lighting on normal shader
		bBlinnPhong = !bBlinnPhong;
	}
	else if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{	//P to toggle parallax mapping
		bParallaxMapping = !bParallaxMapping;
	}
	else if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{	//Manually change height scale for parallax
		heightScale += 0.1f;
		std::cout << "Height scale : " << heightScale << std::endl;
	}
	else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		heightScale -= 0.1f;
		std::cout << "Height scale : " << heightScale << std::endl;
	}
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.handleMouseMove(xoffset, yoffset);
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.handleMouseScroll(yoffset);
}

void do_movement()
{
	if (keyPressedStatus[GLFW_KEY_W])
		camera.handleKeyPress(FORWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_S])
		camera.handleKeyPress(BACKWARD, deltaTime);
	if (keyPressedStatus[GLFW_KEY_A])
		camera.handleKeyPress(LEFT, deltaTime);
	if (keyPressedStatus[GLFW_KEY_D])
		camera.handleKeyPress(RIGHT, deltaTime);
}

void setupQuadVAO()
{

	glm::vec3 pos1(-1.0, 1.0, 0.0);
	glm::vec3 pos2(-1.0, -1.0, 0.0);
	glm::vec3 pos3(1.0, -1.0, 0.0);
	glm::vec3 pos4(1.0, 1.0, 0.0);

	glm::vec2 uv1(0.0, 1.0);
	glm::vec2 uv2(0.0, 0.0);
	glm::vec2 uv3(1.0, 0.0);
	glm::vec2 uv4(1.0, 1.0);

	glm::vec3 nm(0.0, 0.0, 1.0);


	glm::vec3 tangent1, bitangent1;
	glm::vec3 tangent2, bitangent2;

	glm::vec3 edge1 = pos2 - pos1;
	glm::vec3 edge2 = pos3 - pos1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	GLfloat f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);


	edge1 = pos3 - pos1;
	edge2 = pos4 - pos1;
	deltaUV1 = uv3 - uv1;
	deltaUV2 = uv4 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);


	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);


	GLfloat quadVertices[] = {
		pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, 
		tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
		pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, 
		tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
		pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, 
		tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

		pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, 
		tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
		pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, 
		tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
		pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, 
		tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
	};
	glGenVertexArrays(1, &quadVAOId);
	glGenBuffers(1, &quadVBOId);
	glBindVertexArray(quadVAOId);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBOId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
		14 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
		14 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 
		14 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 
		14 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE,
		14 * sizeof(GLfloat), (GLvoid*)(11 * sizeof(GLfloat)));
	glBindVertexArray(0);
}

int loadModels()
{
	//reads a text file containing location of models
	std::ifstream modelPath("modelPath.txt");

	if (!modelPath)
	{
		//error if text file can't be found
		std::cerr << "Error::could not read model path file." << std::endl;
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	//use getline to read each line of the file, for the model location
	std::string modelFilePath;
	std::getline(modelPath, modelFilePath);

	//load the model
	if (!cyborgModel.loadModel(modelFilePath))
	{
		//if model can't be found, error
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	std::getline(modelPath, modelFilePath);

	if (!planet.loadModel(modelFilePath))
	{
		//if model can't be found, error
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	std::getline(modelPath, modelFilePath);

	if (!moon.loadModel(modelFilePath))
	{
		//if model can't be found, error
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	std::getline(modelPath, modelFilePath);

	if (!path.loadModel(modelFilePath) || !secondPath.loadModel(modelFilePath))
	{
		//if model can't be found, error
		glfwTerminate();
		std::system("pause");
		return -1;
	}

	std::getline(modelPath, modelFilePath);

	if (!cube.loadModel(modelFilePath))
	{
		//if model can't be found, error
		glfwTerminate();
		std::system("pause");
		return -1;
	}
}

