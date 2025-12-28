#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);

class Camera{
    public:
        bool firstClick = true;
        float fyaw = 0;
        float fpitch = 0;
        int width = 0;
        int height = 0;
        glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);  
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::mat4 view;
        glm::mat4 model = glm::mat4(1.0f);
        const float radius = 10.0f;
        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;
        const float sensitivity = 100.0f;
        const float cameraSpeed = 0.005f; // adjust accordingly

        Camera(int w, int h):width(w), height(h) {
            // view = glm::lookAt(cameraPos, cameraPos + Orientation, cameraUp);
        }

        void move(float FOVdeg, float near_plane, float far_plane, GLuint& shader, const char* uniform){
            view = glm::lookAt(cameraPos, cameraPos + Orientation, cameraUp);
            glm::mat4 projection  = glm::perspective(glm::radians(FOVdeg), (float)(width/height), near_plane, far_plane);
            glUniformMatrix4fv(glGetUniformLocation(shader, uniform), 1, GL_FALSE, glm::value_ptr(projection * view));
        }
        void process_inputs(GLFWwindow *window)
        {

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                cameraPos += cameraSpeed * Orientation;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                cameraPos -= cameraSpeed * Orientation;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                cameraPos -= glm::normalize(glm::cross(Orientation, cameraUp)) * cameraSpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                cameraPos += glm::normalize(glm::cross(Orientation, cameraUp)) * cameraSpeed;

            //rotate
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                fyaw += 2;
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
                fyaw -= 2;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            // Hides mouse cursor
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

                // Prevents camera from jumping on the first click
                if (firstClick)
                {
                    glfwSetCursorPos(window, (width / 2), (height / 2));
                    firstClick = false;
                }

                // Stores the coordinates of the cursor
                double mouseX;
                double mouseY;
                // Fetches the coordinates of the cursor
                glfwGetCursorPos(window, &mouseX, &mouseY);

                // Normalizes and shifts the coordinates of the cursor such that they begin in the middle of the screen
                // and then "transforms" them into degrees 
                float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
                float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;

                // Calculates upcoming vertical change in the Orientation
                glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, cameraUp)));

                // Decides whether or not the next vertical Orientation is legal or not
                if (glm::abs(glm::angle(newOrientation, cameraUp) - glm::radians(90.0f)) <= glm::radians(85.0f))
                {
                    Orientation = newOrientation;
                }

                // Rotates the Orientation left and right
                Orientation = glm::rotate(Orientation, glm::radians(-rotY), cameraUp);

                // Sets mouse cursor to the middle of the screen so that it doesn't end up roaming around
                glfwSetCursorPos(window, (width / 2), (height / 2));

        }
        else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
        {
            // Unhides cursor since camera is not looking around anymore
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // Makes sure the next time the camera looks around it doesn't jump
            firstClick = true;
        }
    }

};

class Sphere{
    public:
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<unsigned int> indices;
        int stackCount = 5;
        int sectorCount = 5;
        int radius = 1;
        float PI = 3.14;


        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal

        Sphere(int stack_c, int sector_c):stackCount(stack_c), sectorCount(sector_c){ 
            calculate_vertices_normals();
        }

        void calculate_vertices_normals(){
            float sectorStep = 2 * PI / sectorCount;
            float stackStep = PI / stackCount;
            float sectorAngle, stackAngle;
            int vertice_size = 0;

            for(int i = 0; i <= stackCount; ++i){

                stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
                xy = radius * cosf(stackAngle);             // r * cos(u)
                z = radius * sinf(stackAngle);        
                int k1 = i * (sectorCount + 1);
                int k2 = k1 + sectorCount + 1;      

                for(int j = 0;  j <= sectorCount; ++j, ++k1, ++k2){
                    sectorAngle = j * sectorStep;
                    x =  xy * cosf(sectorAngle);
                    y = xy * sinf(sectorAngle);
                    vertices.push_back(x);
                    vertices.push_back(y);
                    vertices.push_back(z);

                    
                    if(i != 0) {
                        indices.push_back(k1);
                        indices.push_back(k2);
                        indices.push_back(k1 + 1);
                    }
                    if(i != (stackCount - 1)) {
                        indices.push_back(k1 + 1);
                        indices.push_back(k2);
                        indices.push_back(k2 + 1);
                    }

                    nx = x *lengthInv;
                    ny = y *lengthInv;
                    nz = z *lengthInv;

                    normals.push_back(nx);
                    normals.push_back(ny);
                    normals.push_back(nz);

                }
            }
        }

       

    private:

};

// Vertex Shader - processes each vertex
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 camMatrix;


void main()
{
    gl_Position = camMatrix * vec4(aPos, 1.0);
}
)";

// Fragment Shader - determines pixel colors
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0); // Orange color
}
)";

// Function to compile a shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
    }
    
    return shader;
}

GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    // Link shaders into a program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
    }
    
    // Clean up - shaders are linked, so we can delete them
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main() {

    if (!glfwInit()) {
        return -1;
    }

    Sphere sphere(30,30);
    Camera camera(1400,800);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1400, 800, "OpenGL Window", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Create shader program
    GLuint shaderProgram = createShaderProgram();


    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphere.vertices.size()* sizeof(float), sphere.vertices.data(), GL_STATIC_DRAW);


    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.indices.size()*sizeof(float), sphere.indices.data(), GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        // glDrawArrays(GL_POINTS, 0, vertexCount);
        camera.process_inputs(window);
        camera.move(45.0f, 0.1f, 100.0f, shaderProgram, "camMatrix");
        glDrawElements(GL_TRIANGLES, 
               sphere.indices.size(),           
               GL_UNSIGNED_INT, // type of indices in EBO
               0);           // offset in the EBO

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}








// ?dsfsdfsdfdsf




// #include <iostream>
// #include <GL/glew.h>
// #include <GLFW/glfw3.h>

// void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// void processInput(GLFWwindow *window);

// // Window dimensions
// const unsigned int SCR_WIDTH = 800;
// const unsigned int SCR_HEIGHT = 600;

// int main()
// {
//     // 1. Initialize GLFW
//     if (!glfwInit())
//     {
//         std::cerr << "Failed to initialize GLFW" << std::endl;
//         return -1;
//     }
    
//     // Optional: Configure OpenGL version (e.g., OpenGL 3.3 Core Profile)
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//     // 2. Create a windowed mode window and its OpenGL context
//     GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
//     if (window == NULL)
//     {
//         std::cerr << "Failed to create GLFW window" << std::endl;
//         glfwTerminate();
//         return -1;
//     }
//     glfwMakeContextCurrent(window);
//     glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

//     // 3. Initialize GLEW/GLAD (load all OpenGL function pointers)
//     // Note: If using GLAD, replace this with gladLoadGL(glfwGetProcAddress)
//     if (glewInit() != GLEW_OK) {
//         std::cerr << "Failed to initialize GLEW" << std::endl;
//         return -1;
//     }    

//     // 4. Render loop
//     while (!glfwWindowShouldClose(window))
//     {
//         // Input processing
//         processInput(window);

//         // Rendering commands here
//         glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Set background color
//         glClear(GL_COLOR_BUFFER_BIT); // Clear the color buffer

//         // Swap front and back buffers and poll for events
//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }

//     // 5. Terminate GLFW and clean up
//     glfwTerminate();
//     return 0;
// }

// // Function to handle all input
// void processInput(GLFWwindow *window)
// {
//     // Close window if Escape key is pressed
//     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//         glfwSetWindowShouldClose(window, true);
// }


// ????????????????????????????????????????


// int main() {
//     Initialize GLFW
//     Sphere sphere(30,30);
//     unsigned int VBO, VAO;
//     glGenVertexArrays(1, &VAO);
//     glGenBuffers(1, &VBO);

//     // Bind the VAO first
//     glBindVertexArray(VAO);

//     // Bind the VBO and copy the vertex data into it
//     glBindBuffer(GL_ARRAY_BUFFER, VBO);
//         float vertices[] = {
//         -0.5f, -0.5f, 0.0f,
//          0.5f, -0.5f, 0.0f,
//          0.0f,  0.5f, 0.0f
//     };
//     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
//     glEnableVertexAttribArray(0);

//     // Color attribute (location = 1 in shader)
//     // The offset is 3 * sizeof(float) because color data starts after position data (3 floats)
//     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//     glEnableVertexAttribArray(1);

//     // Use the shader program
//     glUseProgram(shaderProgram);


//     if (!glfwInit()) {
//         return -1;
//     }
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//     // Create a windowed mode window and its OpenGL context
//     GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Project", NULL, NULL);
//     if (!window) {
//         glfwTerminate();
//         return -1;
//     }
//     glfwMakeContextCurrent(window);
//     glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

   

//     // Render loop
//     while (!glfwWindowShouldClose(window)) {
//         // Render commands
//         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT);

//         glBindVertexArray(VAO); 
//         glDrawArrays(GL_TRIANGLES, 0, 3);

//         // Swap buffers and poll IO events
//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }

//     glfwTerminate();
//     return 0;
// }
