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


#define LIGHT_SOURCE 2
// ability to change emission strength of light source
//  ability to show soft colours of neighboring objects on the other objects


struct RayTracingMaterial{
    glm::vec4 colour;
    glm::vec4 emissionColour;
    glm::vec4 specularColour;
    float emissionStrength;
    float smoothness;
    float specularProbability;
    int flag;
};


struct SphereSmall {
    glm::vec4 position;
    float radius;
    float padding[3];  
    RayTracingMaterial material;
};


struct HitInfo{
    bool did_hit = false;
    float dst;
    glm::vec3 hit_point;
    glm::vec3 normal;
};

struct Ray{
    glm::vec3 origin;
    glm::vec3 dir;
};


HitInfo RaySphere(Ray ray,  float sphereCenter, float sphereRadius){
    HitInfo hit_info;
    glm::vec3 offset_ray_origin = ray.origin - sphereCenter;
    float a  = glm::dot(ray.dir,ray.dir);
    float b = 2 * glm::dot(offset_ray_origin,ray.dir);
    float c = glm::dot(offset_ray_origin,offset_ray_origin) - sphereRadius*sphereRadius;
    float discrim = b*b - 4*a*c;

    if(discrim >= 0){
        float dst = (-1*b - std::sqrt(discrim))/(2*a);
        hit_info.did_hit = true;
        hit_info.dst = dst;
        hit_info.hit_point = ray.origin + (dst*ray.dir);
        hit_info.normal = glm::normalize(sphereCenter - hit_info.hit_point); 
    }
    return hit_info;
}





void framebuffer_size_callback(GLFWwindow* window, int width, int height);



class Camera{
    public:
        bool firstClick = true;
        float fpitch = 0;
        int width = 0;
        int height = 0;
        glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);  
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::mat4 view;
        glm::vec4 viewport; 
        glm::mat4 projection; 
        const float radius = 10.0f;
        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;
        const float sensitivity = 100.0f;
        const float cameraSpeed = 0.010f; 

        Camera(int w, int h, float FOVdeg, float near_plane, float far_plane):width(w), height(h) {
            viewport = glm::vec4 (0, 0, width, height); 
            view = glm::lookAt(cameraPos, cameraPos + Orientation, cameraUp);
            projection  = glm::perspective(glm::radians(FOVdeg), (float)(width/height), near_plane, far_plane);
        }
        void move(float FOVdeg, float near_plane, float far_plane, GLuint& shader, const char* uniform, const char* uniform2, glm::mat4 model){
            view = glm::lookAt(cameraPos, cameraPos + Orientation, cameraUp);
            glUniformMatrix4fv(glGetUniformLocation(shader, uniform), 1, GL_FALSE, glm::value_ptr(projection * view * model));
            glUniformMatrix4fv(glGetUniformLocation(shader, uniform2), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shader, "u_RayOrigin"), 1, glm::value_ptr(cameraPos));

        }

        void check_ray_hits(){
            for(int i{0}; i<height; i++){
                for(int j{0}; j<width; j++){
                    float winX = i;
                    float winY = viewport[3] - j; // Invert Y coordinate   
                    // Near plane (z = 0.0f)
                    glm::vec3 rayStart = glm::unProject(
                        glm::vec3(winX, winY, 0.0f), 
                        view, projection, viewport
                    );

                    // Far plane (z = 1.0f)
                    glm::vec3 rayEnd = glm::unProject(
                        glm::vec3(winX, winY, 1.0f), 
                        view, projection, viewport
                    );
                    Ray ray;
                    ray.origin = cameraPos;
                    ray.dir =  glm::normalize(rayEnd - rayStart); 
                    bool did_hit = RaySphere(ray,0,1).did_hit;
                }
            }
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
        glm::vec3 position;
        RayTracingMaterial material;
        float radius = 1;
        float PI = 3.14;

        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal

        Sphere(int stack_c, int sector_c, glm::vec3 position_, RayTracingMaterial material_, int radius_):stackCount(stack_c), sectorCount(sector_c), position(position_), material(material_), radius(radius_){}

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

void create_spheres(const std::vector<Sphere>& sphereObjects, GLuint& sphereSSBO, GLuint shaderProgram);

// Vertex Shader - processes each vertex
const char* vertexShaderSource = R"(
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment Shader - determines pixel colors
const char* fragmentShaderSource = R"(
#version 430 core


out vec4 FragColor;

uniform vec3 u_RayOrigin;
uniform vec3 u_CameraOrientation;
uniform vec2 u_Resolution;
uniform mat4 view;
uniform mat4 projection;

in vec2 TexCoord;
int Frame;
int maxBounceCount = 10;

#define LIGHT_SOURCE 2

struct RayTracingMaterial
{
    vec4 colour;
    vec4 emissionColour;
    vec4 specularColour;
    float emissionStrength;
    float smoothness;
    float specularProbability;
    int flag;
};

struct Sphere {
    vec4 position;
    float radius;
    float padding[3];  
    RayTracingMaterial material;
};
layout(std430, binding = 0) readonly buffer SphereBuffer {
    Sphere spheres[]; 
};




struct HitInfo{
    bool did_hit;
    float dst;
    vec3 hit_point;
    vec3 normal;
    RayTracingMaterial material;
};

struct Ray{
    vec3 origin;
    vec3 dir;
};


int NextRandom(int state)
{
    state = state * 747796405 + 2891336453;
    int result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return result;
}

float RandomValue(int state)
{
    return NextRandom(state) / 4294967295.0; 
}

// Random value in normal distribution (with mean=0 and sd=1)
float RandomValueNormalDistribution(int state)
{
    float theta = 2 * 3.1415926 * RandomValue(state);
    float rho = sqrt(-2 * log(RandomValue(state)));
    return rho * cos(theta);
}


vec3 RandomDirectionHemisphere(vec3 normal, int state){
    float x  = RandomValueNormalDistribution(state);
    float y  = RandomValueNormalDistribution(state);
    float z  = RandomValueNormalDistribution(state);
    vec3 dir = normalize(vec3(x,y,z));
    return dir* sign(dot(normal,dir));
}


HitInfo RaySphere(Ray ray,  vec3 sphereCenter, float sphereRadius, RayTracingMaterial material){
    HitInfo hit_info;
    hit_info.did_hit = false;
    hit_info.dst = 1e10;
    hit_info.hit_point = vec3(1.0,1.0,1.0);
    hit_info.normal = vec3(1.0,1.0,1.0);

    vec3 offset_ray_origin = ray.origin  - sphereCenter;
    float a  = dot(ray.dir,ray.dir);
    float b = 2 * dot(offset_ray_origin,ray.dir);
    float c = dot(offset_ray_origin,offset_ray_origin) - sphereRadius*sphereRadius;
    float discrim = b*b - 4*a*c;

    if(discrim >= 0){
        float dst = (-b - sqrt(discrim))/(2*a);
        if (dst >= 0.0) {
            hit_info.did_hit = true;
            hit_info.dst = dst;
            hit_info.hit_point = ray.origin + dst * ray.dir;
            hit_info.normal = normalize(hit_info.hit_point - sphereCenter);
            hit_info.material = material;
        }
    }

    return hit_info;
};

vec4 GetEnvironmentLight(Ray ray){
    vec4 emitted_light = vec4(0.0, 0.0, 0.0, 0.0);
    for(int i=0; i < spheres.length(); ++i){
        if(spheres[i].material.flag != LIGHT_SOURCE){
            continue;
        }
        HitInfo hit_info = RaySphere(ray, vec3(spheres[i].position), spheres[i].radius,spheres[i].material);
        if(hit_info.did_hit){
            emitted_light =  spheres[i].material.emissionColour * spheres[i].material.emissionStrength;
        }        
    }
    return emitted_light;
}


HitInfo CalculateRayCollision(Ray ray){
    vec3 u_SphereCenter;
    float u_SphereRadius;
    RayTracingMaterial u_SphereMaterial;

    HitInfo closest_hit_info;
    float distance = 1e10;
    vec4 colour = vec4(0.2, 0.3, 0.3, 1.0);

    for(int i=0; i < spheres.length(); ++i){
        u_SphereCenter = vec3(spheres[i].position);
        u_SphereRadius = spheres[i].radius;
        u_SphereMaterial = spheres[i].material;

        HitInfo hit_info = RaySphere(ray,  u_SphereCenter, u_SphereRadius,  u_SphereMaterial);
        if(hit_info.did_hit && hit_info.dst<distance){
            distance = hit_info.dst;
            closest_hit_info = hit_info;
        }        
    }
    return closest_hit_info;
}


vec4 Trace(){
    vec2 uv = TexCoord * 2.0 - 1.0;
    vec2 pixelCoord = TexCoord * u_Resolution;
    int pixelIndex = int(pixelCoord.y + pixelCoord.x * u_Resolution.x);
    int state = pixelIndex + Frame * 719393;

    vec4 incoming_light = vec4(0.0f,0.0f,0.0f,0.0f);
    vec4 ray_color = vec4(1.0f,1.0f,1.0f,1.0f);

    vec4 target = inverse(projection) * vec4(uv.x, uv.y, 1.0, 1.0);
    vec3 rayDir = vec3(inverse(view) * vec4(normalize(vec3(target) / target.w), 0.0));
    
    Ray ray;
    ray.origin = u_RayOrigin;
    ray.dir = normalize(rayDir);

    int count = 0;
    while(count< maxBounceCount){
        HitInfo hit_info = CalculateRayCollision(ray);
        if(hit_info.did_hit){
            RayTracingMaterial material  = hit_info.material;
            if(count == 1 && material.flag == LIGHT_SOURCE){
                incoming_light = vec4(1.0f,1.0f,1.0f,1.0f) * ray_color;
                break;
            }
            ray.origin = hit_info.hit_point;
            ray.dir = RandomDirectionHemisphere(hit_info.normal,state);
            vec4 emitted_light = material.emissionColour * material.emissionStrength;
            incoming_light += emitted_light * ray_color; 
            ray_color *= material.colour;  
        }
        else{
            incoming_light = vec4(0.0f,0.0f,0.0f,1.0f) * ray_color;
            break;
        }

        count+=1;
    }
    return incoming_light;
}




void main()
{
    FragColor = Trace();
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

    std::vector<Sphere>sphere_arr;
    
    RayTracingMaterial material;
    material.colour = glm::vec4(1.0f,0.0f,0.0f,1.0f);
    material.emissionColour = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    material.specularColour = glm::vec4(1.0f,0.0f,0.0f,1.0f);
    material.emissionStrength =0.0f;
    material.smoothness=1.0f;
    material.specularProbability=1.0f;
    material.flag =1;

    glm::vec3 sphereCenter =glm::vec3(0.0f,0.0f,-7.0f);
    Sphere sphere(30,30,sphereCenter,material, 5.0f);
    sphere_arr.push_back(sphere);



    
    RayTracingMaterial material2;
    material2.colour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.emissionColour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.specularColour= glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.emissionStrength =1.0f;
    material2.smoothness=1.0f;
    material2.specularProbability=1.0f;
    material2.flag = LIGHT_SOURCE;

    Sphere sphere2(30,30, glm::vec3(2.0f,2.0f,-100.0f),material2, 80.0f);
    sphere_arr.push_back(sphere2);


    RayTracingMaterial material3;
    material3.colour = glm::vec4(0.0f,1.0f,1.0f,1.0f);
    material3.emissionColour = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    material3.specularColour= glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material3.emissionStrength =1.0f;
    material3.smoothness=1.0f;
    material3.specularProbability=1.0f;
    material3.flag = 1;

    Sphere sphere3(30,30, glm::vec3(7.0f,7.0f,-5.0f),material3, 3.0f);
    sphere_arr.push_back(sphere3);

    Camera camera(1200,1200,45.0f, 0.1f, 100.0f);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 1200, "OpenGL Window", NULL, NULL);
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
    GLuint sphereSSBO = 0;

    create_spheres(sphere_arr, sphereSSBO,shaderProgram);

    // Create fullscreen quad (covers entire screen in NDC coordinates)
    float quadVertices[] = {
        // positions        // texture coords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // bottom-left
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // top-right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f   // bottom-right
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute (if needed)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // Render loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        camera.process_inputs(window);  
        camera.view = glm::lookAt(camera.cameraPos, camera.cameraPos + camera.Orientation, camera.cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "u_RayOrigin"), 1, glm::value_ptr(camera.cameraPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "u_CameraOrientation"), 1, glm::value_ptr(camera.Orientation));
        glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), 1200.0f, 1200.0f);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      
        
        glBindVertexArray(0);
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
};

void create_spheres(const std::vector<Sphere>& sphereObjects, GLuint& sphereSSBO, GLuint shaderProgram) {
    std::vector<SphereSmall> spheres;   
    for (const auto& obj : sphereObjects) {
        SphereSmall s;
        s.position = glm::vec4(obj.position, 1.0f);
        s.radius = obj.radius;
        s.padding[0] = 0.0f;
        s.padding[1] = 0.0f;
        s.padding[2] = 0.0f;
        s.material = obj.material;
        
        spheres.push_back(s);
    }

    if (sphereSSBO == 0) glGenBuffers(1, &sphereSSBO);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(SphereSmall), 
                 spheres.data(), GL_STATIC_DRAW);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO);
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "NumSpheres"), (int)spheres.size());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}