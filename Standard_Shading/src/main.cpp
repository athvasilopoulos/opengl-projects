// Include C++ headers
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

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
void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);


#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Standard Shading"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint lightLocation;
GLuint diffuceColorSampler, specularColorSampler;
GLuint diffuseTexture, specularTexture;
GLuint objVAO, triangleVAO;
GLuint objVerticiesVBO, objUVVBO, objNormalsVBO;
GLuint triangleVerticesVBO, triangleNormalsVBO;
std::vector<vec3> objVertices, objNormals;
std::vector<vec2> objUVs;

#define RENDER_TRIANGLE 0

// Material struct
struct Material{
    vec3 Ks;
    vec3 Kd;
    vec3 Ka;
    float Ns;
};

// Materials array
struct Material mats[4];

// Initial light values
vec3 lposition(0.0f, 0.0f, 4.0f);
float light_red = 1.0f;
float light_green = 1.0f;
float light_blue = 1.0f;
float light_power = 10.0f;

void createContext()
{
    shaderProgram = loadShaders(
        "StandardShading.vertexshader",
        "StandardShading.fragmentshader");
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Load Suzanne
    loadOBJWithTiny("suzanne.obj", objVertices, objUVs, objNormals);
    
    /* Flat shading implementation if needed
    for (int i = 0; i < objVertices.size(); i += 3) {
        objNormals[i] = cross(normalize(objVertices[i + 1] - objVertices[i]), normalize(objVertices[i + 2] - objVertices[i]));
        objNormals[i + 1] = objNormals[i];
        objNormals[i + 2] = objNormals[i];
    }
    //*/

    // Load diffuse and specular texture maps
    diffuseTexture = loadSOIL("suzanne_diffuse.bmp");
    specularTexture = loadSOIL("suzanne_specular.bmp");

    // get pointers to the uniform variables
    diffuceColorSampler = glGetUniformLocation(shaderProgram, "diffuceColorSampler");
    specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");

    projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    lightLocation = glGetUniformLocation(shaderProgram, "light_position_cameraspace");
    
    // Bind obj buffers
    glGenVertexArrays(1, &objVAO);
    glBindVertexArray(objVAO);

    // vertex VBO
    glGenBuffers(1, &objVerticiesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, objVerticiesVBO);
    glBufferData(GL_ARRAY_BUFFER, objVertices.size() * sizeof(glm::vec3),
        &objVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    // normal VBO
    glGenBuffers(1, &objNormalsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, objNormalsVBO);
    glBufferData(GL_ARRAY_BUFFER, objNormals.size() * sizeof(glm::vec3),
        &objNormals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);
    

    // uvs VBO
    glGenBuffers(1, &objUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, objUVVBO);
    glBufferData(GL_ARRAY_BUFFER, objUVs.size() * sizeof(glm::vec2),
        &objUVs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(2);
    //*/
}

void free()
{
    glDeleteBuffers(1, &triangleVerticesVBO);
    glDeleteBuffers(1, &triangleNormalsVBO);
    glDeleteVertexArrays(1, &triangleVAO);

    glDeleteBuffers(1, &objVerticiesVBO);
    glDeleteBuffers(1, &objUVVBO);
    glDeleteBuffers(1, &objNormalsVBO);
    glDeleteVertexArrays(1, &objVAO);

    glDeleteTextures(1, &diffuseTexture);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void mainLoop()
{
    
    //*/ Meterial declaration
    struct Material temp;

    // 1. Bronze  
    temp.Ka = vec3(0.2125, 0.1275, 0.054);
    temp.Kd = vec3(0.714, 0.4284, 0.18144);
    temp.Ks = vec3(0.393548, 0.271906, 0.166721);
    temp.Ns = 25.6;
    mats[0] = temp;

    // 2. Jade  
    temp.Ka = vec3(0.135, 0.2225, 0.1575);
    temp.Kd = vec3(0.54, 0.89, 0.63);
    temp.Ks = vec3(0.316228, 0.316228, 0.316228);
    temp.Ns = 12.8;
    mats[1] = temp;
    
    // 3. Silver 
    temp.Ka = vec3(0.19225, 0.19225, 0.19225);
    temp.Kd = vec3(0.50754, 0.50754, 0.50754);
    temp.Ks = vec3(0.508273, 0.508273, 0.508273);
    temp.Ns = 51.2;
    mats[2] = temp;

    // 4. Pewter 
    temp.Ka = vec3(0.105882, 0.058824, 0.113725);
    temp.Kd = vec3(0.427451, 0.470588, 0.541176);
    temp.Ks = vec3(0.333333, 0.333333, 0.521569);
    temp.Ns = 9.84615;
    mats[3] = temp;
    //*/

    // Models' X-axis position
    float trans[4] = { -4.5, -1.5, 1.5, 4.5 };
    
    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // Camera and VP arrays update
        camera->update();
        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;

        // bind obj
        glBindVertexArray(objVAO);
        
        
        // Pass the uniform light properties
        glm::vec3 lightPos = lposition;
        auto lightPos_cameraspace = vec3(viewMatrix * vec4(lightPos, 1.0f));
        glUniform3f(lightLocation, lightPos_cameraspace.x, lightPos_cameraspace.y, lightPos_cameraspace.z); // light
        glUniform3f(glGetUniformLocation(shaderProgram, "light_color"), light_red, light_green, light_blue);
        glUniform1f(glGetUniformLocation(shaderProgram, "light_power"), light_power);

        // Instatiate 4 Suzannes
        for (int i = 0; i < 4; i++){
            glm::mat4 modelMatrix = glm::translate(mat4(), vec3(trans[i], 0.0f, 0.0f));

            // Transfer MVP to the shaders
            glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
            glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

            // Define the material for each suzanne
            glUniform3f(glGetUniformLocation(shaderProgram, "mat.Ks"), mats[i].Ks.x, mats[i].Ks.y, mats[i].Ks.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "mat.Kd"), mats[i].Kd.x, mats[i].Kd.y, mats[i].Kd.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "mat.Ka"), mats[i].Ka.x, mats[i].Ka.y, mats[i].Ka.z);
            glUniform1f(glGetUniformLocation(shaderProgram, "mat.Ns"), mats[i].Ns);
        
            // Bind textures and transmit diffuse and specular maps to the GPU
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTexture);
            glUniform1i(diffuceColorSampler, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularTexture);
            glUniform1i(specularColorSampler, 1);

            // draw
            glDrawArrays(GL_TRIANGLES, 0, objVertices.size());
        
        }
        glfwSwapBuffers(window);

        glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);
}

void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
    static double lastTime = glfwGetTime();
    const float speed = 1.0f;

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);
    if (deltaTime > 0.5f) deltaTime = 0.1f;

    // Light movement using IJKLUO keys
    // Move light forward
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        lposition += vec3(0.0f, 0.0f, -1.0f) * deltaTime * speed;
    }
    // Move light backward
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        lposition -= vec3(0.0f, 0.0f, -1.0f) * deltaTime * speed;
    }
    // Strafe light right
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        lposition += vec3(1.0f, 0.0f, 0.0f) * deltaTime * speed;
    }
    // Strafe light left
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        lposition -= vec3(1.0f, 0.0f, 0.0f) * deltaTime * speed;
    }
    // Move light up
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        lposition += vec3(0.0f, 1.0f, 0.0f) * deltaTime * speed;
    }
    // Move light down
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        lposition -= vec3(0.0f, 1.0f, 0.0f) * deltaTime * speed;
    }

    // Light color manipulation using RGB keys
    // Increment the red tone of the light
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        light_red += 0.05;
        if (light_red > 1.0) light_red = 0.0;
    }
    // Increment the green tone of the light
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        light_green += 0.05;
        if (light_green > 1.0) light_green = 0.0;
    }
    // Increment the blue tone of the light
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        light_blue += 0.05;
        if (light_blue > 1.0) light_blue = 0.0;
    }

    // Light power manipulation using MN keys
    // Increment the light power
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        light_power += 0.5;
    }
    // Decrement the light power
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        light_power -= 0.5;
        if (light_power < 0.0f) light_power = 0.0;
    }

    lastTime = currentTime;
}

void initialize()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        throw runtime_error("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        throw runtime_error(string(string("Failed to open GLFW window.") +
            " If you have an Intel GPU, they are not 3.3 compatible." +
            "Try the 2.1 version.\n"));
    }
    glfwMakeContextCurrent(window);

    // Start GLEW extension handler
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
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


    glfwSetKeyCallback(window, pollKeyboard);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // enable textures
    glEnable(GL_TEXTURE_2D);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);
}

int main(void)
{
    try
    {
        initialize();
        createContext();
        mainLoop();
        free();
    }
    catch (exception& ex)
    {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}
