#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"
#include <iostream>


using namespace glm;

Camera::Camera(GLFWwindow* window) : window(window)
{
    // Initialize the attributes
    position = glm::vec3(0, 0, 5);
    horizontalAngle = 3.14f;
    verticalAngle = 0.0f;
    FoV = 45.0f;
    speed = 3.0f;
    mouseSpeed = 0.001f;
    fovSpeed = 0.03f;
}


void Camera::onMouseMove(double xPos, double yPos) {
    static double lastxPos = xPos;
    static double lastyPos = yPos;

    horizontalAngle += mouseSpeed * (lastxPos - xPos);
    verticalAngle += mouseSpeed * (lastyPos - yPos);

    lastxPos = xPos;
    lastyPos = yPos;
}

void Camera::update()
{
    
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);


    // Spherical coordinates to create the basic vectors
	vec3 direction(
		cos(verticalAngle)*sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle)*cos(horizontalAngle)
    );

    // Right vector
	vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0.0f,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    vec3 up = cross(right, direction);

    // Create the WASD movement controls
    // Move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		position -= right * deltaTime * speed;
    }

    // Create zoom effects using arrows
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		FoV -= fovSpeed;
        if (FoV <= 0) FoV = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		FoV += fovSpeed;
	}

    // Create up and down movement using Q and E
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        position += up * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        position -= up * deltaTime * speed;
    }

    vec3 up_copy = up;
    /* Peek around the corner effect, digital using just shift
    float peekShift = 0.1f;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        up_copy -= right * peekShift;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        up_copy += right * peekShift;
    }
    //*/

    //* Peek around the corner effect, analog using a static variable and speed
    static float peekShift = 0.0f;
    float peekSpeed = 0.001f;    //the speed of the shift
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        peekShift -= peekSpeed;
        if (peekShift <= -0.15f) peekShift = -0.15f;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        peekShift += peekSpeed;
        if (peekShift >= 0.15f) peekShift = 0.15f;
    }
    up_copy += right * peekShift;
    //*/

    // Update projection and view matrices
	projectionMatrix = perspective(radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    viewMatrix = lookAt(
		position,
        position + direction,
        up_copy
    );

    lastTime = currentTime;
}