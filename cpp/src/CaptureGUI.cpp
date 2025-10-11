#include "CaptureGUI.hpp"
#include "PerformanceProfiler.hpp"
#include <iostream>
#include <sys/stat.h>
#include <cmath>
#include <algorithm>

// Static member definition
CaptureGUI* CaptureGUI::instance = nullptr;

// Shader sources for texture rendering
const char* CaptureGUI::vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* CaptureGUI::fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    vec4 texColor = texture(ourTexture, TexCoord);
    FragColor = vec4(texColor.bgr, texColor.a);  // Convert BGR to RGB
}
)";

CaptureGUI::CaptureGUI()
    : initialized(false), recording(false), exit_requested(false),
      finished_recording(false), cancelled_recording(false),
      texture_id(0), program_id(0), vao_id(0), vbo_id(0),
      window_width(1920), window_height(1080), frame_count(0), first_frame(true) {
    
    // Set up default camera parameters
    init_params.camera_resolution = sl::RESOLUTION::HD1080;
    init_params.camera_fps = 30;
    init_params.depth_mode = sl::DEPTH_MODE::NEURAL;//_LIGHT;//NEURAL_PLUS;
    init_params.coordinate_units = sl::UNIT::METER;
    init_params.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    
    // Clear any existing instance
    if (instance != nullptr) {
        delete instance;
    }
    instance = this;
    
    // Setup runtime parameters - Optimized for spatial mapping quality  
    runtime_params.confidence_threshold = 100; // Higher confidence for better depth quality
    runtime_params.texture_confidence_threshold = 100;
    // Enable depth processing for NEURAL_PLUS mode - essential for spatial mapping
    runtime_params.enable_depth = true;
    runtime_params.enable_fill_mode = true; // Fill mode improves depth completeness
}

CaptureGUI::CaptureGUI(sl::RESOLUTION resolution)
    : initialized(false), recording(false), exit_requested(false),
      finished_recording(false), cancelled_recording(false),
      texture_id(0), program_id(0), vao_id(0), vbo_id(0),
      window_width(1920), window_height(1080), frame_count(0), first_frame(true) {
    
    // Set up camera parameters with specified resolution
    init_params.camera_resolution = resolution;
    init_params.camera_fps = 30;
    init_params.depth_mode = sl::DEPTH_MODE::NEURAL;//_LIGHT;//NEURAL_PLUS; // Best quality for spatial mapping
    init_params.coordinate_units = sl::UNIT::METER;
    init_params.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    
    // Adjust window size based on resolution
    if (resolution == sl::RESOLUTION::HD720) {
        window_width = 1280;
        window_height = 720;
        std::cout << "CaptureGUI: Using HD720 resolution (1280x720)" << std::endl;
    } else {
        window_width = 1920;
        window_height = 1080;
        std::cout << "CaptureGUI: Using HD1080 resolution (1920x1080)" << std::endl;
    }
    
    // Clear any existing instance
    if (instance != nullptr) {
        delete instance;
    }
    instance = this;
    
    // Setup runtime parameters - Optimized for spatial mapping quality  
    runtime_params.confidence_threshold = 100; // Higher confidence for better depth quality
    runtime_params.texture_confidence_threshold = 100;
    // Enable depth processing for NEURAL_PLUS mode - essential for spatial mapping
    runtime_params.enable_depth = true;
    runtime_params.enable_fill_mode = true; // Fill mode improves depth completeness
}

CaptureGUI::CaptureGUI(sl::RESOLUTION resolution, sl::DEPTH_MODE depth_mode)
    : initialized(false), recording(false), exit_requested(false),
      finished_recording(false), cancelled_recording(false),
      texture_id(0), program_id(0), vao_id(0), vbo_id(0),
      window_width(1920), window_height(1080), frame_count(0), first_frame(true) {
    
    // Set up camera parameters with specified resolution and depth mode
    init_params.camera_resolution = resolution;
    init_params.camera_fps = 30;
    init_params.depth_mode = depth_mode;
    init_params.coordinate_units = sl::UNIT::METER;
    init_params.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    
    // Adjust window size based on resolution
    if (resolution == sl::RESOLUTION::HD720) {
        window_width = 1280;
        window_height = 720;
        std::cout << "CaptureGUI: Using HD720 resolution (1280x720)" << std::endl;
    } else {
        window_width = 1920;
        window_height = 1080;
        std::cout << "CaptureGUI: Using HD1080 resolution (1920x1080)" << std::endl;
    }
    
    // Display depth mode
    std::cout << "CaptureGUI: Using " << (depth_mode == sl::DEPTH_MODE::NEURAL ? "NEURAL" : 
                                          depth_mode == sl::DEPTH_MODE::NEURAL_PLUS ? "NEURALPLUS" : "NONE") 
              << " depth mode" << std::endl;
    
    // Clear any existing instance
    if (instance != nullptr) {
        delete instance;
    }
    instance = this;
    
    // Setup runtime parameters based on depth mode
    runtime_params.confidence_threshold = 100; // Higher confidence for better depth quality
    runtime_params.texture_confidence_threshold = 100;
    
    // Enable depth processing if not using NONE mode
    if (depth_mode != sl::DEPTH_MODE::NONE) {
        runtime_params.enable_depth = true;
        runtime_params.enable_fill_mode = true; // Fill mode improves depth completeness
        std::cout << "CaptureGUI: Depth processing enabled" << std::endl;
    } else {
        runtime_params.enable_depth = false;
        runtime_params.enable_fill_mode = false;
        std::cout << "CaptureGUI: Depth processing disabled (NONE mode)" << std::endl;
    }
}

CaptureGUI::~CaptureGUI() {
    if (recording) {
        stopRecording();
    }
    
    if (initialized) {
        if (texture_id != 0) glDeleteTextures(1, &texture_id);
        if (vbo_id != 0) glDeleteBuffers(1, &vbo_id);
        if (vao_id != 0) glDeleteVertexArrays(1, &vao_id);
        if (program_id != 0) glDeleteProgram(program_id);
    }
    
    instance = nullptr;
}

bool CaptureGUI::init(int argc, char** argv, uint64_t target_serial, const std::string& svo_path) {
    svo_output_path = svo_path;
    
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    
    // Get screen dimensions and create fullscreen window
    window_width = glutGet(GLUT_SCREEN_WIDTH);
    window_height = glutGet(GLUT_SCREEN_HEIGHT);
    
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("ZED Camera Capture");
    glutFullScreen();
    
    // Initialize GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "ERROR: glewInit failed: " << glewGetErrorString(err) << std::endl;
        return false;
    }
    
    // Setup OpenGL
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create shader program
    if (!createShaderProgram()) {
        std::cerr << "Failed to create shader program" << std::endl;
        return false;
    }
    
    // Setup camera
    init_params.camera_resolution = sl::RESOLUTION::HD1080;
    init_params.camera_fps = 30;
    init_params.depth_mode = sl::DEPTH_MODE::NEURAL;
    init_params.coordinate_units = sl::UNIT::METER;
    init_params.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    init_params.sdk_verbose = false;  // Disable SDK internal logging
    
    // Try to open camera with specific serial number
    init_params.input.setFromSerialNumber(target_serial);
    
    sl::ERROR_CODE camera_state = zed.open(init_params);
    if (camera_state != sl::ERROR_CODE::SUCCESS) {
        std::cerr << "Failed to open camera " << target_serial << ": " << camera_state << std::endl;
        return false;
    }
    
    // Get camera resolution for texture setup
    sl::CalibrationParameters calib = zed.getCameraInformation().camera_configuration.calibration_parameters;
    preview_width = calib.right_cam.image_size.width;
    preview_height = calib.right_cam.image_size.height;
    
    // Setup texture and geometry
    setupTexture();
    
    // Setup buttons
    setupButtons();
    
    // Setup GLUT callbacks
    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutPassiveMotionFunc(motionCallback);
    glutKeyboardFunc(keyboardCallback);
    
    initialized = true;
    return true;
}

bool CaptureGUI::startRecording() {
    if (recording || !initialized) {
        return false;
    }
    
    // Create output directory
    size_t last_slash = svo_output_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_path = svo_output_path.substr(0, last_slash);
        std::string mkdir_cmd = "mkdir -p " + dir_path;
        system(mkdir_cmd.c_str());
    }
    
    // Setup recording parameters
    recording_params.video_filename = sl::String(svo_output_path.c_str());
    recording_params.compression_mode = sl::SVO_COMPRESSION_MODE::H265;
    
    // Start recording
    sl::ERROR_CODE recording_state = zed.enableRecording(recording_params);
    if (recording_state != sl::ERROR_CODE::SUCCESS) {
        std::cerr << "Failed to start recording: " << recording_state << std::endl;
        return false;
    }
    
    recording = true;
    frame_count = 0;
    first_frame = true;
    recording_start_time = std::chrono::steady_clock::now();
    
    std::cout << "Started recording to: " << svo_output_path << std::endl;
    return true;
}

void CaptureGUI::stopRecording() {
    if (!recording) return;
    
    recording = false;
    zed.disableRecording();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - recording_start_time);
    
    std::cout << "Recording stopped. Frames: " << frame_count 
              << ", Duration: " << duration.count() << "s" << std::endl;
}

void CaptureGUI::run() {
    if (!initialized) return;
    
    startRecording();
    glutMainLoop();
}

void CaptureGUI::setupTexture() {
    // Create texture
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create vertex data for texture quad (left half of screen)
    float vertices[] = {
        // positions   // texture coords
        -1.0f,  1.0f,  0.0f, 0.0f,   // top left
        -1.0f, -1.0f,  0.0f, 1.0f,   // bottom left  
         0.0f, -1.0f,  1.0f, 1.0f,   // bottom right
         0.0f,  1.0f,  1.0f, 0.0f    // top right
    };
    
    unsigned int indices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };
    
    // Setup VAO/VBO
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
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void CaptureGUI::setupButtons() {
    // Calculate button dimensions and positions (right half of screen)
    float button_width = 0.8f;  // 80% of right half width
    float button_height = 0.3f; // 30% of screen height
    
    // Finish button (green) - top of right half
    finish_button.x = 0.1f;      // 10% margin from right half start
    finish_button.y = 0.1f;      // 10% from top
    finish_button.width = button_width;
    finish_button.height = button_height;
    finish_button.label = "FINISH";
    finish_button.color[0] = 0.2f;  // Green
    finish_button.color[1] = 0.8f;
    finish_button.color[2] = 0.2f;
    finish_button.hovered = false;
    finish_button.pressed = false;
    
    // Cancel button (red) - bottom of right half
    cancel_button.x = 0.1f;      // 10% margin from right half start  
    cancel_button.y = 0.6f;      // 60% from top
    cancel_button.width = button_width;
    cancel_button.height = button_height;
    cancel_button.label = "CANCEL";
    cancel_button.color[0] = 0.8f;  // Red
    cancel_button.color[1] = 0.2f;
    cancel_button.color[2] = 0.2f;
    cancel_button.hovered = false;
    cancel_button.pressed = false;
}

bool CaptureGUI::compileShader(GLuint& shader_id, GLenum type, const char* source) {
    shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source, NULL);
    glCompileShader(shader_id);
    
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader_id, 512, NULL, info_log);
        std::cerr << "Shader compilation error: " << info_log << std::endl;
        return false;
    }
    return true;
}

bool CaptureGUI::createShaderProgram() {
    GLuint vertex_shader, fragment_shader;
    
    if (!compileShader(vertex_shader, GL_VERTEX_SHADER, vertex_shader_src) ||
        !compileShader(fragment_shader, GL_FRAGMENT_SHADER, fragment_shader_src)) {
        return false;
    }
    
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);
    
    GLint success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program_id, 512, NULL, info_log);
        std::cerr << "Shader linking error: " << info_log << std::endl;
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

bool CaptureGUI::isPointInButton(const ButtonState& button, int x, int y) {
    // Convert screen coordinates to normalized device coordinates for right half
    float norm_x = (2.0f * x / window_width) - 1.0f;  // -1 to 1
    float norm_y = 1.0f - (2.0f * y / window_height); // 1 to -1 (flip Y)
    
    // Convert to right half coordinates (0 to 1 range)
    if (norm_x < 0.0f) return false; // Not in right half
    float right_x = norm_x; // 0 to 1 in right half
    
    return (right_x >= button.x && right_x <= button.x + button.width &&
            norm_y >= button.y && norm_y <= button.y + button.height);
}

void CaptureGUI::updateCamera() {
    if (!initialized) return;
    
    PROFILE_SCOPE("CaptureGUI::updateCamera");
    
    {
        PROFILE_SCOPE("CaptureGUI::grab");
        sl::ERROR_CODE grab_status = zed.grab(runtime_params);
        
        if (grab_status == sl::ERROR_CODE::SUCCESS) {
            {
                PROFILE_SCOPE("CaptureGUI::retrieveImage");
                // Retrieve right camera image - try GPU memory first
                zed.retrieveImage(image_right, sl::VIEW::RIGHT, sl::MEM::CPU);
            }
            {
                PROFILE_SCOPE("CaptureGUI::updateTexture");
                updateTexture();
            }
        
        if (recording) {
            frame_count++;
            
            // Log frame timing information
            auto current_time = std::chrono::steady_clock::now();
            if (first_frame) {
                std::cout << "Frame " << frame_count << ": 0 ms (first frame)" << std::endl;
                first_frame = false;
            } else {
                auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - last_frame_time).count();
                double fps = frame_count / (std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - recording_start_time).count() / 1000.0);
                std::cout << "Frame " << frame_count << ": " << time_since_last 
                          << " ms, FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
                
                // Print performance report every 10 seconds
                if (frame_count % 300 == 0) { // ~10 seconds at 30fps
                    PerformanceProfiler::getInstance().printReport();
                }
            }
            last_frame_time = current_time;
        }
        } else {
            // Add diagnostic info for grab failures
            if (grab_status != sl::ERROR_CODE::SUCCESS) {
                std::cout << "Grab failed: " << grab_status << std::endl;
            }
        }
    }
}

void CaptureGUI::updateTexture() {
    if (texture_id == 0 || !image_right.isInit()) return;
    
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                 image_right.getWidth(), image_right.getHeight(), 
                 0, GL_BGRA, GL_UNSIGNED_BYTE, image_right.getPtr<sl::uchar1>());
}

void CaptureGUI::drawPreview() {
    // Use shader program
    glUseProgram(program_id);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // Draw quad
    glBindVertexArray(vao_id);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void CaptureGUI::drawButtons() {
    // Simple colored rectangles for buttons
    glUseProgram(0);  // Use fixed function pipeline
    glDisable(GL_TEXTURE_2D);
    
    auto drawButton = [this](const ButtonState& button) {
        float brightness = button.hovered ? 1.2f : 1.0f;
        if (button.pressed) brightness = 0.8f;
        
        glColor3f(button.color[0] * brightness, 
                 button.color[1] * brightness, 
                 button.color[2] * brightness);
        
        // Convert button coordinates to screen coordinates
        float x1 = button.x;
        float y1 = button.y; 
        float x2 = button.x + button.width;
        float y2 = button.y + button.height;
        
        glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
        
        // Draw button border
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
    };
    
    drawButton(finish_button);
    drawButton(cancel_button);
    
    glEnable(GL_TEXTURE_2D);
}

void CaptureGUI::handleButtonClick(int x, int y) {
    if (isPointInButton(finish_button, x, y)) {
        std::cout << "Finish button clicked - saving recording..." << std::endl;
        finished_recording = true;
        exit_requested = true;
        stopRecording();
        glutLeaveMainLoop();
    } else if (isPointInButton(cancel_button, x, y)) {
        std::cout << "Cancel button clicked - discarding recording..." << std::endl;
        cancelled_recording = true;
        exit_requested = true;
        stopRecording();
        
        // Delete the SVO file
        if (remove(svo_output_path.c_str()) == 0) {
            std::cout << "Deleted recording file: " << svo_output_path << std::endl;
        }
        
        glutLeaveMainLoop();
    }
}

void CaptureGUI::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Update camera feed
    updateCamera();
    
    // Draw preview on left half
    drawPreview();
    
    // Draw buttons on right half  
    drawButtons();
    
    // Display recording status
    if (recording) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - recording_start_time);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        // Simple text display - in a real implementation you'd use a text rendering library
        // For now, just show recording indicator with a simple rectangle
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);  // Red recording indicator
        glVertex2f(0.05f, 0.9f);
        glVertex2f(0.15f, 0.9f); 
        glVertex2f(0.15f, 0.95f);
        glVertex2f(0.05f, 0.95f);
        glEnd();
    }
    
    glutSwapBuffers();
}

// Static callback functions
void CaptureGUI::displayCallback() {
    if (instance) {
        instance->render();
    }
}

void CaptureGUI::reshapeCallback(int width, int height) {
    if (instance) {
        instance->window_width = width;
        instance->window_height = height;
        glViewport(0, 0, width, height);
    }
}

void CaptureGUI::mouseCallback(int button, int state, int x, int y) {
    if (!instance) return;
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        instance->handleButtonClick(x, y);
    }
}

void CaptureGUI::motionCallback(int x, int y) {
    if (!instance) return;
    
    // Update button hover states
    instance->finish_button.hovered = instance->isPointInButton(instance->finish_button, x, y);
    instance->cancel_button.hovered = instance->isPointInButton(instance->cancel_button, x, y);
    
    glutPostRedisplay();
}

void CaptureGUI::keyboardCallback(unsigned char key, int x, int y) {
    if (!instance) return;
    
    if (key == 27 || key == 'q' || key == 'Q') { // ESC or Q key
        std::cout << "Exiting via keyboard..." << std::endl;
        instance->cancelled_recording = true;
        instance->exit_requested = true;
        instance->stopRecording();
        
        // Delete the SVO file
        if (remove(instance->svo_output_path.c_str()) == 0) {
            std::cout << "Deleted recording file: " << instance->svo_output_path << std::endl;
        }
        
        glutLeaveMainLoop();
    }
}
