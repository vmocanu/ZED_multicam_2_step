#ifndef __CAPTURE_GUI_HDR__
#define __CAPTURE_GUI_HDR__

#include <sl/Camera.hpp>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <string>
#include <thread>
#include <chrono>
#include <functional>

struct ButtonState {
    float x, y, width, height;
    bool hovered;
    bool pressed;
    std::string label;
    float color[3]; // RGB color
};

class CaptureGUI {
public:
    CaptureGUI();
    CaptureGUI(sl::RESOLUTION resolution);
    CaptureGUI(sl::RESOLUTION resolution, sl::DEPTH_MODE depth_mode);
    ~CaptureGUI();

    // Initialize GUI and start camera
    bool init(int argc, char** argv, uint64_t target_serial, const std::string& svo_path);
    
    // Start recording
    bool startRecording();
    
    // Stop recording 
    void stopRecording();
    
    // Run the GUI main loop
    void run();
    
    // Check if GUI should exit
    bool shouldExit() const { return exit_requested; }
    
    // Check if recording finished successfully
    bool isFinished() const { return finished_recording; }
    
    // Check if recording was cancelled
    bool isCancelled() const { return cancelled_recording; }
    
    // Get the SVO output path
    const std::string& getSVOPath() const { return svo_output_path; }
    
private:
    // OpenGL/GLUT callbacks
    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void mouseCallback(int button, int state, int x, int y);
    static void motionCallback(int x, int y);
    static void keyboardCallback(unsigned char key, int x, int y);
    
    // Instance methods
    void render();
    void updateCamera();
    void drawPreview();
    void drawButtons();
    void handleButtonClick(int x, int y);
    void setupTexture();
    void updateTexture();
    
    // Camera and recording
    sl::Camera zed;
    sl::InitParameters init_params;
    sl::RecordingParameters recording_params;
    sl::Mat image_left, image_right;
    sl::RuntimeParameters runtime_params;
    
    // OpenGL resources
    GLuint texture_id;
    GLuint program_id;
    GLuint vao_id, vbo_id;
    
    // GUI state
    static CaptureGUI* instance;
    ButtonState finish_button;
    ButtonState cancel_button;
    int window_width, window_height;
    int preview_width, preview_height;
    
    // Control flags
    bool initialized;
    bool recording;
    bool exit_requested;
    bool finished_recording;
    bool cancelled_recording;
    
    // Recording info
    std::string svo_output_path;
    int frame_count;
    
    // Frame timing for logging
    std::chrono::steady_clock::time_point last_frame_time;
    std::chrono::steady_clock::time_point recording_start_time;
    bool first_frame;
    
    // Shader sources
    static const char* vertex_shader_src;
    static const char* fragment_shader_src;
    
    // Helper methods
    bool compileShader(GLuint& shader_id, GLenum type, const char* source);
    bool createShaderProgram();
    void setupButtons();
    bool isPointInButton(const ButtonState& button, int x, int y);
    void renderText(float x, float y, const std::string& text, float scale = 1.0f);
};

#endif // __CAPTURE_GUI_HDR__
