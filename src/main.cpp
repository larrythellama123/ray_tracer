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
#include <fstream>
#include <iostream>
#include <sstream> 
#include <string> 
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

struct Triangle{
    glm::vec4 a;
    glm::vec4 b;
    glm::vec4 c;
    RayTracingMaterial material;
};

struct BVH{
    glm::vec4 min = vec4(1e10,1e10,1e10,1.0f);
    glm::vec4 max = vec4(0.0f,0.0f,0.0f,1.0f);
    glm::vec4 centre = min;
};
void GrowBVHVertice(BVH bvh, vec4 vertice){
    bvh.min = min(bvh.min, vertice);
    bvh.max = max(bvh.max, vertice);
    bvh.centre = (bvh.min+bvh.max)/2;
};
BVH GrowBVHTriangle(std::vector<Triangle> triangles){
    BVH bvh;
    for(int i {0};  i < triangles.size(); i++){
        GrowBVHVertice(bvh, triangles[i].a);
        GrowBVHVertice(bvh,triangles[i].b);
        GrowBVHVertice(bvh,triangles[i].c);
    }
    return BVH;
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

void initPingPongBuffers(int width, int height, GLuint* fbos, GLuint* textures);

void resetBuffers(GLuint fboA, GLuint fboB) ;

std::vector<Triangle> load_object(std::string file_path, RayTracingMaterial material, float screen_offset){

    std::ifstream file(file_path);

    std::vector<glm::vec4> vertice_data;

    std::vector<Triangle> triangle_data;

    triangle_data.reserve(300000); 

    vertice_data.reserve(500000); // Pre-allocate reasonable size



    int line_number = 0;

    if (!file.is_open()) {

        std::cerr << "Error: Could not open file!" << std::endl;

    }

    std::string line;

    while (std::getline(file, line)) {

        line_number++;


        std::istringstream iss(line);

        std::string prefix;

        iss >> prefix;

        if(prefix == "v"){

            float x, y, z;

            iss >> x >> y >> z;  

            glm::vec4 vector = glm::vec4(x*10,y*10,(z*10)-screen_offset,1.0f);

            vertice_data.push_back(vector);

        }

        if(prefix == "f"){

            int x, y, z;
            if (iss >> x >> y >> z) {

                // Convert to 0-based indexing and check bounds

                int idx1 = x - 1;

                int idx2 = y - 1; 

                int idx3 = z - 1;



                // Bounds checking

                if (idx1 >= 0 && idx1 < vertice_data.size() &&

                    idx2 >= 0 && idx2 < vertice_data.size() &&

                    idx3 >= 0 && idx3 < vertice_data.size()) {



                    Triangle tri = {vertice_data[idx1], vertice_data[idx2], vertice_data[idx3], material};
                    triangle_data.push_back(tri);

                } else {

                    std::cerr << "Error: Invalid face indices on line " << line_number 

                            << " (indices: " << x << "," << y << "," << z 

                            << ", vertex count: " << vertice_data.size() << ")" << std::endl;

                }

            } else {

                std::cerr << "Warning: Invalid face on line " << line_number << std::endl;

            }

        }

    }

    return triangle_data;
}

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
        const float cameraSpeed = 0.90f; 

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

                    // Far plane (z
// Far plan
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
        bool process_inputs(GLFWwindow *window)
        {
            bool flag = false;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            {
                cameraPos += cameraSpeed * Orientation;
                flag = true; 
            }
                  
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            {
                cameraPos -= cameraSpeed * Orientation;
                flag = true;   
            }
               

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            {
                cameraPos -= glm::normalize(glm::cross(Orientation, cameraUp)) * cameraSpeed;
                flag = true;
            }
                   

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            {
                cameraPos += glm::normalize(glm::cross(Orientation, cameraUp)) * cameraSpeed;
                flag = true;
            }
                   


            
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
                    flag = true;   

            }
            else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
            {
                // Unhides cursor since camera is not looking around anymore
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                // Makes sure the next time the camera looks around it doesn't jump
                firstClick = true;
            }
            return flag;
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
void displayImage(GLuint texture, GLuint displayShader) {
    glUseProgram(displayShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(displayShader, "displayTexture"), 0);
    // VAO is already bound
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void create_spheres(const std::vector<Sphere>& sphereObjects, GLuint& sphereSSBO, GLuint shaderProgram);
void create_triangles(const std::vector<Triangle>& triangles, GLuint& triangleSSBO, GLuint shaderProgram);

//Grow ALL BVHs

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

uniform int Frame;


in vec2 TexCoord;
int maxBounceCount = 5;

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

struct Triangle{
    vec4 a;
    vec4 b;
    vec4 c;
    RayTracingMaterial material;
};

layout(std430, binding = 0) readonly buffer SphereBuffer {
    Sphere spheres[]; 
};

layout(std430, binding = 1) readonly buffer TriangleBuffer {
    Triangle triangles[]; 
};

layout(std430, binding = 2) readonly buffer BVHBuffer {
    BVH BVHs[]; 
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

struct BVH{
    vec4 min = vec4(1e10,1e10,1e10,1.0f);
    vec4 max = vec4(0.0f,0.0f,0.0f,1.0f);
    vec4 centre = min;
};

struct BVHNode{

}


float RandomValue(inout uint state)
{
    
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    result = (result >> 22u) ^ result;
    return float(result) / 4294967295.0;
}


float RandomValueNormalDistribution(inout uint state)
{
    float theta = 2 * 3.14159 * RandomValue(state);
    float rho = sqrt(-2 * log(RandomValue(state)));
    return rho * cos(theta);
}



vec3 RandomDirection(inout uint state)
{
    float x = RandomValueNormalDistribution(state);
    float y = RandomValueNormalDistribution(state);
    float z = RandomValueNormalDistribution(state);
    return normalize(vec3(x, y, z));
}

vec3 RandomDirectionHemisphere(vec3 normalVector, inout uint state)
{
    vec3 randomDirectionVector = RandomDirection(state);
    if (dot(normalVector, randomDirectionVector) < 0)
    {
        randomDirectionVector = -randomDirectionVector;
    }
    return randomDirectionVector;
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

HitInfo RayTriangle(const Ray ray, const Triangle tri)
    {
        vec3 orig  = ray.origin;
        vec3 dir = ray.dir;

        vec3 v0 = vec3(tri.a);
        vec3 v1 = vec3(tri.b);
        vec3 v2 = vec3(tri.c);

        HitInfo hit_info;
        hit_info.did_hit = false;
        hit_info.dst = 1e10;
        hit_info.hit_point = vec3(1.0,1.0,1.0);
        hit_info.normal = vec3(1.0,1.0,1.0);

        hit_info.did_hit = false;
        vec3 v0v1 = v1 - v0;
        vec3 v0v2 = v2 - v0;
        vec3 pvec = cross(dir,v0v2);
        float det = dot(pvec,v0v1);

        // If the determinant is negative, the triangle is back-facing.
        // If the determinant is close to 0, the ray misses the triangle.
        // If det is close to 0, the ray and triangle are parallel.

        const float kEpsilon = 0.0000001;
        // if (det < kEpsilon || fabs(det) < kEpsilon){
        //     return hit_info;
        // }
        if (abs(det) < kEpsilon) {
            return hit_info;
        }
       
        float invDet = 1 / det;

        vec3 tvec = orig - v0;
        float u = dot(tvec,pvec) * invDet;
        if (u < 0 || u > 1) return hit_info;

        vec3 qvec = cross(tvec, v0v1);
        float v = dot(dir,qvec) * invDet;
        if (v < 0 || u + v > 1) return hit_info;
        
        float t = dot(v0v2,qvec) * invDet;

        // return if dst t is less than small value as this means a ray dir is being multiplied by a negative, 
        //so it is moving in the incorrect dir to the ray dir
        if (t < kEpsilon) return hit_info;

        
        hit_info.did_hit = true;
        hit_info.hit_point = orig + t*dir;
        hit_info.dst = t;
        hit_info.normal = normalize(cross(v0v2,v0v1));
        hit_info.material = tri.material;

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
    HitInfo hit_info;

    for(int i=0; i < spheres.length(); ++i){
        u_SphereCenter = vec3(spheres[i].position);
        u_SphereRadius = spheres[i].radius;
        u_SphereMaterial = spheres[i].material;

        hit_info = RaySphere(ray,  u_SphereCenter, u_SphereRadius,  u_SphereMaterial);
        if(hit_info.did_hit && hit_info.dst<distance){
            distance = hit_info.dst;
            closest_hit_info = hit_info;
        }        
    }
    for(int i=0; i < triangles.length(); ++i){
        hit_info = RayTriangle(ray, triangles[i]);
        if(hit_info.did_hit && hit_info.dst<distance){
            distance = hit_info.dst;
            closest_hit_info = hit_info;
        }    
    }
    return closest_hit_info;
}


vec4 Trace(inout uint state){
    BVH bvh
    vec2 uv = TexCoord * 2.0 - 1.0;
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
            incoming_light = vec4(0.2f,0.1f,0.3f,1.0f) * ray_color;
            break;
        }

        count+=1;
    }
    return incoming_light;
}

layout(rgba32f, binding = 0) uniform readonly image2D prevFrame;
layout(rgba32f, binding = 1) uniform writeonly image2D currentFrame;

void main()
{
    uint rays_per_pixel = 10;
    vec2 pixelCoord = TexCoord * u_Resolution;
    int pixelIndex = int(pixelCoord.y + pixelCoord.x * u_Resolution.x);
    ivec2 pixelCoords = ivec2(pixelCoord); 
    
    vec4 totalColor = vec4(0.0);
    for(uint i = 0; i < rays_per_pixel; i++){
        uint state = uint(pixelIndex) + (Frame * rays_per_pixel + i) * 719393u;
        totalColor += Trace(state);
    }
    vec4 currentRayColor = totalColor / float(rays_per_pixel);
    
    if(Frame == 0){
        imageStore(currentFrame, pixelCoords, currentRayColor);  
    } else {
        vec4 accumulatedColor = imageLoad(prevFrame, pixelCoords);  
        
        vec4 finalColor = (accumulatedColor * float(Frame) + currentRayColor) / float(Frame + 1);
        
        imageStore(currentFrame, pixelCoords, finalColor); 
    }
}


)";




const char* displayVertexShader = R"(
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

// Add this simple fragment shader for display
const char* displayFragmentShader = R"(
#version 430 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D displayTexture;

void main()
{
    FragColor = texture(displayTexture, TexCoord);
}
)";

// Create display shader program (add this in main after creating main shader)

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

GLuint createShaderProgram2() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, displayVertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, displayFragmentShader);
    
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
    material.colour = glm::vec4(1.0f,1.0f,0.0f,1.0f);
    material.emissionColour = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    material.specularColour = glm::vec4(1.0f,0.0f,0.0f,1.0f);
    material.emissionStrength =0.0f;
    material.smoothness=1.0f;
    material.specularProbability=1.0f;
    material.flag =1;

    glm::vec3 sphereCenter =glm::vec3(0.0f,0.0f,0.0f);
    Sphere sphere(30,30,sphereCenter,material, 50.0f);
    sphere_arr.push_back(sphere);



    
    RayTracingMaterial material2;
    material2.colour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.emissionColour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.specularColour= glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material2.emissionStrength =1.0f;
    material2.smoothness=1.0f;
    material2.specularProbability=1.0f;
    material2.flag = LIGHT_SOURCE;

    Sphere sphere2(30,30, glm::vec3(2.0f,2.0f,-700.0f),material2, 300.0f);
    // sphere_arr.push_back(sphere2);


    RayTracingMaterial material3;
    material3.colour = glm::vec4(1.0f,0.0f,0.0f,1.0f);
    material3.emissionColour = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    material3.specularColour= glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material3.emissionStrength =1.0f;
    material3.smoothness=1.0f;
    material3.specularProbability=1.0f;
    material3.flag = 1;

    Sphere sphere3(30,30, glm::vec3(0.0f,-250.0f,-7.0f),material3, 250.0f);
    // sphere_arr.push_back(sphere3);

    RayTracingMaterial material4;
    material4.colour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material4.emissionColour = glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material4.specularColour= glm::vec4(1.0f,1.0f,1.0f,1.0f);
    material4.emissionStrength =1.0f;
    material4.smoothness=1.0f;
    material4.specularProbability=1.0f;
    material4.flag = LIGHT_SOURCE;

    // Sphere sphere4(30,30, glm::vec3(-80.0f,30.0f,-7.0f),material4, 30.0f);
    Sphere sphere4(30,30, glm::vec3(0.0f,0.0f,-300.0f),material4, 200.0f);
    sphere_arr.push_back(sphere4);

    std::vector<Triangle> triangle_arr = load_object("VideoShip.obj", material,60.0f);
    std::vector<Triangle> tri_arr;
    Triangle tri = {glm::vec4(1.0f,1.0f,1.0f,1.0f),glm::vec4(50.0f,70.0f,-7.0f,1.0f),glm::vec4(5.0f,100.0f,-7.0f,1.0f), material};
    tri_arr.push_back(tri);

    int width = 1400;
    int height = 1400;
    Camera camera(width,height,45.0f, 0.1f, 100.0f);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Window", NULL, NULL);
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
    GLuint triangleSSBO = 0;
    GLuint BVHSSBO = 0;



    create_spheres(sphere_arr, sphereSSBO,shaderProgram);
    create_triangles(tri_arr, triangleSSBO, shaderProgram);
    
    BVH bvh = GrowBVHTriangle(const std::vector<Triangle>& triangles);
    std::vector<BVH> BVHs;
    BVHs.push_back(bvh);
    create_BVHs(BVHs, BVHSSBO, shaderProgram); 
    SAH_BVH();

    // Create fullscreen quad (covers entire screen in NDC coordinates)
    float quadVertices[] = {
        // positions        // texture coords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // bottom-left
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // top-right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f   // bottom-right
    };

    
    GLuint fbos[2], textures[2];
    int currentWriteBuffer = 0;
    initPingPongBuffers(width, height, fbos, textures);

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

    int Frame = 0;
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    GLuint displayShader = createShaderProgram2(); 

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        bool moved = camera.process_inputs(window);
        if(moved){
            Frame = 0;
            resetBuffers(fbos[0], fbos[1]);  
        }
        
        int readBuffer = 1 - currentWriteBuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  
        
        glUseProgram(shaderProgram);
        // Bind images to image units
        glBindImageTexture(0, textures[readBuffer], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);   
        glBindImageTexture(1, textures[currentWriteBuffer], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); 
        
        glBindVertexArray(VAO);
        
        camera.view = glm::lookAt(camera.cameraPos, camera.cameraPos + camera.Orientation, camera.cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "u_RayOrigin"), 1, glm::value_ptr(camera.cameraPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "u_CameraOrientation"), 1, glm::value_ptr(camera.Orientation));
        glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), 1400.0f, 1400.0f);
   
        
        glUniform1i(glGetUniformLocation(shaderProgram, "Frame"), Frame);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        displayImage(textures[currentWriteBuffer], displayShader); 
        
        currentWriteBuffer = 1 - currentWriteBuffer;
        Frame++;
        
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


void create_triangles(const std::vector<Triangle>& triangles, GLuint& triangleSSBO, GLuint shaderProgram) {
    if (triangleSSBO == 0) glGenBuffers(1, &triangleSSBO);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), 
                 triangles.data(), GL_STATIC_DRAW);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleSSBO);
    glUseProgram(shaderProgram);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}



void create_BVHs(const std::vector<BVH>& BVHs, GLuint& BVHSSBO, GLuint shaderProgram) {

    if (BVHSSBO == 0) glGenBuffers(1, &BVHSSBO);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, BVHSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, BVHs.size() * sizeof(BVH), 
                 BVHs.data(), GL_STATIC_DRAW);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, BVHSSBO);
    glUseProgram(shaderProgram);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
//For each bucket split, do the split and return 2 BVHs
//calc  the min and max coords for each of the splits 
//check cost function for each of the BVHs
//divide again based on recursion

//For each bucket split, do the split and return 2 BVHs
//calc  the min and max coords for each of the splits 
//check cost function for each of the BVHs
//add them to a min heap then we can extract the min BVh from the heap 

//return the best BVH from the OG BVH split and then wrap this func in a for loop to cover the BVH split until 4 primitves





BVH SAH_BVH(const std::vector<Triangle>& triangle_arr, BVH bvh){
    uint8_t num_buckets = 16;
    BVH best_BVH;
    float best_cost_function = 1e10;
    std::vector<glm::vec3> centroids;
    for(int i {0}; i<triangle_arr.size(); i++){
        glm::vec3 centroid = glm::vec3(triangle_arr[i].a + triangle_arr[i].b + triangle_arr[i].c)*0.5;
        centroids.emplace_back(centroid);
    }
    //split along the z-axis
    float step_Z = (bvh.max.z -bvh.min.z)/(num_buckets-1);
    float step_X = (bvh.max.x -bvh.min.x)/(num_buckets-1); 
    float step_Y = (bvh.max.y -bvh.min.y)/(num_buckets-1); 


    

    glm::vec3 extent_bvh = bvh.max -bvh.min;
    float parent_area = extent_bvh.x*extent_bvh.y + extent_bvh.y*extent_bvh.z + extent_bvh.z*extent_bvh.x;
.
    //split across z-axis first
    for(int i{0};  i<num_buckets; i++){
        //use 15 parititons to split 16 buckets
        //For each split check the cost function of the other 2 buckets 
        // keep track of which BVH had the best cost function
        BVH bvh_tmp_1;
        bvh_tmp_1.max = vec4(bvh.max.x,bvh.max.y,bvh.min.z+step_Z*(i+1),1.0f);
        bvh_tmp_1.min = bvh.min;
        glm::vec3 extent_bvh_1 = bvh_tmp_1.max - bvh_tmp_1.min;
        float child_area_1 = extent_bvh_1.x*extent_bvh_1.y + extent_bvh_1.y*extent_bvh_1.z + extent_bvh_1.z*extent_bvh_1.x;

        BVH bvh_tmp_2;
        bvh_tmp_2.min = bvh_tmp_1.max;
        bvh_tmp_2.max = bvh.max;
        glm::vec3 extent_bvh_2 = bvh_tmp_2.max - bvh_tmp_2.min;
        float area = extent_bvh_2.x*extent_bvh_2.y + extent_bvh_2.y*extent_bvh_2.z + extent_bvh_2.z*extent_bvh_2.x;
        //set the z boundary for this partition
        float boundary_Z = bvh.min.z+step_Z*(i+1);
        
        float boundary_Z = bvh.min +  vec4(0.0f,0.0f,step_Z*(i+1),0.0f);
        float boundary_X = bvh.min +  vec4(step_X*(i+1),0.0f,0.0f,0.0f);
        float boundary_Y = bvh.min +  vec4(0.0f,step_Y*(i+1),0.0f,0.0f);

        //ratio
        float SAR_1 = child_area_1/parent_area;
        float SAR_2 = child_area_2/parent_area;
        
        uint8_t count_bvh_tmp_1 = 0;
        uint8_t count_bvh_tmp_2 = 0;

        //check number of centroids in each partition
        for(int j{0};  j<centroids.size(); j++){
            if (centroids[j].z < boundary_Z){
                count_bvh_tmp_1 += 1;
            }
            else{
                count_bvh_tmp_2 += 1;
            }
        }
        float cost_function = 0.125 + SAR_1*count_bvh_tmp_1 + SAR_2*count_bvh_tmp_2;
        if(cost_function < best_cost_function){
            best_cost_function = cost_fucntion;
            if(count_bvh_tmp_2>count_bvh_tmp_1){
                best_BVH = bvh_tmp_2; 
            }
            else{
                best_BVH = bvh_tmp_1;
            }
        }
    }

    return best_bvh;
}

BVH SplittingAndCost(vec4 bvh_tmp_1_max, float boundary, float best_cost_function, BVH best_BVH, BVH bvh, float parent_area, int[] flag){
        //use 15 parititons to split 16 buckets
        //For each split check the cost function of the other 2 buckets 
        // keep track of which BVH had the best cost function
        BVH bvh_tmp_1;
        bvh_tmp_1.max = bvh_tmp_1_max;
        bvh_tmp_1.min = bvh.min;
        glm::vec3 extent_bvh_1 = bvh_tmp_1.max - bvh_tmp_1.min;
        float child_area_1 = extent_bvh_1.x*extent_bvh_1.y + extent_bvh_1.y*extent_bvh_1.z + extent_bvh_1.z*extent_bvh_1.x;

        BVH bvh_tmp_2;
        bvh_tmp_2.min = bvh_tmp_1.max;
        bvh_tmp_2.max = bvh.max;
        glm::vec3 extent_bvh_2 = bvh_tmp_2.max - bvh_tmp_2.min;
        float child_area_2 = extent_bvh_2.x*extent_bvh_2.y + extent_bvh_2.y*extent_bvh_2.z + extent_bvh_2.z*extent_bvh_2.x;

        
        float boundary_Z = bvh.min +  vec4(0.0f,0.0f,step_Z*(i+1),0.0f);
        float boundary_X = bvh.min +  vec4(step_X*(i+1),0.0f,0.0f,0.0f);
        float boundary_Y = bvh.min +  vec4(0.0f,step_Y*(i+1),0.0f,0.0f);

        //ratio
        float SAR_1 = child_area_1/parent_area;
        float SAR_2 = child_area_2/parent_area;
        
        uint8_t count_bvh_tmp_1 = 0;
        uint8_t count_bvh_tmp_2 = 0;

        float centroid = 0;


        //check number of centroids in each partition
        for(int j{0};  j<centroids.size(); j++){
            if (centroids[j].z < boundary_Z){
                count_bvh_tmp_1 += 1;
            }
            else{
                count_bvh_tmp_2 += 1;
            }
        }
        float cost_function = 0.125 + SAR_1*count_bvh_tmp_1 + SAR_2*count_bvh_tmp_2;
        if(cost_function < best_cost_function){
            best_cost_function = cost_function;
            if(count_bvh_tmp_2>count_bvh_tmp_1){
                best_BVH = bvh_tmp_2; 
            }
            else{
                best_BVH = bvh_tmp_1;
            }
        }
}

uint count_centroids(const centroids){

}




void initPingPongBuffers(int width, int height,GLuint* fbos,  GLuint* textures) {
    glGenFramebuffers(2, fbos);
    glGenTextures(2, textures);

    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        
        // Use GL_RGBA32F for high-precision accumulation
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
    }
}


void resetBuffers(GLuint fboA, GLuint fboB) {
    // Clear first buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    // Clear second buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboB);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

