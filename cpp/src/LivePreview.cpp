#include "LivePreview.hpp"
#include <iostream>

LivePreview* LivePreview::instance = nullptr;

const char* LivePreview::vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* LivePreview::fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

LivePreview::LivePreview(sl::Resolution resolution)
    : texture_id(0), program_id(0), vao_id(0), vbo_id(0),
      window_width(resolution.width), window_height(resolution.height),
      image_updated(false), running(false), stop_requested(false) {
    
    // Allocate image buffer
    current_image.alloc(resolution, sl::MAT_TYPE::U8_C4, sl::MEM::CPU);
    
    std::cout << "LivePreview: Initialized for " << window_width << "x" << window_height << std::endl;
}

LivePreview::~LivePreview() {
    stop();
}

bool LivePreview::start(int argc, char** argv) {
    if (running) {
        std::cout << "LivePreview: Already running" << std::endl;
        return false;
    }
    
    stop_requested = false;
    instance = this;
    
    // Start render thread
    render_thread = std::thread(&LivePreview::renderLoop, this, argc, argv);
    
    // Wait a bit for GL context to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return true;
}

void LivePreview::stop() {
    if (!running && !render_thread.joinable()) {
        return;
    }
    
    std::cout << "LivePreview: Stopping..." << std::endl;
    stop_requested = true;
    
    if (render_thread.joinable()) {
        render_thread.join();
    }
    
    running = false;
    std::cout << "LivePreview: Stopped" << std::endl;
}

void LivePreview::updateImage(const sl::Mat& image) {
    std::lock_guard<std::mutex> lock(image_mutex);
    if (image.isInit() && running) {
        image.copyTo(current_image);
        image_updated = true;
    }
}

void LivePreview::renderLoop(int argc, char** argv) {
    std::cout << "LivePreview: Starting render loop" << std::endl;
    
    // Initialize GLUT (in this thread)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("ZED Live Preview - Left Camera");
    
    // Initialize GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "LivePreview: glewInit failed: " << glewGetErrorString(err) << std::endl;
        return;
    }
    
    // Setup OpenGL
    setupGL();
    
    // Register callbacks
    glutDisplayFunc(LivePreview::displayCallback);
    glutReshapeFunc(LivePreview::reshapeCallback);
    glutKeyboardFunc(LivePreview::keyboardCallback);
    glutIdleFunc(LivePreview::idleCallback);
    
    running = true;
    std::cout << "LivePreview: Render loop started" << std::endl;
    
    // Enter GLUT main loop
    glutMainLoop();
    
    // Cleanup when loop exits
    if (texture_id) glDeleteTextures(1, &texture_id);
    if (vao_id) glDeleteVertexArrays(1, &vao_id);
    if (vbo_id) glDeleteBuffers(1, &vbo_id);
    if (program_id) glDeleteProgram(program_id);
    
    running = false;
    std::cout << "LivePreview: Render loop exited" << std::endl;
}

void LivePreview::setupGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    
    // Create shader program
    if (!createShaderProgram()) {
        std::cerr << "LivePreview: Failed to create shader program" << std::endl;
        return;
    }
    
    // Setup texture
    setupTexture();
    
    // Setup quad for rendering
    float vertices[] = {
        // positions   // texture coords
        -1.0f,  1.0f,  0.0f, 0.0f,  // top left
        -1.0f, -1.0f,  0.0f, 1.0f,  // bottom left
         1.0f, -1.0f,  1.0f, 1.0f,  // bottom right
         1.0f,  1.0f,  1.0f, 0.0f   // top right
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    glGenVertexArrays(1, &vao_id);
    glGenBuffers(1, &vbo_id);
    
    GLuint ebo;
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao_id);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void LivePreview::setupTexture() {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Initialize with black image
    unsigned char* black_data = new unsigned char[window_width * window_height * 4];
    memset(black_data, 0, window_width * window_height * 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, black_data);
    delete[] black_data;
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool LivePreview::compileShader(GLuint& shader_id, GLenum type, const char* source) {
    shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source, NULL);
    glCompileShader(shader_id);
    
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader_id, 512, NULL, infoLog);
        std::cerr << "LivePreview: Shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    return true;
}

bool LivePreview::createShaderProgram() {
    GLuint vertex_shader, fragment_shader;
    
    if (!compileShader(vertex_shader, GL_VERTEX_SHADER, vertex_shader_src)) {
        return false;
    }
    
    if (!compileShader(fragment_shader, GL_FRAGMENT_SHADER, fragment_shader_src)) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);
    
    GLint success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program_id, 512, NULL, infoLog);
        std::cerr << "LivePreview: Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

void LivePreview::updateTexture() {
    std::lock_guard<std::mutex> lock(image_mutex);
    
    if (!image_updated || !current_image.isInit()) {
        return;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, window_width, window_height,
                    GL_RGBA, GL_UNSIGNED_BYTE, current_image.getPtr<sl::uchar1>(sl::MEM::CPU));
    glBindTexture(GL_TEXTURE_2D, 0);
    
    image_updated = false;
}

void LivePreview::render() {
    if (stop_requested) {
        glutLeaveMainLoop();
        return;
    }
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Update texture if new image available
    updateTexture();
    
    // Render textured quad
    glUseProgram(program_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glBindVertexArray(vao_id);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    glutSwapBuffers();
}

// Static callbacks
void LivePreview::displayCallback() {
    if (instance) {
        instance->render();
    }
}

void LivePreview::reshapeCallback(int width, int height) {
    glViewport(0, 0, width, height);
}

void LivePreview::keyboardCallback(unsigned char key, int x, int y) {
    if (key == 27 || key == 'q' || key == 'Q') {  // ESC or Q to quit
        if (instance) {
            instance->stop_requested = true;
        }
    }
}

void LivePreview::idleCallback() {
    // Trigger redisplay at reasonable rate (don't hog CPU)
    std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS max
    glutPostRedisplay();
}

