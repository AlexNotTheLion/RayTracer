#include <SDL/SDL.h>
#include <GLEW/glew.h>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#include <windows.h>

static int win(0);
std::vector<std::string > textures;

bool InitGL()//initialize glew
{
	glewExperimental = GL_TRUE;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "Error: GLEW failed to initialise with message: " << glewGetErrorString(err) << std::endl;
		return false;
	}
	std::cout << "INFO: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

	std::cout << "INFO: OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "INFO: OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "INFO: OpenGL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	return true;
}



GLuint CreateTriangleVAO()//tell opengl what verties to draw
{
	GLuint VAO = 0;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	float vertices[] = {
		 -1.0f, -1.0f,
		  1.0f, -1.0f,
		 -1.0f,  1.0f,

		  1.0f, -1.0f,
		  1.0f, 1.0f,
		 -1.0f,  1.0f
	};
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDisableVertexAttribArray(0);

	return VAO;
}

void DrawVAOTris(GLuint VAO, int numVertices, GLuint shaderProgram)//tell opengl to actually draw it to the screen
{
	glUseProgram(shaderProgram);

	glBindVertexArray(VAO);

	glDrawArrays(GL_TRIANGLES, 0, numVertices);

	glBindVertexArray(0);

	glUseProgram(0);
}

bool CheckShaderCompiled(GLint shader) //check the shader compiled correctly
{
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLsizei len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = new GLchar[len + 1];
		glGetShaderInfoLog(shader, len, &len, log);
		std::cerr << "ERROR: Shader compilation failed: " << log << std::endl;
		delete[] log;

		return false;
	}
	return true;
}

std::string readFile(const char* filePath)//function to load shaders
{
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);

	if (!fileStream.is_open())
	{
		std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}

	std::string line = "";
	while (!fileStream.eof())
	{
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}


GLuint LoadShaders()//load each shader
{
	//load in shaders from file
	std::string vertShaderStr = readFile("../Shaders/vShader.glsl");
	std::string fragShaderStr = readFile("../Shaders/fShader.glsl");

	const GLchar* vShaderText = vertShaderStr.c_str();
	const GLchar* fShaderText = fragShaderStr.c_str();


	GLuint program = glCreateProgram();

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vShader, 1, &vShaderText, NULL);

	glCompileShader(vShader);

	if (!CheckShaderCompiled(vShader))
	{
		return 0;
	}

	glAttachShader(program, vShader);

	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShader, 1, &fShaderText, NULL);


	glCompileShader(fShader);
	if (!CheckShaderCompiled(fShader))
	{
		return 0;
	}
	glAttachShader(program, fShader);

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = new GLchar[len + 1];
		glGetProgramInfoLog(program, len, &len, log);
		std::cerr << "ERROR: Shader linking failed: " << log << std::endl;
		delete[] log;

		return 0;
	}

	return program;
}


//program global variables
std::vector<SDL_Keycode> Keys;
glm::mat4 viewMatrix;
glm::vec3 light_pos = glm::vec3(0);
glm::vec3 cameraPos;
glm::vec3 cameraRot;
float brightness = 10.0f;
glm::mat4 projectionMatrix;
GLuint shaderProgram;
float t;
float speed = 3;
float deltaTime;
float ticks;
float oldticks;
std::vector<int> fpsList;
int fileNum;
int sum;

int obs = 15;
int reflect = 5;
std::string added = "";

void writeFile()//stores fps to file
{
	if (fpsList.size() == 0)
	{
		return;
	}

	sum = 0;
	std::ifstream checkFile;
	std::string name = "fps";
	std::string newName = name;

	std::string folderName = std::to_string(obs) + " obs " + std::to_string(reflect) + " reflect" + added;

	CreateDirectory(("../test/" + folderName + added).c_str(), NULL);

	checkFile.open("../test/" + folderName + "/" + name + ".txt");
	while (checkFile)//if file exists add a incrementing extension
	{
		checkFile.close();
		newName = name + "(" + std::to_string(fileNum) + ")";
		checkFile.open("../test/" + folderName + "/" + newName + ".txt");
		fileNum++;
	}

	std::ofstream fpsFile("../test/" + folderName + "/" + newName + ".txt");

	fpsFile << "build mode : release" << "\n";
	fpsFile << "number of objects : " << std::to_string(obs) << "\n";
	fpsFile << "number of reflections : " << std::to_string(reflect) << "\n";

	for (auto it : fpsList)//write fps to file
	{
		fpsFile << it << "\n";
		sum += it;
	}

	sum = sum / fpsList.size();
	fpsFile << "average fps : " << sum << "\n" << "end test\n";
	fpsFile.close();

}


//clamp function from c++17
template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
	assert(!(hi < lo));
	return(v < lo) ? lo : (hi < v) ? hi : v;
}

void moveMouse(glm::vec2 pos) //manages mouse movement
{
	cameraRot += glm::vec3(0.0, -pos.x, 0.0);
	cameraRot += glm::vec3(-pos.y, 0.0, 0.0);

	cameraRot.x = clamp(cameraRot.x, -45.0f, 45.0f);
}

bool keyDown(SDL_Keycode key)//function to check if passed key code is pressed
{
	for (auto it : Keys)
	{
		if (it == key)
		{
			return true;
		}
	}
	return false;
}

void inputHandeler()//manages keyboard inputs
{
	glUseProgram(shaderProgram);

	glm::vec3 amount = glm::vec3(0);

	if (keyDown(SDLK_w)) // move forward
	{
		amount += glm::vec3(0.0, 0.0, -speed);
	}
	if (keyDown(SDLK_a))//move left
	{
		amount += glm::vec3(-speed, 0.0, 0.0);
	}
	if (keyDown(SDLK_s))//move backwards
	{
		amount += glm::vec3(0.0, 0.0, speed);
	}
	if (keyDown(SDLK_d))//move right
	{
		amount += glm::vec3(speed, 0.0, 0.0);
	}
	if (keyDown(SDLK_q)) // roll clockwise
	{
		cameraRot += glm::vec3(0.0, 0.0, 100.0) * deltaTime;
	}
	if (keyDown(SDLK_e)) // roll anticlockwise
	{
		cameraRot += glm::vec3(0.0, 0.0, -100.0) * deltaTime;
	}
	if (keyDown(SDLK_r))//resets to default view
	{
		cameraPos = glm::vec3(0);
		cameraRot = glm::vec3(0);
	}


	//moves in direction facing 
	glm::vec3 direction = glm::vec3(sin(glm::radians(cameraRot.y)), 0, cos(glm::radians(cameraRot.y)));
	glm::vec3 up = glm::vec3(0, 1, 0);
	direction = glm::normalize(direction);

	cameraPos += direction * amount.z * deltaTime;
	cameraPos += glm::normalize(glm::cross(up, direction)) * amount.x * deltaTime;
	//


	//create view matrix
	viewMatrix = glm::translate(glm::mat4(1.0f), cameraPos);
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRot.y), glm::vec3(0, 1, 0));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRot.x), glm::vec3(1, 0, 0));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRot.z), glm::vec3(0, 0, 1));


	//moving a sphere
	float x = sin(t) * 2;
	float z = cos(t) * 2;
	t += deltaTime;

	light_pos = glm::vec3(x, 0.0, z);


	//pass inverse view matrix to shader
	GLint view_location = glGetUniformLocation(shaderProgram, "inverseViewMatrix");
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(viewMatrix));


	//pass light pos to shader
	GLint light_location = glGetUniformLocation(shaderProgram, "light_pos");
	glUniform3f(light_location, light_pos.x, light_pos.y, light_pos.z);

	if (brightness < 0) brightness = 0;

	//pass light brightness to shader
	GLint lightBrightness_location = glGetUniformLocation(shaderProgram, "light_brightness");
	glUniform1f(lightBrightness_location, brightness);
}

void loadCubeMap(std::string name)//load environment map / skybox
{
	glUseProgram(shaderProgram);

	std::vector<std::string> faces
	{
			"right.jpg",
			"left.jpg",
			"top.jpg",
			"bottom.jpg",
			"front.jpg",
			"back.jpg"
	};

	unsigned int texture;
	int size = 4 * textures.size();
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0 + size);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	int width, height, nrChannels;
	std::string iPath = "../Materials/" + name + "/";
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		std::string load = iPath + faces[i];
		unsigned char* data = stbi_load(load.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
			std::cout << "loaded cubemap successfully\n";
		}
		else
		{
			std::cout << "failed to load skybox texture : " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint cube = glGetUniformLocation(shaderProgram, "cubemap");

	glUniform1i(cube, size);
}

void loadTexture() //loads textures
{
	const int size = 10;
	glUseProgram(shaderProgram);
	GLint albedoSamples[size];
	//load albedo textures
	for (int i = 0; i < textures.size(); i++)
	{
		std::cout << "loading " + textures[i] + " albedo\n";
		albedoSamples[i] = i;
		unsigned int texture;
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width, height, nrChannels;
		std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_albedo.png";
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

		if (!data)
		{
			std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_albedo.jpg";
			data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		}

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "failed to load albedo texture\n";
		}
		stbi_image_free(data);

	}

	//pass to shader
	GLuint albedo = glGetUniformLocation(shaderProgram, "u_albedo");

	glUniform1iv(albedo, textures.size(), albedoSamples);



	//load roughness textures
	GLint roughnessSamples[size];
	for (int i = 0; i < textures.size(); i++)
	{
		std::cout << "loading " + textures[i] + " roughness\n";
		roughnessSamples[i] = i + textures.size();
		unsigned int texture;
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0 + i + textures.size());
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width, height, nrChannels;
		std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_roughness.png";
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

		if (!data)
		{
			std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_roughness.jpg";
			data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		}

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "failed to load roughness texture\n";
		}
		stbi_image_free(data);
	}

	//pass to shader
	GLuint roughness = glGetUniformLocation(shaderProgram, "u_roughness");

	glUniform1iv(roughness, textures.size(), roughnessSamples);


	//load metalic textures
	GLint metalicSamples[size];
	for (int i = 0; i < textures.size(); i++)
	{
		std::cout << "loading " + textures[i] + " metalic\n";
		metalicSamples[i] = i + (2 * textures.size());
		unsigned int texture;
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0 + i + (2 * textures.size()));
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width, height, nrChannels;
		std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_metalic.png";
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

		if (!data)
		{
			std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_metalic.jpg";
			data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		}

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "failed to load metalic texture\n";
		}
		stbi_image_free(data);

	}

	//pass to shader
	GLuint metalic = glGetUniformLocation(shaderProgram, "u_metalic");

	glUniform1iv(metalic, textures.size(), metalicSamples);

	//load metalic textures
	GLint normalSamples[size];
	for (int i = 0; i < textures.size(); i++)
	{
		std::cout << "loading " + textures[i] + " normal\n";
		normalSamples[i] = i + (3 * textures.size());
		unsigned int texture;
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0 + i + (3 * textures.size()));
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width, height, nrChannels;
		std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_normal.png";
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

		if (!data)
		{
			std::string path = "../Materials/textures/" + textures[i] + "/" + textures[i] + "_normal.jpg";
			data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		}

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "failed to load normal texture\n";
		}
		stbi_image_free(data);

	}

	//pass to shader
	GLuint normal = glGetUniformLocation(shaderProgram, "u_normal");

	glUniform1iv(normal, textures.size(), normalSamples);

	return;
}


int main(int argc, char* args[])
{
	bool testing = false;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "Whoops! Something went very wrong, cannot initialise SDL :(" << std::endl;
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);//setting open gl version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	//window attributes
	int windowPosX = 100;
	int windowPosY = 100;
	int windowWidth = 720;
	int windowHeight = 480;


	//using sdl to prep a window then launching opengl within it
	SDL_Window* window = SDL_CreateWindow("OpenGL", windowPosX, windowPosY, windowWidth, windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	if (!testing)
		SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_GL_SetSwapInterval(0);

	if (!InitGL())
	{
		return -1;
	}

	GLuint triangleVAO = CreateTriangleVAO();

	shaderProgram = LoadShaders();

	glUseProgram(shaderProgram);

	//create matrixes
	viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)));//build view matrix
	projectionMatrix = glm::inverse(glm::perspective(glm::radians(30.0f), (float)windowWidth / (float)windowHeight, 0.001f, 100.0f));//build projection matrix

	cameraPos = glm::vec3(0);
	cameraRot = glm::vec3(0);

	//pass them into shader
	GLint view_location = glGetUniformLocation(shaderProgram, "inverseViewMatrix");
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	GLint projection_location = glGetUniformLocation(shaderProgram, "inverseProjectionMatrix");
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	//add textures to texture list
	textures.push_back("titanium");

	//load the textures
	loadTexture();
	//load the environment map / skybox
	loadCubeMap("skybox/jpg");

	int frames = 0;

	SDL_RaiseWindow(window);

	bool contains;
	bool go = true;
	bool firstRun = true;

	float lastTime = SDL_GetTicks();
	while (go)
	{

		//calculate delta time
		ticks = SDL_GetTicks();
		deltaTime = (ticks - oldticks) / 1000.0f;
		oldticks = ticks;

		frames++;
		if (ticks >= lastTime + 1000)//frame rate counter
		{
			printf("Fps: %d \n", frames);
			if (!firstRun)
			{
				fpsList.push_back(frames);
			}
			else
			{
				firstRun = false;
			}
			lastTime = ticks;
			frames = 0;
		}

		if (fpsList.size() >= 5 & testing)
		{
			go = false;
		}

		SDL_Event event;
		while (SDL_PollEvent(&event))//manages sdl events, such as key press' or just general sdl stuff like sdl quit
		{
			switch (event.type)
			{
			case SDL_QUIT:
				go = false;
				break;
			case SDL_KEYDOWN:
				contains = false;
				for (auto it : Keys)//add pressed key to a list if it hasnt been added already
				{
					if (it == event.key.keysym.sym)
					{
						contains = true;
						break;
					}
				}
				if (!contains) Keys.push_back(event.key.keysym.sym);
				break;
			case SDL_KEYUP:
				for (auto it = Keys.begin(); it != Keys.end();)//remove all instances of key from a list
				{
					if (*it == event.key.keysym.sym)
					{
						it = Keys.erase(it);
					}
					else
					{
						it++;
					}
				}
				break;
			case SDL_MOUSEMOTION://store mouse movement relative to the window
				moveMouse(glm::vec2(event.motion.xrel, event.motion.yrel));
				break;
			}
			if (keyDown(SDLK_ESCAPE))//close on escape
			{
				go = false;
			}
		}

		inputHandeler();//write fps to file

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		DrawVAOTris(triangleVAO, 6, shaderProgram);
		SDL_GL_SwapWindow(window);
	}
	if (testing)
		writeFile();

	//clean up and close opengl / sdl
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	//store average fps

	return 0;
}