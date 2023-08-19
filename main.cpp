#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"
#include "camera.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// deltatime is used so that the camera speed is same across all systems
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Constants
int GRID_SIZE = 20;
float wave_amplitude = 25.0f;  // sombrero amplitude
float wave_length = 2.0f;      // sombrero wavelength
float ripple_Strength = 2.5;   // ripple strength
float ripple_frequency = 1.0;  // ripple frequency
float radius_to_center = 10.0; // torus radius to center
float tube_radius = 5.0;       // radius of the tube of torus
float fence_height = 15.0;     // height of the fence of intersecting fences
float stair_distance = 15;     // distance between two stairs
float letterO_height = 0.5;    // height of the letter O
float letterO_size = 5;        // size of letter O
float top_hat_height = 0.25;   // height of top hat function
float bump_height = 0.2;       // height of the bump function
int choice = 1;                // to chose which graph to display

float calculateHeight(float x, float y)
{
    float r = std::sqrt(x * x + y * y);
    return wave_amplitude * (std::sin(r / wave_length) / (r / wave_length));
}
float calculateRipple(float x, float y)
{
    return ripple_Strength * sin(glfwGetTime() * ripple_frequency + x / 5 + y / 5);
}
float calculateTorus(float x, float z)
{
    return sqrt(pow(tube_radius, 2) - pow(radius_to_center - sqrt(pow(x, 2) + pow(z, 2)), 2));
}
float intersectingFences(float x, float y)
{
    return fence_height / exp(pow((x * 5), 2) * pow((y * 5), 2));
}
float sign(float v) // signum function
{
    return (v < 0) ? -1 : ((v > 0) ? 1 : 0);
}
float stairs(float x, float y)
{
    return sign(x - stair_distance + abs(y * 2)) / .5 + sign(x - .5 + abs(y * 2)) / 1;
}
float letterO(float x, float y)
{
    return (-sign(20 - (pow(x, 2) + pow(y, 2))) + sign(20 - (pow(x, 2) / abs(letterO_size) + pow(y, 2) / abs(letterO_size)))) / abs(letterO_height);
}
float topHat(float x, float y)
{
    return (sign(20 - (pow(x, 2) + pow(y, 2))) + sign(20 - (pow(x, 2) / 3 + pow(y, 2) / 3))) / abs(top_hat_height) - 1;
}
float bumps(float x, float y)
{
    return sin(6 * x) * cos(6 * y) / abs(bump_height);
}
bool captureMouse = true;
bool cameraControl = true;
// define in order to toggle mouse caputere
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        captureMouse = !captureMouse;
        if (captureMouse)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        cameraControl = captureMouse;
    }
}
// toggle wireframe Mode
bool wireframeMode = true;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Function Plotter", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    // glfwSetInputMode(window, GLFW_CURSOR,GLFW_CURSOR_HIDDEN);
    glfwSetKeyCallback(window, key_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    Shader ourShader("default.vert", "default.frag");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    float vertices[] = {
        0.0f, 0.0f, 0.0f};

    // for plotted points
    unsigned int VBOcurve, VAOcurve;
    glGenVertexArrays(1, &VAOcurve);
    glGenBuffers(1, &VBOcurve);
    glBindVertexArray(VAOcurve);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    Shader axesShader("axes.vert", "axes.frag");

    // Vertex data for the axes
    float axisVertices[] = {
        // X-axis
        -20.0f, 0.0f, 0.0f,
        20.0f, 0.0f, 0.0f,
        // Y-axis
        0.0f, -20.0f, 0.0f,
        0.0f, 20.0f, 0.0f,
        // Z-axis
        0.0f, 0.0f, -20.0f,
        0.0f, 0.0f, 20.0f};

    // Initialize VAO and VBO for axes
    unsigned int VAOaxes, VBOaxes;
    glGenVertexArrays(1, &VAOaxes);
    glGenBuffers(1, &VBOaxes);
    glBindVertexArray(VAOaxes);
    glBindBuffer(GL_ARRAY_BUFFER, VBOaxes);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Tell OpenGL a new frame is about to begin
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // activate shader
        ourShader.Activate();

        // create transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // retrieve the matrix uniform locations
        unsigned int viewLoc = glGetUniformLocation(ourShader.ID, "view");
        unsigned int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");

        // pass them to the shaders (3 different ways)
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Render plotted mesh (choice 1: Sombrero)
        if (choice == 1)
        {
            std::vector<float> sombreroVertices;
            std::vector<unsigned int> sombreroIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            // Generate vertex data for the sombrero function
            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = calculateHeight(x, z);
                    sombreroVertices.push_back(x);
                    sombreroVertices.push_back(y);
                    sombreroVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    sombreroIndices.push_back(topLeft);
                    sombreroIndices.push_back(bottomLeft);
                    sombreroIndices.push_back(topRight);

                    sombreroIndices.push_back(topRight);
                    sombreroIndices.push_back(bottomLeft);
                    sombreroIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, sombreroVertices.size() * sizeof(float), sombreroVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sombreroIndices.size() * sizeof(unsigned int), sombreroIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, sombreroIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 2: ripple)
        else if (choice == 2)
        {
            std::vector<float> rippleVertices;
            std::vector<unsigned int> rippleIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            // Generate vertex data for the ripple function
            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = calculateRipple(x, z);
                    rippleVertices.push_back(x);
                    rippleVertices.push_back(y);
                    rippleVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    rippleIndices.push_back(topLeft);
                    rippleIndices.push_back(bottomLeft);
                    rippleIndices.push_back(topRight);

                    rippleIndices.push_back(topRight);
                    rippleIndices.push_back(bottomLeft);
                    rippleIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, rippleVertices.size() * sizeof(float), rippleVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, rippleIndices.size() * sizeof(unsigned int), rippleIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, rippleIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 3: torus)
        else if (choice == 3)
        {
            std::vector<float> torusVertices;
            std::vector<unsigned int> torusIndices;
            int numSegments = 40;
            int numRings = 20;

            for (int i = 0; i < numRings; ++i)
            {
                float phi = 2.5f * glm::pi<float>() * static_cast<float>(i) / numRings;
                for (int j = 0; j < numSegments; ++j)
                {
                    float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / numSegments;

                    float x = (radius_to_center + tube_radius * std::cos(theta)) * std::cos(phi);
                    float y = tube_radius * std::sin(theta);
                    float z = (radius_to_center + tube_radius * std::cos(theta)) * std::sin(phi);

                    torusVertices.push_back(x);
                    torusVertices.push_back(y);
                    torusVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int i = 0; i < numRings - 1; ++i)
            {
                for (int j = 0; j < numSegments - 1; ++j)
                {
                    int topLeft = i * numSegments + j;
                    int topRight = topLeft + 1;
                    int bottomLeft = (i + 1) * numSegments + j;
                    int bottomRight = bottomLeft + 1;

                    torusIndices.push_back(topLeft);
                    torusIndices.push_back(bottomLeft);
                    torusIndices.push_back(topRight);

                    torusIndices.push_back(topRight);
                    torusIndices.push_back(bottomLeft);
                    torusIndices.push_back(bottomRight);
                }
            }
            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, torusVertices.size() * sizeof(float), torusVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, torusIndices.size() * sizeof(unsigned int), torusIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, torusIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 4: Intersecting Fences)
        else if (choice == 4)
        {
            std::vector<float> fenceVertices;
            std::vector<unsigned int> fenceIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = intersectingFences(x, z);
                    fenceVertices.push_back(x);
                    fenceVertices.push_back(y);
                    fenceVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    fenceIndices.push_back(topLeft);
                    fenceIndices.push_back(bottomLeft);
                    fenceIndices.push_back(topRight);

                    fenceIndices.push_back(topRight);
                    fenceIndices.push_back(bottomLeft);
                    fenceIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, fenceVertices.size() * sizeof(float), fenceVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, fenceIndices.size() * sizeof(unsigned int), fenceIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, fenceIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 5: Stairs)
        else if (choice == 5)
        {
            std::vector<float> stairsVertices;
            std::vector<unsigned int> stairsIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = stairs(x, z);
                    stairsVertices.push_back(x);
                    stairsVertices.push_back(y);
                    stairsVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    stairsIndices.push_back(topLeft);
                    stairsIndices.push_back(bottomLeft);
                    stairsIndices.push_back(topRight);

                    stairsIndices.push_back(topRight);
                    stairsIndices.push_back(bottomLeft);
                    stairsIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, stairsVertices.size() * sizeof(float), stairsVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, stairsIndices.size() * sizeof(unsigned int), stairsIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, stairsIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 6: Letter O)
        else if (choice == 6)
        {
            std::vector<float> letterVertices;
            std::vector<unsigned int> letterIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = letterO(x, z);
                    letterVertices.push_back(x);
                    letterVertices.push_back(y);
                    letterVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    letterIndices.push_back(topLeft);
                    letterIndices.push_back(bottomLeft);
                    letterIndices.push_back(topRight);

                    letterIndices.push_back(topRight);
                    letterIndices.push_back(bottomLeft);
                    letterIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, letterVertices.size() * sizeof(float), letterVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, letterIndices.size() * sizeof(unsigned int), letterIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, letterIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 7: Top Hat)
        else if (choice == 7)
        {
            std::vector<float> hatVertices;
            std::vector<unsigned int> hatIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = topHat(x, z);
                    hatVertices.push_back(x);
                    hatVertices.push_back(y);
                    hatVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    hatIndices.push_back(topLeft);
                    hatIndices.push_back(bottomLeft);
                    hatIndices.push_back(topRight);

                    hatIndices.push_back(topRight);
                    hatIndices.push_back(bottomLeft);
                    hatIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, hatVertices.size() * sizeof(float), hatVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, hatIndices.size() * sizeof(unsigned int), hatIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, hatIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // Render plotted mesh (choice 8: Bumps)
        else if (choice == 8)
        {
            std::vector<float> bumpsVertices;
            std::vector<unsigned int> bumpsIndices;
            int gridSize = 40;
            float xStep = 1.0f;
            float zStep = 1.0f;

            for (float x = -GRID_SIZE; x < GRID_SIZE; x += xStep)
            {
                for (float z = -GRID_SIZE; z < GRID_SIZE; z += zStep)
                {
                    float y = bumps(x, z);
                    bumpsVertices.push_back(x);
                    bumpsVertices.push_back(y);
                    bumpsVertices.push_back(z);
                }
            }

            // Generate indices for triangles
            for (int row = 0; row < gridSize - 1; row++)
            {
                for (int col = 0; col < gridSize - 1; col++)
                {
                    int topLeft = row * gridSize + col;
                    int topRight = topLeft + 1;
                    int bottomLeft = (row + 1) * gridSize + col;
                    int bottomRight = bottomLeft + 1;

                    bumpsIndices.push_back(topLeft);
                    bumpsIndices.push_back(bottomLeft);
                    bumpsIndices.push_back(topRight);

                    bumpsIndices.push_back(topRight);
                    bumpsIndices.push_back(bottomLeft);
                    bumpsIndices.push_back(bottomRight);
                }
            }

            glBindVertexArray(VAOcurve);

            // Bind the vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VBOcurve);
            glBufferData(GL_ARRAY_BUFFER, bumpsVertices.size() * sizeof(float), bumpsVertices.data(), GL_STATIC_DRAW);

            // Bind the element buffer
            GLuint EBOcurve;
            glGenBuffers(1, &EBOcurve);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcurve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bumpsIndices.size() * sizeof(unsigned int), bumpsIndices.data(), GL_STATIC_DRAW);

            // Set the vertex attribute pointers
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Calculate and set the model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Set the color based on the y value
            unsigned int colorLoc = glGetUniformLocation(ourShader.ID, "color");
            glUniform1f(colorLoc, 1.0f); // Set a constant color for the mesh

            // Draw the mesh using indices
            glDrawElements(GL_TRIANGLES, bumpsIndices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the vertex array and buffers
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glDeleteBuffers(1, &EBOcurve);
        }

        // ourShader.Delete();
        axesShader.Activate();
        // retrieve the matrix uniform locations
        viewLoc = glGetUniformLocation(axesShader.ID, "view");
        projectionLoc = glGetUniformLocation(axesShader.ID, "projection");

        // pass them to the shaders
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAOaxes);
        glDrawArrays(GL_LINES, 0, 6);

        // Check if ImGui window is hovered in order to make it interactive
        if (ImGui::IsWindowHovered())
        {
            captureMouse = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            if (captureMouse)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }

        ImGui::Begin("Interactive Controls");
        // Text that appears in the window
        ImGui::Text("Parameters");
        // Slider that appears in the window
        if (choice == 1)
        {
            ImGui::SliderFloat("Wave Amplitude", &wave_amplitude, 15.0f, 35.0f);
            ImGui::SliderFloat("wave Length", &wave_length, 0.0f, 10.0f);
        }
        if (choice == 2)
        {
            ImGui::SliderFloat("Ripple Strength", &ripple_Strength, 0.0f, 20.0f);
            ImGui::SliderFloat("Ripple Frequency", &ripple_frequency, 0.0f, 20.0f);
        }
        if (choice == 3)
        {
            ImGui::SliderFloat("Radiust to Center", &radius_to_center, 0.0f, 20.0f);
            ImGui::SliderFloat("Tube Radius", &tube_radius, 0.0f, 20.0f);
        }
        if (choice == 4)
        {
            ImGui::SliderFloat("Fence Height", &fence_height, 0.0f, 25.0f);
        }
        if (choice == 5)
        {
            ImGui::SliderFloat("Stair Distance", &fence_height, 0.0f, 25.0f);
        }
        if (choice == 6)
        {
            ImGui::SliderFloat("Size", &letterO_size, 0.0f, 15.0f);
            ImGui::SliderFloat("Height", &letterO_height, 0.0f, 1.0f);
        }
        if (choice == 7)
        {
            ImGui::SliderFloat("Top Hat Height", &top_hat_height, 0.0f, 5.0f);
        }
        if (choice == 8)
        {
            ImGui::SliderFloat("Bump Height", &bump_height, 0.0f, 2.0f);
        }
        ImGui::RadioButton("Sombrero Function", &choice, 1);
        ImGui::RadioButton("Wave Function", &choice, 2);
        ImGui::RadioButton("Torus Function", &choice, 3);
        ImGui::RadioButton("Intersecting Fences", &choice, 4);
        ImGui::RadioButton("Stairs", &choice, 5);
        ImGui::RadioButton("Letter O", &choice, 6);
        ImGui::RadioButton("Top Hat", &choice, 7);
        ImGui::RadioButton("Bumps", &choice, 8);
        if (ImGui::Checkbox("Wireframe Mode", &wireframeMode))
        {
            glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Ends the window
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Deletes all ImGUI instances
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &VAOcurve);
    glDeleteBuffers(1, &VBOcurve);
    glDeleteVertexArrays(1, &VAOaxes);
    glDeleteBuffers(1, &VBOaxes);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    // change parameters
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        if (choice == 1)
            wave_amplitude += 0.1;
        else if (choice == 2)
            ripple_frequency += 0.1;
        else if (choice == 3)
            radius_to_center += 0.1;
        else if (choice == 4)
            fence_height += 0.1;
        else if (choice == 5)
            stair_distance += 0.1;
        else if (choice == 6)
            letterO_height += 0.005;
        else if (choice == 7)
            top_hat_height += 0.005;
        else if (choice == 8)
            bump_height += 0.005;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        if (choice == 1)
            wave_amplitude -= 0.1;
        else if (choice == 2)
            ripple_frequency -= 0.1;
        else if (choice == 3)
            radius_to_center -= 0.1;
        else if (choice == 4)
            fence_height -= 0.1;
        else if (choice == 5)
            stair_distance -= 0.1;
        else if (choice == 6)
            letterO_height -= 0.005;
        else if (choice == 7)
            top_hat_height -= 0.005;
        else if (choice == 8)
            bump_height -= 0.005;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        if (choice == 1)
            wave_length += 0.02;
        else if (choice == 2)
            ripple_Strength += 0.1;
        else if (choice == 3)
            tube_radius += 0.1;
        else if (choice == 6)
            letterO_size += 0.1;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        if (choice == 1)
            wave_length -= 0.02;
        if (choice == 2)
            ripple_Strength -= 0.1;
        else if (choice == 3)
            tube_radius -= 0.1;
        else if (choice == 6)
            letterO_size -= 0.1;
    }
    // change graph to be displayed
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        choice = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        choice = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        choice = 3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        choice = 4;
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        choice = 5;
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
        choice = 6;
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
        choice = 7;
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
        choice = 8;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    if (captureMouse)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}