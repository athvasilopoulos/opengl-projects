// Include C++ headers
#include <iostream>
#include <string>
#include <map>

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
#include <common/skeleton.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
struct Light; struct Material;
void uploadMaterial(const Material& mtl);
void uploadLight(const Light& light);
map<int, mat4> calculateModelPoseFromCoordinates(map<int, float> q);
vector<mat4> calculateSkinningTransformations(map<int, float> q);
vector<float> calculateSkinningIndices();

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Skinning and Animation"

// global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
// light properties
GLuint LaLocation, LdLocation, LsLocation, lightPositionLocation, lightPowerLocation;
// material properties
GLuint KdLocation, KsLocation, KaLocation, NsLocation;

GLuint surfaceVAO, surfaceVerticesVBO, surfacesBoneIndecesVBO, maleBoneIndicesVBO;
Drawable *segment, *skeletonSkin;
GLuint useSkinningLocation, boneTransformationsLocation;
Skeleton* skeleton;

struct Light {
    glm::vec4 La;
    glm::vec4 Ld;
    glm::vec4 Ls;
    glm::vec3 lightPosition_worldspace;
    float power;
};

struct Material {
    glm::vec4 Ka;
    glm::vec4 Kd;
    glm::vec4 Ks;
    float Ns;
};

const Material boneMaterial{
    vec4{ 0.1, 0.1, 0.1, 1 },
    vec4{ 1.0, 1.0, 1.0, 1 },
    vec4{ 0.3, 0.3, 0.3, 1 },
    0.1f
};

Light light{
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec3{ 0, 4, 4 },
    20.0f
};

// Coordinate names for mnemonic indexing
enum CoordinateName {
    PELVIS_TRA_X = 0, PELVIS_TRA_Y, PELVIS_TRA_Z, PELVIS_ROT_X, PELVIS_ROT_Y,
    PELVIS_ROT_Z, HIP_R_FLEX, HIP_R_ADD, HIP_R_ROT, HIP_L_FLEX, HIP_L_ADD, HIP_L_ROT, 
    KNEE_R_FLEX, KNEE_L_FLEX, ANKLE_R_FLEX, ANKLE_L_FLEX,
    LUMBAR_FLEX, LUMBAR_BEND, LUMBAR_ROT, DOFS
};
// Joint names for mnemonic indexing
enum JointName {
    BASE = 0, HIP_R, HIP_L, KNEE_R, KNEE_L, ANKLE_R, ANKLE_L, SUBTALAR_R, SUBTALAR_L, 
    MTP_R, MTP_L, BACK, JOINTS
};
// Body names for mnemonic indexing
enum BodyName {
    PELVIS = 0, FEMUR_R, FEMUR_L, TIBIA_R, TIBIA_L, TALUS_R, TALUS_L, CALCN_R, CALCN_L, 
    TOES_R, TOES_L, TORSO, BODIES
};

// Default pose used for binding the skeleton and the mesh
static const map<int, float> bindingPose = {
    {CoordinateName::PELVIS_TRA_X,  0.0f},
    {CoordinateName::PELVIS_TRA_Y, 0.0f},
    {CoordinateName::PELVIS_TRA_Z, 0.0f},
    {CoordinateName::PELVIS_ROT_X,  0.0f},
    {CoordinateName::PELVIS_ROT_Y, 0.0f},
    {CoordinateName::PELVIS_ROT_Z, 0.0f},
    {CoordinateName::HIP_R_FLEX, 3.0f},
    {CoordinateName::HIP_L_FLEX, 3.0f},
    {CoordinateName::HIP_R_ADD, -5.0f},
    {CoordinateName::HIP_L_ADD, 5.0f},
    {CoordinateName::HIP_R_ROT, 0.0f},
    {CoordinateName::HIP_L_ROT, 0.0f},
    {CoordinateName::KNEE_R_FLEX, -15.0f},
    {CoordinateName::KNEE_L_FLEX, -15.0f},
    {CoordinateName::ANKLE_R_FLEX, 15.0f},
    {CoordinateName::ANKLE_L_FLEX, 15.0f},
    {CoordinateName::LUMBAR_FLEX, 0.0f},
    {CoordinateName::LUMBAR_BEND, 0.0f},
    {CoordinateName::LUMBAR_ROT, 0.0f}
};

// Function to pass the material to the shaders
void uploadMaterial(const Material& mtl) {
    glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
    glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(NsLocation, mtl.Ns);
}

// Function to pass the light properties to the shaders
void uploadLight(const Light& light) {
    glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
    glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
                light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(lightPowerLocation, light.power);
}

// Function to calculate the local transformations for a pose
map<int, mat4> calculateModelPoseFromCoordinates(map<int, float> q) {
    map<int, mat4> jointLocalTransformations;

    // base/pelvis joint
    mat4 pelvisTra = translate(mat4(), vec3(
        q[CoordinateName::PELVIS_TRA_X],
        q[CoordinateName::PELVIS_TRA_Y],
        q[CoordinateName::PELVIS_TRA_Z]));
    mat4 pelvisRotX = rotate(mat4(), radians(q[CoordinateName::PELVIS_ROT_X]), vec3(1, 0, 0));
    mat4 pelvisRotY = rotate(mat4(), radians(q[CoordinateName::PELVIS_ROT_Y]), vec3(0, 1, 0));
    mat4 pelvisRotZ = rotate(mat4(), radians(q[CoordinateName::PELVIS_ROT_Z]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::BASE] = pelvisTra * pelvisRotX * pelvisRotY * pelvisRotZ;

    // right hip joint
    vec3 hipROffset = vec3(-0.072, -0.068, 0.086);
    mat4 hipRTra = translate(mat4(), hipROffset);
    mat4 hipRRotX = rotate(mat4(), radians(q[CoordinateName::HIP_R_ADD]), vec3(1, 0, 0));
    mat4 hipRRotY = rotate(mat4(), radians(q[CoordinateName::HIP_R_ROT]), vec3(0, 1, 0));
    mat4 hipRRotZ = rotate(mat4(), radians(q[CoordinateName::HIP_R_FLEX]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::HIP_R] = hipRTra * hipRRotX * hipRRotY * hipRRotZ;

    // right knee joint
    vec3 kneeROffset = vec3(0.0, -0.40, 0.0);
    mat4 kneeRTra = translate(mat4(1.0), kneeROffset);
    mat4 kneeRRotZ = rotate(mat4(), radians(q[CoordinateName::KNEE_R_FLEX]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::KNEE_R] = kneeRTra * kneeRRotZ;

    // right ankle joint
    vec3 ankleROffset = vec3(0, -0.430, 0);
    mat4 ankleRTra = translate(mat4(1.0), ankleROffset);
    mat4 ankleRRotZ = rotate(mat4(), radians(q[CoordinateName::ANKLE_R_FLEX]), vec3(0, 0, 1));
    mat4 talusRModelMatrix = ankleRRotZ;
    jointLocalTransformations[JointName::ANKLE_R] =  ankleRTra * ankleRRotZ;

    // right calcn joint
    vec3 calcnROffset = vec3(-0.062, -0.053, 0.010);
    mat4 calcnRTra = translate(mat4(1.0), calcnROffset);
    jointLocalTransformations[JointName::SUBTALAR_R] = calcnRTra;

    // right mtp joint
    vec3 toesROffset = vec3(0.184, -0.002, 0.001);
    mat4 mtpRTra = translate(mat4(1.0), toesROffset);
    jointLocalTransformations[JointName::MTP_R] = mtpRTra;

    // back joint
    vec3 backOffset = vec3(-0.103, 0.09, 0.0);
    mat4 lumbarTra = translate(mat4(1.0), backOffset);
    mat4 lumbarRotX = rotate(mat4(), radians(q[CoordinateName::LUMBAR_BEND]), vec3(1, 0, 0));
    mat4 lumbarRotY = rotate(mat4(), radians(q[CoordinateName::LUMBAR_ROT]), vec3(0, 1, 0));
    mat4 lumbarRotZ = rotate(mat4(), radians(q[CoordinateName::LUMBAR_FLEX]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::BACK] = lumbarTra * lumbarRotX * lumbarRotY * lumbarRotZ;

    // left hip joint
    vec3 hipLOffset = vec3(-0.072, -0.068, -0.086);
    mat4 hipLTra = translate(mat4(), hipLOffset);
    mat4 hipLRotX = rotate(mat4(), radians(q[CoordinateName::HIP_L_ADD]), vec3(1, 0, 0));
    mat4 hipLRotY = rotate(mat4(), radians(q[CoordinateName::HIP_L_ROT]), vec3(0, 1, 0));
    mat4 hipLRotZ = rotate(mat4(), radians(q[CoordinateName::HIP_L_FLEX]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::HIP_L] = hipLTra * hipLRotX * hipLRotY * hipLRotZ;

    // left knee joint
    vec3 kneeLOffset = vec3(0.0, -0.40, 0.0);
    mat4 kneeLTra = translate(mat4(1.0), kneeLOffset);
    mat4 kneeLRotZ = rotate(mat4(), radians(q[CoordinateName::KNEE_L_FLEX]), vec3(0, 0, 1));
    jointLocalTransformations[JointName::KNEE_L] = kneeLTra * kneeLRotZ;

    // left ankle joint
    vec3 ankleLOffset = vec3(0, -0.430, 0);
    mat4 ankleLTra = translate(mat4(1.0), ankleLOffset);
    mat4 ankleLRotZ = rotate(mat4(), radians(q[CoordinateName::ANKLE_L_FLEX]), vec3(0, 0, 1));
    mat4 talusLModelMatrix = ankleLRotZ;
    jointLocalTransformations[JointName::ANKLE_L] = ankleLTra * ankleLRotZ;

    // left calcn joint
    vec3 calcnLOffset = vec3(-0.062, -0.053, -0.010);
    mat4 calcnLTra = translate(mat4(1.0), calcnLOffset);
    jointLocalTransformations[JointName::SUBTALAR_L] = calcnLTra;

    // left mtp joint
    vec3 toesLOffset = vec3(0.184, -0.002, -0.001);
    mat4 mtpLTra = translate(mat4(1.0), toesLOffset);
    jointLocalTransformations[JointName::MTP_L] = mtpLTra;

    return jointLocalTransformations;
}

// Function to calculate the skinning transformations, using the binding pose
vector<mat4> calculateSkinningTransformations(map<int, float> q) {
    auto jointLocalTransformationsBinding = calculateModelPoseFromCoordinates(bindingPose);
    skeleton->setPose(jointLocalTransformationsBinding);
    auto bindingWorldTransformations = skeleton->getJointWorldTransformations();

    auto jointLocalTransformationsCurrent = calculateModelPoseFromCoordinates(q);
    skeleton->setPose(jointLocalTransformationsCurrent);
    auto currentWorldTransformations = skeleton->getJointWorldTransformations();

    vector<mat4> skinningTransformations(JointName::JOINTS);
    for (auto joint : bindingWorldTransformations) {
        mat4 BInvWorld = glm::inverse(joint.second);
        mat4 JWorld = currentWorldTransformations[joint.first];
        skinningTransformations[joint.first] = JWorld * BInvWorld;
    }

    return skinningTransformations;
}

vector<float> calculateSkinningIndices() {
    // Assignment of a body index for each vertex in the model (skin) based
    // on its proximity to a body part
    vector<float> indices;
    for (auto v : skeletonSkin->indexedVertices) {
        // dummy
        //indices.push_back(1.0);
        if (v.y <= -0.07 && v.y >= -0.5 && v.z > 0.00 && v.z < 0.25) {
            indices.push_back(JointName::HIP_R);
        }
        else if (v.y < -0.5 && v.y > -0.85 && v.z > 0.00 &&  v.z < 0.25) {
            indices.push_back(JointName::KNEE_R);
        } 
        else if (v.y <= -0.85 &&  v.y >= -1.0 && v.z > 0.00 && v.z < 0.25) {
            indices.push_back(JointName::ANKLE_R);
        } 
        else if (v.y <= -0.07 && v.y >= -0.5 && v.z < 0.00 && v.z > -0.25) {
            indices.push_back(JointName::HIP_L);
        }
        else if (v.y < -0.5 && v.y > -0.85 && v.z < 0.00 && v.z > -0.25) {
            indices.push_back(JointName::KNEE_L);
        }
        else if (v.y <= -0.85 && v.y >= -1.0 && v.z < 0.00 && v.z > -0.25) {
            indices.push_back(JointName::ANKLE_L);
        }
        else if (v.y > 0.0 || v.y > -0.4 && v.z > 0.25 || v.y > -0.4 && v.z < -0.25) {
            indices.push_back(JointName::BACK);
        } 
        else {
            indices.push_back(JointName::BASE);
        }
    }
    return indices;
}

void createContext() {
    shaderProgram = loadShaders(
        "StandardShading.vertexshader",
        "StandardShading.fragmentshader");

    // Get pointers to uniforms
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
    projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
    KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
    KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
    KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
    NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
    LaLocation = glGetUniformLocation(shaderProgram, "light.La");
    LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
    LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
    lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
    lightPowerLocation = glGetUniformLocation(shaderProgram, "light.power");
    useSkinningLocation = glGetUniformLocation(shaderProgram, "useSkinning");
    boneTransformationsLocation = glGetUniformLocation(shaderProgram, "boneTransformations");

    // A skeleton is a collection of joints and bodies. Each body is independent
    // of each other (conceptually). Furthermore, each body can  have many
    // drawables (geometries) attached. The joints are related to each other
    // and form a parent child relations. A joint is attached on a body.
    skeleton = new Skeleton(modelMatrixLocation, viewMatrixLocation, projectionMatrixLocation);

    // Relation definitions between bodies and joints

    // pelvis root joint
    Joint* baseJoint = new Joint();
    baseJoint->parent = NULL;
    skeleton->joints[JointName::BASE] = baseJoint;

    Body* pelvisBody = new Body();
    pelvisBody->drawables.push_back(new Drawable("models/sacrum.vtp"));
    pelvisBody->drawables.push_back(new Drawable("models/pelvis.vtp"));
    pelvisBody->drawables.push_back(new Drawable("models/l_pelvis.vtp"));
    pelvisBody->joint = baseJoint;
    skeleton->bodies[BodyName::PELVIS] = pelvisBody;

    // right femur
    Joint* hipR = new Joint();
    hipR->parent = baseJoint;
    skeleton->joints[JointName::HIP_R] = hipR;

    Body* femurR = new Body();
    femurR->drawables.push_back(new Drawable("models/femur.vtp"));
    femurR->joint = hipR;
    skeleton->bodies[BodyName::FEMUR_R] = femurR;

    // right tibia
    Joint* kneeR = new Joint();
    kneeR->parent = hipR;
    skeleton->joints[JointName::KNEE_R] = kneeR;

    Body* tibiaR = new Body();
    tibiaR->drawables.push_back(new Drawable("models/tibia.vtp"));
    tibiaR->drawables.push_back(new Drawable("models/fibula.vtp"));
    tibiaR->joint = kneeR;
    skeleton->bodies[BodyName::TIBIA_R] = tibiaR;

    // right talus
    Joint* ankleR = new Joint();
    ankleR->parent = kneeR;
    skeleton->joints[JointName::ANKLE_R] = ankleR;

    Body* talusR = new Body();
    talusR->drawables.push_back(new Drawable("models/talus.vtp"));
    talusR->joint = ankleR;
    skeleton->bodies[BodyName::TALUS_R] = talusR;

    // right calcn
    Joint* subtalarR = new Joint();
    subtalarR->parent = ankleR;
    skeleton->joints[JointName::SUBTALAR_R] = subtalarR;

    Body* calcnR = new Body();
    calcnR->drawables.push_back(new Drawable("models/foot.vtp"));
    calcnR->joint = subtalarR;
    skeleton->bodies[BodyName::CALCN_R] = calcnR;

    // toes
    Joint* mtpR = new Joint();
    mtpR->parent = subtalarR;
    skeleton->joints[JointName::MTP_R] = mtpR;

    Body* toesR = new Body();
    toesR->drawables.push_back(new Drawable("models/bofoot.vtp"));
    toesR->joint = mtpR;
    skeleton->bodies[BodyName::TOES_R] = toesR;

    // torso
    Joint* back = new Joint();
    back->parent = baseJoint;
    skeleton->joints[JointName::BACK] = back;

    Body* torso = new Body();
    torso->drawables.push_back(new Drawable("models/hat_spine.vtp"));
    torso->drawables.push_back(new Drawable("models/hat_jaw.vtp"));
    torso->drawables.push_back(new Drawable("models/hat_skull.vtp"));
    torso->drawables.push_back(new Drawable("models/hat_ribs.vtp"));
    torso->joint = back;
    skeleton->bodies[BodyName::TORSO] = torso;

    // left femur
    Joint* hipL = new Joint();
    hipL->parent = baseJoint;
    skeleton->joints[JointName::HIP_L] = hipL;

    Body* femurL = new Body();
    femurL->drawables.push_back(new Drawable("models/l_femur.vtp"));
    femurL->joint = hipL;
    skeleton->bodies[BodyName::FEMUR_L] = femurL;

    // left tibia
    Joint* kneeL = new Joint();
    kneeL->parent = hipL;
    skeleton->joints[JointName::KNEE_L] = kneeL;

    Body* tibiaL = new Body();
    tibiaL->drawables.push_back(new Drawable("models/l_tibia.vtp"));
    tibiaL->drawables.push_back(new Drawable("models/l_fibula.vtp"));
    tibiaL->joint = kneeL;
    skeleton->bodies[BodyName::TIBIA_L] = tibiaL;

    // left talus
    Joint* ankleL = new Joint();
    ankleL->parent = kneeL;
    skeleton->joints[JointName::ANKLE_L] = ankleL;

    Body* talusL = new Body();
    talusL->drawables.push_back(new Drawable("models/l_talus.vtp"));
    talusL->joint = ankleL;
    skeleton->bodies[BodyName::TALUS_L] = talusL;

    // left calcn
    Joint* subtalarL = new Joint();
    subtalarL->parent = ankleL;
    skeleton->joints[JointName::SUBTALAR_L] = subtalarL;

    Body* calcnL = new Body();
    calcnL->drawables.push_back(new Drawable("models/l_foot.vtp"));
    calcnL->joint = subtalarL;
    skeleton->bodies[BodyName::CALCN_L] = calcnL;

    // left toes
    Joint* mtpL = new Joint();
    mtpL->parent = subtalarL;
    skeleton->joints[JointName::MTP_L] = mtpL;

    Body* toesL = new Body();
    toesL->drawables.push_back(new Drawable("models/l_bofoot.vtp"));
    toesL->joint = mtpL;
    skeleton->bodies[BodyName::TOES_L] = toesL;

    // skin
    skeletonSkin = new Drawable("models/male.obj");
    auto maleBoneIndices = calculateSkinningIndices();
    glGenBuffers(1, &maleBoneIndicesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, maleBoneIndicesVBO);
    glBufferData(GL_ARRAY_BUFFER, maleBoneIndices.size() * sizeof(float),
                 &maleBoneIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(3);
}

void free() {
    delete segment;
    delete skeleton;
    delete skeletonSkin;

    glDeleteBuffers(1, &surfaceVAO);
    glDeleteVertexArrays(1, &surfaceVerticesVBO);
    glDeleteVertexArrays(1, &surfacesBoneIndecesVBO);

    glDeleteVertexArrays(1, &maleBoneIndicesVBO);

    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void mainLoop() {
    camera->position = vec3(0, 0, 2.5);
    do {
        static float last_time = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Camera
        camera->update();
        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;

        // Light
        uploadLight(light);

        // Moonwalk animation creation 
        static float posX = 0.0f;
        float time = glfwGetTime();
        map<int, float> q;
        posX -= 0.5f * (time - last_time);
        if (posX < -2.0f) posX = 2.0f;
        // Create the pose for the current frame
        q[CoordinateName::PELVIS_TRA_X] = posX;
        q[CoordinateName::PELVIS_TRA_Y] = 0;
        q[CoordinateName::PELVIS_TRA_Z] = 0;
        q[CoordinateName::PELVIS_ROT_X] = 0;
        q[CoordinateName::PELVIS_ROT_Y] = 0;
        q[CoordinateName::PELVIS_ROT_Z] = 0;
        q[CoordinateName::HIP_R_FLEX] = 30 * cos(2.5 * time) + 10;
        q[CoordinateName::HIP_R_ADD] = 0;
        q[CoordinateName::HIP_R_ROT] = 0;
        q[CoordinateName::HIP_L_FLEX] = 30 * cos(2.5 * time + radians(180.0f)) + 10;
        q[CoordinateName::HIP_L_ADD] = 0;
        q[CoordinateName::HIP_L_ROT] = 0;
        q[CoordinateName::KNEE_R_FLEX] = -25 * cos(2.5 * time) - 25;
        q[CoordinateName::KNEE_L_FLEX] = -25 * cos(2.5 * time + radians(180.0f)) - 25;
        q[CoordinateName::ANKLE_R_FLEX] = -25 * cos(2.5 * time) - 5;
        q[CoordinateName::ANKLE_L_FLEX] = -25 * cos(2.5 * time + radians(180.0f)) - 5;
        q[CoordinateName::LUMBAR_FLEX] = -15;
        q[CoordinateName::LUMBAR_BEND] = 0;
        q[CoordinateName::LUMBAR_ROT] = -10 * cos(2.5 * time);
        
        // Draw the skeleton
        glUniform1i(useSkinningLocation, 0);
        uploadMaterial(boneMaterial);
        skeleton->draw(viewMatrix, projectionMatrix);

        // Draw the skin (wireframe)
        skeletonSkin->bind();
        mat4 maleModelMatrix = mat4(1);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &maleModelMatrix[0][0]);
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

        // Bone transformations
        auto T = calculateSkinningTransformations(q);
        glUniformMatrix4fv(boneTransformationsLocation, T.size(),
            GL_FALSE, &T[0][0][0]);

        glUniform1i(useSkinningLocation, 1);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        skeletonSkin->draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        last_time = time;
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

    // Cull triangles which normal is not towards the camera
    // glEnable(GL_CULL_FACE);

    // Enable point size when drawing points
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);
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