#include "ClientPublisher.hpp"

ClientPublisher::ClientPublisher() : running(false)
{
    init_parameters.camera_resolution = sl::RESOLUTION::HD1080;
    init_parameters.camera_fps = 30;
    // Always use NEURAL_PLUS for fusion mode (best quality for 3D reconstruction)
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_PLUS;
}

ClientPublisher::ClientPublisher(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE /*depth_mode*/) : running(false)
{
    init_parameters.camera_resolution = resolution;
    init_parameters.camera_fps = fps;
    // Always use NEURAL_PLUS for fusion mode (best quality for 3D reconstruction)
    // Depth mode parameter is ignored - NEURAL_PLUS is hardcoded
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_PLUS;
}

ClientPublisher::~ClientPublisher()
{
    zed.close();
}

bool ClientPublisher::open(sl::InputType input) {
    // already running
    if (runner.joinable())
        return false;

    init_parameters.input = input;
    if (input.getType() == sl::InputType::INPUT_TYPE::SVO_FILE)
        init_parameters.svo_real_time_mode = true;  // Keep real-time for fusion to work properly
    init_parameters.coordinate_units = sl::UNIT::METER;
    init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    init_parameters.sdk_verbose = false;  // Disable SDK internal logging
    auto state = zed.open(init_parameters);
    if (state != sl::ERROR_CODE::SUCCESS)
    {
        std::cout << "Error: " << state << std::endl;
        return false;
    }

    sl::PositionalTrackingParameters positional_tracking_parameters; 
    state = zed.enablePositionalTracking(positional_tracking_parameters);
    if (state != sl::ERROR_CODE::SUCCESS)
    {
        std::cout << "Error: " << state << std::endl;
        return false;
    }
    
    return true;
}

void ClientPublisher::start()
{
    if (zed.isOpened()) {
        running = true;
        // the camera should stream its data so the fusion can subscibe to it to gather the detected body and others metadata needed for the process.
        zed.startPublishing();
        // the thread can start to process the camera grab in background
        runner = std::thread(&ClientPublisher::work, this);
    }
}

void ClientPublisher::stop()
{
    running = false;
    if (runner.joinable())
        runner.join();
    zed.close();
}

void ClientPublisher::work()
{
    // In this sample we use a dummy thread to process the ZED data.
    // you can replace it by your own application and use the ZED like you use to, retrieve its images, depth, sensors data and so on.
    // As long as you call the grab function the camera will be able to seamlessly transmit the data to the fusion module.

    // Setup runtime parameters
    sl::RuntimeParameters runtime_parameters;
    // Use low depth confidence to avoid introducing noise in the constructed model
    runtime_parameters.confidence_threshold = 50;

    while (running) {
        if (zed.grab(runtime_parameters) == sl::ERROR_CODE::SUCCESS) {
            // Frame grabbed successfully - fusion module will retrieve data via subscription
            
            // Track frame timestamps for FPS calculation if enabled
            if (fps_tracking_enabled) {
                std::lock_guard<std::mutex> lock(fps_mutex);
                frame_timestamps.push_back(std::chrono::steady_clock::now());
                // Keep only the last MAX_FRAME_TIMESTAMPS
                if (frame_timestamps.size() > MAX_FRAME_TIMESTAMPS) {
                    frame_timestamps.pop_front();
                }
            }
            
            // Retrieve image for preview if enabled
            if (retrieve_images) {
                sl::Mat temp_image;
                if (zed.retrieveImage(temp_image, sl::VIEW::LEFT, sl::MEM::CPU) == sl::ERROR_CODE::SUCCESS) {
                    std::lock_guard<std::mutex> lock(image_mutex);
                    if (!latest_left_image.isInit()) {
                        latest_left_image.alloc(temp_image.getResolution(), sl::MAT_TYPE::U8_C4, sl::MEM::CPU);
                    }
                    temp_image.copyTo(latest_left_image);
                }
            }
        }
    }
}

void ClientPublisher::setStartSVOPosition(unsigned pos) {
    zed.setSVOPosition(pos);
}


