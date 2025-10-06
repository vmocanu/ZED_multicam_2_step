#ifndef __LIVE_PREVIEW_HDR__
#define __LIVE_PREVIEW_HDR__

#include <sl/Camera.hpp>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

class LivePreview {
public:
    LivePreview(sl::Resolution resolution);
    ~LivePreview();

    // Start the preview window in a separate thread
    bool start(int argc, char** argv);
    
    // Stop the preview window
    void stop();
    
    // Update the image to be displayed (thread-safe)
    void updateImage(const sl::Mat& image);
    
    // Check if preview is running
    bool isRunning() const { return running; }
    
private:
    // OpenGL/GLUT callbacks (must be static)
    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void keyboardCallback(unsigned char key, int x, int y);
    static void idleCallback();
    
    // Instance methods
    void render();
    void updateTexture();
    void setupGL();
    void setupTexture();
    bool createShaderProgram();
    bool compileShader(GLuint& shader_id, GLenum type, const char* source);
    
    // Thread function
    void renderLoop(int argc, char** argv);
    
    // OpenGL resources
    GLuint texture_id;
    GLuint program_id;
    GLuint vao_id, vbo_id;
    
    // Window dimensions
    int window_width, window_height;
    
    // Image buffer (thread-safe)
    std::mutex image_mutex;
    sl::Mat current_image;
    bool image_updated;
    
    // Control flags
    std::atomic<bool> running;
    std::atomic<bool> stop_requested;
    std::thread render_thread;
    
    // Static instance for callbacks
    static LivePreview* instance;
    
    // Shader sources
    static const char* vertex_shader_src;
    static const char* fragment_shader_src;
};

#endif // __LIVE_PREVIEW_HDR__

