// Include C++ headers
#include <iostream>
#include <string>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/model.h>
#include <common/texture.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Texture Mapping"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram;
GLuint MVPLocation;
GLuint textureSampler;
GLuint texture;
GLuint suzanneVAO;
GLuint suzanneVerticiesVBO, suzanneUVVBO;
std::vector<vec3> suzanneVertices, suzanneNormals;
std::vector<vec2> suzanneUVs;

GLuint movingtexture;
GLuint timeUniform;
GLuint movingTextureSampler;

GLuint movingtexture2;
GLuint movingTextureSampler2;

void createContext() {

    shaderProgram = loadShaders("texture.vertexshader", "texture.fragmentshader");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Get a pointer location to MVP matrix in the vertex shader
    MVPLocation = glGetUniformLocation(shaderProgram, "MVP");

    // Load the Suzanne model
    loadOBJ("suzanne.obj", suzanneVertices, suzanneUVs, suzanneNormals);

    // VAO
    glGenVertexArrays(1, &suzanneVAO);
    glBindVertexArray(suzanneVAO);

    // vertex VBO
    glGenBuffers(1, &suzanneVerticiesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, suzanneVerticiesVBO);
    glBufferData(GL_ARRAY_BUFFER, suzanneVertices.size() * sizeof(glm::vec3),
                 &suzanneVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    // Get a handle and load the standard texture
    textureSampler = glGetUniformLocation(shaderProgram, "textureSampler");
    texture = loadBMP("uvtemplate.bmp");

    // Get a handle and load the moving textures
    timeUniform = glGetUniformLocation(shaderProgram, "time");
    movingTextureSampler = glGetUniformLocation(shaderProgram, "movingTextureSampler");
	movingtexture = loadBMP("water.bmp");
    movingTextureSampler2 = glGetUniformLocation(shaderProgram, "movingTextureSampler2");
    movingtexture2 = loadBMP("water2.bmp");


    // uvs VBO
    glGenBuffers(1, &suzanneUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, suzanneUVVBO);
    glBufferData(GL_ARRAY_BUFFER, suzanneUVs.size() * sizeof(glm::vec2),
        &suzanneUVs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
}
void free() {
    glDeleteBuffers(1, &suzanneVerticiesVBO);
    glDeleteBuffers(1, &suzanneUVVBO);
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &suzanneVAO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void mainLoop() {
    do {
        mat4 MVP, modelMatrix, viewMatrix, projectionMatrix;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Bind Suzanne
        glBindVertexArray(suzanneVAO);
        
        // Update the camera and the MVP arrays
        camera->update();
        projectionMatrix = camera->projectionMatrix;
        viewMatrix = camera->viewMatrix;
        modelMatrix = glm::mat4(1.0);
        
        MVP = projectionMatrix * viewMatrix * modelMatrix;
        
        // Send the MVP array to the shaders
        glUniformMatrix4fv(MVPLocation, 1, GL_FALSE, &MVP[0][0]);

        // Activate the textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(textureSampler, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, movingtexture);
		glUniform1i(movingTextureSampler, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, movingtexture2);
        glUniform1i(movingTextureSampler2, 2);

        // Pass the time to the shader
        glUniform1f(timeUniform, (float)glfwGetTime() / 20.0);

        // Draw, disabling depth test because the
        // object is transparent.
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, suzanneVertices.size());
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);
}

void initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        throw runtime_error("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        throw runtime_error(string(string("Failed to open GLFW window.") +
                            " If you have an Intel GPU, they are not 3.3 compatible." +
                            "Try the 2.1 version.\n"));
    }
    glfwMakeContextCurrent(window);

    // Start GLEW extension handler
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw runtime_error("Failed to initialize GLEW\n");
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

    // Gray background color
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Cull triangles whose normal is not towards the camera
    // glEnable(GL_CULL_FACE);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        camera->onMouseMove(xpos, ypos);
        }
    );
}

int main(void) {
    try {
        initialize();
        createContext();
        mainLoop();
        free();
    } catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }
    return 0;
}