///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2025, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// ZED include
#include "ClientPublisher.hpp"
#include "CaptureRecorder.hpp"
#include "CaptureGUI.hpp"
#include "GLViewer.hpp"
#include "utils.hpp"
#include <signal.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <sys/stat.h>
#include <memory>

#define BUILD_MESH 1

// Application modes
enum class AppMode {
    CAPTURE,     // Record SVO files from each camera
    FUSION,      // Perform fusion on recorded SVO files
    LIVE_FUSION, // Original mode: live capture + fusion
    RECONSTRUCT  // Perform spatial mapping on single SVO file
};

enum class CameraSelection {
    ZEDX_MINI,  // Only use ZED X Mini (SN: 57709210)
    ZEDX,       // Only use ZED X (SN: 47797222) 
    BOTH        // Use both cameras (default)
};

void print_usage() {
    std::cout << "ZED Spatial Mapping Fusion Application" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion <mode> <config_file> [options]        (for multi-camera)" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion capture --camera <zedx|zedx-mini> [options]  (single camera)" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion reconstruct <svo_file>               (reconstruct mode)" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  capture    Record SVO files from each camera" << std::endl;
    std::cout << "  fusion     Perform fusion on recorded SVO files" << std::endl;
    std::cout << "  live       Live capture + fusion (original mode)" << std::endl;
    std::cout << "  reconstruct <svo_file>  Perform spatial mapping on single SVO file" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  config_file    JSON configuration file (required for multi-camera modes)" << std::endl;
    std::cout << std::endl;
    std::cout << "Capture Mode Options:" << std::endl;
    std::cout << "  --duration <seconds>    Recording duration (default: 30)" << std::endl;
    std::cout << "  --output-dir <path>     Output directory for SVO files (default: ./svo_recordings)" << std::endl;
    std::cout << "  --resolution <res>      Camera resolution: 720 or 1080 (default: 1080)" << std::endl;
    std::cout << "  --depth-mode <mode>     Depth processing: neural_light, neural, neural_plus (default: neural_light)" << std::endl;
    std::cout << "  --camera <selection>    Camera selection: zedx-mini, zedx, both (default: both)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Multi-camera modes (require config file)" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion capture config.json --duration 60" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion capture config.json --camera both" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion fusion config.json" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion live config.json" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Single camera modes (no config file needed)" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion capture --camera zedx --depth-mode neural_plus --duration 30" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion capture --camera zedx-mini --resolution 720" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Reconstruct mode" << std::endl;
    std::cout << "  ./ZED_SpatialMappingFusion reconstruct camera_47797222.svo" << std::endl;
}

AppMode parse_mode(const std::string& mode_str) {
    if (mode_str == "capture") return AppMode::CAPTURE;
    if (mode_str == "fusion") return AppMode::FUSION;
    if (mode_str == "live") return AppMode::LIVE_FUSION;
    if (mode_str == "reconstruct") return AppMode::RECONSTRUCT;
    return AppMode::LIVE_FUSION; // default
}

CameraSelection parse_camera_selection(const std::string& camera_str) {
    if (camera_str == "zedx-mini") return CameraSelection::ZEDX_MINI;
    if (camera_str == "zedx") return CameraSelection::ZEDX;
    if (camera_str == "both") return CameraSelection::BOTH;
    return CameraSelection::BOTH; // default
}

std::string get_camera_name(uint64_t serial_number) {
    if (serial_number == 57709210) return "ZED-X-Mini";
    if (serial_number == 47797222) return "ZED-X";
    return "Camera-" + std::to_string(serial_number);
}

// Forward declarations
int run_reconstruct_mode(const std::string& svo_file_path);

// Global flag for signal handling
volatile bool exit_requested = false;

void signal_handler(int signal) {
    std::cout << std::endl << "Received signal " << signal << ". Stopping gracefully..." << std::endl;
    exit_requested = true;
}

// Capture mode implementation  
int run_capture_mode(const std::vector<sl::FusionConfiguration>& configurations, 
                     int recording_duration, const std::string& output_dir,
                     sl::RESOLUTION resolution, sl::DEPTH_MODE depth_mode,
                     CameraSelection camera_selection,
                     int argc = 0, char **argv = nullptr) {
    
    // Filter cameras based on selection
    std::vector<sl::FusionConfiguration> filtered_configs;
    for (const auto& config : configurations) {
        bool include_camera = false;
        switch (camera_selection) {
            case CameraSelection::ZEDX_MINI:
                include_camera = (config.serial_number == 57709210);
                break;
            case CameraSelection::ZEDX:
                include_camera = (config.serial_number == 47797222);
                break;
            case CameraSelection::BOTH:
                include_camera = true;
                break;
        }
        if (include_camera) {
            filtered_configs.push_back(config);
        }
    }

    if (filtered_configs.empty()) {
        std::cout << "Error: No cameras found matching selection criteria!" << std::endl;
        std::cout << "Available cameras:" << std::endl;
        for (const auto& config : configurations) {
            std::cout << "  " << get_camera_name(config.serial_number) << " (SN: " << config.serial_number << ")" << std::endl;
        }
        return EXIT_FAILURE;
    }

    std::cout << "=== CAPTURE MODE ===" << std::endl;
    std::cout << "Recording duration: " << recording_duration << " seconds" << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;
    std::cout << "Resolution: " << (resolution == sl::RESOLUTION::HD720 ? "HD720 (1280x720)" : "HD1080 (1920x1080)") << std::endl;
    std::cout << "Depth mode: " << (depth_mode == sl::DEPTH_MODE::NEURAL_LIGHT ? "NEURAL_LIGHT" : 
                                    depth_mode == sl::DEPTH_MODE::NEURAL ? "NEURAL" : "NEURAL_PLUS") << std::endl;
    std::cout << "Camera selection: " << (camera_selection == CameraSelection::ZEDX_MINI ? "ZED-X-Mini only" :
                                          camera_selection == CameraSelection::ZEDX ? "ZED-X only" : "Both cameras") << std::endl;
    std::cout << "Active cameras: " << filtered_configs.size() << std::endl;
    for (const auto& config : filtered_configs) {
        std::cout << "  " << get_camera_name(config.serial_number) << " (SN: " << config.serial_number << ")" << std::endl;
    }
    std::cout << std::endl;

    // Check if we have the special device (47797222) that should use GUI
    const uint64_t GUI_DEVICE_ID = 47797222;
    bool has_gui_device = false;
    sl::FusionConfiguration gui_device_config;

    for (const auto& conf : filtered_configs) {
        if (conf.serial_number == GUI_DEVICE_ID) {
            has_gui_device = true;
            gui_device_config = conf;
            break;
        }
    }
    
    // If we have the GUI device, use the special GUI capture mode
    if (has_gui_device && argc > 0 && argv != nullptr) {
        std::cout << "Found special device " << GUI_DEVICE_ID << " - using GUI capture mode" << std::endl;
        
        // Generate SVO filename for GUI device
        std::string svo_filename = "camera_" + std::to_string(GUI_DEVICE_ID) + ".svo";
        std::string svo_path = output_dir + "/" + svo_filename;
        
        // Create and run GUI capture
        CaptureGUI gui_capture(resolution, depth_mode);
        if (!gui_capture.init(argc, argv, GUI_DEVICE_ID, svo_path)) {
            std::cout << "Failed to initialize GUI capture for device " << GUI_DEVICE_ID << std::endl;
            // Fall back to regular capture mode
        } else {
            // Run GUI capture
            gui_capture.run();
            
            // Check result
            if (gui_capture.isFinished()) {
                std::cout << "GUI capture completed successfully!" << std::endl;
                std::cout << "Recorded to: " << gui_capture.getSVOPath() << std::endl;
                return EXIT_SUCCESS;
            } else if (gui_capture.isCancelled()) {
                std::cout << "GUI capture was cancelled by user." << std::endl;
                return EXIT_FAILURE;
            } else {
                std::cout << "GUI capture ended unexpectedly." << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    
    // Regular capture mode for all devices (or fallback)
    std::cout << "Using standard capture mode" << std::endl;

    // Create output directory (simple approach for C++14 compatibility)
    std::string mkdir_cmd = "mkdir -p " + output_dir;
    system(mkdir_cmd.c_str());
    
    // Setup signal handling for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create recorders for each camera using unique_ptr to avoid move/copy issues
    std::vector<std::unique_ptr<CaptureRecorder>> recorders;
    std::vector<std::string> svo_paths;
    
    for (size_t i = 0; i < filtered_configs.size(); ++i) {
        recorders.push_back(std::make_unique<CaptureRecorder>(resolution, 30, depth_mode)); // Use specified resolution and depth mode
        
        // Set camera name for logging
        recorders[i]->setCameraName(get_camera_name(filtered_configs[i].serial_number));
        
        // Generate SVO filename based on serial number
        std::string svo_filename = "camera_" + std::to_string(filtered_configs[i].serial_number) + ".svo";
        std::string svo_path = output_dir + "/" + svo_filename;
        svo_paths.push_back(svo_path);
    }
    
    // Start recording on all cameras
    int active_recordings = 0;
    for (size_t i = 0; i < filtered_configs.size(); ++i) {
        const auto& conf = filtered_configs[i];
        
        // Only start recording for cameras that should run locally
        if (conf.communication_parameters.getType() == sl::CommunicationParameters::COMM_TYPE::INTRA_PROCESS) {
            std::cout << "Starting recording for " << get_camera_name(conf.serial_number) 
                      << " (SN: " << conf.serial_number << ")..." << std::flush;
            
            if (recorders[i]->startRecording(conf.input_type, svo_paths[i], conf.serial_number)) {
                active_recordings++;
                std::cout << " OK" << std::endl;
            } else {
                std::cout << " FAILED" << std::endl;
            }
        }
    }
    
    if (active_recordings == 0) {
        std::cout << "No cameras started recording. Exiting." << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << std::endl << "Recording started on " << active_recordings << " camera(s)." << std::endl;
    std::cout << "Press Ctrl+C to stop recording early." << std::endl;
    std::cout << std::endl;
    
    // Wait for recording duration or user interruption
    auto start_time = std::chrono::steady_clock::now();
    while (!exit_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
            
        if (elapsed >= recording_duration) {
            std::cout << "Recording duration reached. Stopping..." << std::endl;
            break;
        }
        
        // Print progress every 10 seconds
        if (elapsed % 10 == 0 && elapsed > 0) {
            std::cout << "Recording progress: " << elapsed << "/" << recording_duration << " seconds" << std::endl;
        }
    }
    
    // Stop all recordings
    std::cout << "Stopping recordings..." << std::endl;
    for (auto& recorder : recorders) {
        if (recorder->isRecording()) {
            recorder->stopRecording();
        }
    }
    
    // Print summary
    std::cout << std::endl << "=== RECORDING SUMMARY ===" << std::endl;
    for (size_t i = 0; i < configurations.size(); ++i) {
        // Check if file exists using stat (C++14 compatible)
        struct stat buffer;
        if (stat(svo_paths[i].c_str(), &buffer) == 0) {
            long file_size = buffer.st_size;
            std::cout << "Camera " << configurations[i].serial_number << ": " 
                      << svo_paths[i] << " (" << (file_size / 1024 / 1024) << " MB)" << std::endl;
        }
    }
    
    std::cout << std::endl << "Capture completed successfully!" << std::endl;
    std::cout << "To run fusion on recorded data, use:" << std::endl;
    std::cout << "./ZED_SpatialMappingFusion fusion <config_file>" << std::endl;
    
    return EXIT_SUCCESS;
}

// Reconstruct mode - perform spatial mapping on a single SVO file
int run_reconstruct_mode(const std::string& svo_file_path) {
    std::cout << "=== RECONSTRUCT MODE ===" << std::endl;
    std::cout << "SVO file: " << svo_file_path << std::endl;

    // Check if SVO file exists
    std::ifstream svo_file(svo_file_path);
    if (!svo_file.good()) {
        std::cout << "Error: SVO file not found: " << svo_file_path << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize ZED camera with SVO file
    sl::Camera zed;
    sl::InitParameters init_params;
    init_params.input.setFromSVOFile(sl::String(svo_file_path.c_str()));
    // Note: Resolution will be read from SVO file automatically
    init_params.depth_mode = sl::DEPTH_MODE::NEURAL_PLUS; // Use best quality for reconstruction
    init_params.coordinate_units = sl::UNIT::METER;
    init_params.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    
    auto returned_state = zed.open(init_params);
    if (returned_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Error opening SVO file: " << returned_state << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "SVO file opened successfully" << std::endl;
    
    // Get camera information including resolution from the initialization parameters
    auto camera_info = zed.getCameraInformation();
    auto init_params_used = zed.getInitParameters();
    sl::RESOLUTION svo_resolution = init_params_used.camera_resolution;
    
    std::cout << "SVO Resolution: " << (svo_resolution == sl::RESOLUTION::HD720 ? "HD720 (1280x720)" : 
                                       svo_resolution == sl::RESOLUTION::HD1080 ? "HD1080 (1920x1080)" :
                                       svo_resolution == sl::RESOLUTION::HD2K ? "HD2K (2208x1242)" : "Other") << std::endl;
    
    // Get total frame count
    int total_frames = zed.getSVONumberOfFrames();
    std::cout << "Total frames in SVO: " << total_frames << std::endl;
    
    // Enable spatial mapping
    sl::SpatialMappingParameters mapping_parameters;
    mapping_parameters.resolution_meter = 0.05f;  // 5cm resolution
    mapping_parameters.max_memory_usage = 2048;   // 2GB
    mapping_parameters.save_texture = true;       // Save texture information
    
    auto spatial_mapping_state = zed.enableSpatialMapping(mapping_parameters);
    if (spatial_mapping_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Error enabling spatial mapping: " << spatial_mapping_state << std::endl;
        zed.close();
        return EXIT_FAILURE;
    }
    
    std::cout << "Spatial mapping enabled" << std::endl;
    std::cout << "Processing frames..." << std::endl;
    
    // Process all frames in the SVO
    sl::RuntimeParameters runtime_params;
    runtime_params.confidence_threshold = 50;
    runtime_params.texture_confidence_threshold = 100;
    
    int processed_frames = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto grab_status = zed.grab(runtime_params);
        if (grab_status == sl::ERROR_CODE::END_OF_SVOFILE_REACHED) {
            std::cout << "End of SVO file reached" << std::endl;
            break;
        }
        
        if (grab_status == sl::ERROR_CODE::SUCCESS) {
            processed_frames++;
            
            // Update spatial mapping (processing happens automatically)
            
            // Progress indicator
            if (processed_frames % 30 == 0 || processed_frames == total_frames) {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double fps = processed_frames / (elapsed + 0.001);
                
                std::cout << "Frame " << processed_frames << "/" << total_frames 
                          << " (" << std::fixed << std::setprecision(1) << (100.0 * processed_frames / total_frames) << "%) "
                          << "Processing FPS: " << std::setprecision(1) << fps << std::endl;
            }
        } else {
            std::cout << "Frame grab failed: " << grab_status << std::endl;
        }
    }
    
    std::cout << "Spatial mapping processing completed" << std::endl;
    std::cout << "Extracting mesh..." << std::endl;
    
    // Extract mesh
    sl::Mesh mesh;
    auto mesh_state = zed.extractWholeSpatialMap(mesh);
    if (mesh_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Error extracting mesh: " << mesh_state << std::endl;
        zed.close();
        return EXIT_FAILURE;
    }
    
    // Save mesh
    std::string mesh_filename = svo_file_path.substr(0, svo_file_path.find_last_of('.')) + "_mesh.ply";
    bool save_state = mesh.save(sl::String(mesh_filename.c_str()));
    if (!save_state) {
        std::cout << "Error saving mesh" << std::endl;
        zed.close();
        return EXIT_FAILURE;
    }
    
    std::cout << "Mesh saved to: " << mesh_filename << std::endl;
    std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "Triangles: " << mesh.triangles.size() << std::endl;
    
    zed.close();
    return EXIT_SUCCESS;
}

// Fusion mode implementation (original logic)
int run_fusion_mode(const std::vector<sl::FusionConfiguration>& configurations, 
                    const sl::COORDINATE_SYSTEM& COORDINATE_SYSTEM, 
                    const sl::UNIT& UNIT, 
                    int argc, char **argv) {
    
    std::cout << "=== FUSION MODE ===" << std::endl;
    std::cout << "Number of cameras/SVO files: " << configurations.size() << std::endl;
    std::cout << std::endl;

      // Check if the ZED camera should run within the same process or if they are running on the edge.
      // Note: Camera parameters (resolution, 30fps, depth_mode) are configurable via command line
    std::vector<ClientPublisher> clients(configurations.size());
    int id_ = 0;
    std::map<int, std::string> svo_files;
    for (auto conf: configurations) {
        // if the ZED camera should run locally, then start a thread to handle it
        if(conf.communication_parameters.getType() == sl::CommunicationParameters::COMM_TYPE::INTRA_PROCESS){
            std::cout << "Try to open ZED " <<conf.serial_number << ".." << std::flush;
            auto state = clients[id_].open(conf.input_type);

            if (!state) {
                std::cerr << "Could not open ZED: " << conf.input_type.getConfiguration() << ". Skipping..." << std::endl;
                continue;
            }

            if (conf.input_type.getType() == sl::InputType::INPUT_TYPE::SVO_FILE)
                svo_files.insert(std::make_pair(id_, conf.input_type.getConfiguration()));

            std::cout << ". ready !" << std::endl;

            id_++;
        }
    }

    // Synchronize SVO files in SVO mode
    bool enable_svo_sync = (svo_files.size() > 1);
    if (enable_svo_sync) {
        std::cout << "Starting SVO sync process..." << std::endl;
        std::map<int, int> cam_idx_to_svo_frame_idx = syncDATA(svo_files);

        for (auto &it : cam_idx_to_svo_frame_idx) {
            std::cout << "Setting camera " << it.first << " to frame " << it.second << std::endl;
            clients[it.first].setStartSVOPosition(it.second);
        }
    }

    // start camera threads
    for (auto &it: clients)
        it.start();

    // Now that the ZED camera are running, we need to initialize the fusion module
    sl::InitFusionParameters init_params;
    // Note: InitFusionParameters doesn't have camera-specific settings
    // Camera settings are configured individually in ClientPublisher
    init_params.coordinate_units = UNIT;
    init_params.coordinate_system = COORDINATE_SYSTEM;
    init_params.verbose = true;

    // create and initialize it
    sl::Fusion fusion;
    auto state = fusion.init(init_params);
    if(state != sl::FUSION_ERROR_CODE::SUCCESS){
        std::cout<<"ERROR Init "<<state<<std::endl;
        return 1;
    }

    // subscribe to every cameras of the setup to internally gather their data
    std::vector<sl::CameraIdentifier> cameras;
    for (auto& it : configurations) {
        sl::CameraIdentifier uuid(it.serial_number);
        // to subscribe to a camera you must give its serial number, the way to communicate with it (shared memory or local network), and its world pose in the setup.        
        auto state = fusion.subscribe(uuid, it.communication_parameters, it.pose, it.override_gravity);
        if (state != sl::FUSION_ERROR_CODE::SUCCESS)
            std::cout << "Unable to subscribe to " << std::to_string(uuid.sn) << " . " << state << std::endl;
        else
            cameras.push_back(uuid);
    }

    // check that at least one camera is connected
    if (cameras.empty()) {
        std::cout << "no connections " << std::endl;
        return EXIT_FAILURE;
    }

    sl::PositionalTrackingFusionParameters positional_tracking_param;
    state = fusion.enablePositionalTracking(positional_tracking_param);
    if(state != sl::FUSION_ERROR_CODE::SUCCESS){
        std::cout<<"ERROR PositionalTracking "<<state<<std::endl;
        return 1;
    }

    // creation of a 3D viewer
    GLViewer viewer;
    viewer.init(argc, argv);

    sl::SpatialMappingFusionParameters spatial_mapping_parameters;
#if BUILD_MESH
    spatial_mapping_parameters.map_type = sl::SpatialMappingParameters::SPATIAL_MAP_TYPE::MESH;
    sl::Mesh map;
#else
    spatial_mapping_parameters.map_type = sl::SpatialMappingParameters::SPATIAL_MAP_TYPE::FUSED_POINT_CLOUD;
    sl::FusedPointCloud map;
#endif

    // Set mapping range, it will set the resolution accordingly (a higher range, a lower resolution)
    spatial_mapping_parameters.set(sl::SpatialMappingParameters::MAPPING_RANGE::SHORT);
    spatial_mapping_parameters.set(sl::SpatialMappingParameters::MAPPING_RESOLUTION::HIGH);
    // Request partial updates only (only the last updated chunks need to be re-draw)
    spatial_mapping_parameters.use_chunk_only = true;
    // Stability counter defines how many times a stable 3D points should be seen before it is integrated into the spatial mapping
    spatial_mapping_parameters.stability_counter = 4;

    state = fusion.enableSpatialMapping(spatial_mapping_parameters);
    if(state != sl::FUSION_ERROR_CODE::SUCCESS){
        std::cout<<"ERROR Spatial Mapping "<<state<<std::endl;
        return 1;
    }

    sl::Timestamp last_update = 0;
    bool wait_for_mesh = false;    

    auto ts = sl::getCurrentTimeStamp();
    // run the fusion as long as the viewer is available.
    while (viewer.isAvailable()) {
        // run the fusion process (which gather data from all camera, sync them and process them)
        
        if (fusion.process() == sl::FUSION_ERROR_CODE::SUCCESS) {

            std::cout<<"New frame at TS: "<<(sl::getCurrentTimeStamp().getMilliseconds()-ts.getMilliseconds())<<std::endl;
            ts = sl::getCurrentTimeStamp();
            if(!wait_for_mesh && (ts.getMilliseconds() - last_update.getMilliseconds() > 100 )){
                fusion.requestSpatialMapAsync();
                wait_for_mesh =true;
            }

            if(wait_for_mesh && fusion.getSpatialMapRequestStatusAsync() == sl::FUSION_ERROR_CODE::SUCCESS){
                fusion.retrieveSpatialMapAsync(map);
                // update the 3D view
                viewer.updateMap(map);
                wait_for_mesh = false;
                last_update = ts;
            }
        }
    }

    viewer.exit();

    map.save("MyMap.ply", sl::MESH_FILE_FORMAT::PLY);

    for (auto &it: clients)
        it.stop();

    fusion.close();

    return EXIT_SUCCESS;
}

// Helper function to create SVO-based configuration
std::vector<sl::FusionConfiguration> create_svo_configurations(
    const std::vector<sl::FusionConfiguration>& original_configs, 
    const std::string& svo_directory) {
    
    std::vector<sl::FusionConfiguration> svo_configs;
    
    for (const auto& config : original_configs) {
        sl::FusionConfiguration svo_config = config;
        
        // Create SVO file path
        std::string svo_filename = "camera_" + std::to_string(config.serial_number) + ".svo";
        std::string svo_path = svo_directory + "/" + svo_filename;
        
        // Check if SVO file exists using stat (C++14 compatible)
        struct stat buffer;
        if (stat(svo_path.c_str(), &buffer) == 0) {
            svo_config.input_type.setFromSVOFile(sl::String(svo_path.c_str()));
            svo_configs.push_back(svo_config);
            std::cout << "Found SVO file for camera " << config.serial_number << ": " << svo_path << std::endl;
        } else {
            std::cout << "Warning: SVO file not found for camera " << config.serial_number << ": " << svo_path << std::endl;
        }
    }
    
    return svo_configs;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Parse command line arguments
    AppMode app_mode = parse_mode(argv[1]);
    std::string json_config = "";
    int arg_offset = 2;  // Default offset for config file
    
    // Parse optional arguments
    int recording_duration = 30;  // seconds
    std::string output_dir = "./svo_recordings";
    sl::RESOLUTION resolution = sl::RESOLUTION::HD1080;  // default to 1080p
    sl::DEPTH_MODE depth_mode = sl::DEPTH_MODE::NEURAL_LIGHT;  // default to neural_light
    CameraSelection camera_selection = CameraSelection::BOTH;  // default to both cameras
    std::string svo_file_path = "";  // for reconstruct mode
    
    // Handle different modes with their specific argument requirements
    if (app_mode == AppMode::RECONSTRUCT) {
        if (argc < 3) {
            std::cout << "Error: Reconstruct mode requires an SVO file path" << std::endl;
            print_usage();
            return EXIT_FAILURE;
        }
        svo_file_path = std::string(argv[2]);
        // Note: Resolution will be read automatically from SVO file
    } else if (app_mode == AppMode::CAPTURE) {
        // For capture mode, check if we're using single camera mode (no config file needed)
        bool single_camera_mode = false;
        for (int i = 2; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "--camera" && i + 1 < argc) {
                std::string camera_str(argv[i + 1]);
                if (camera_str == "zedx" || camera_str == "zedx-mini") {
                    single_camera_mode = true;
                    camera_selection = parse_camera_selection(camera_str);
                    std::cout << "Single camera mode detected: " << camera_str << std::endl;
                    arg_offset = 2; // No config file, start parsing from argv[2]
                    break;
                }
            }
        }
        
        if (!single_camera_mode) {
            // Multi-camera mode requires config file
            if (argc < 3) {
                std::cout << "Error: Multi-camera capture mode requires a config file" << std::endl;
                print_usage();
                return EXIT_FAILURE;
            }
            json_config = std::string(argv[2]);
            arg_offset = 3; // Config file present, start parsing from argv[3]
        }
    } else {
        // Other modes (fusion, live) require config file
        if (argc < 3) {
            std::cout << "Error: " << argv[1] << " mode requires a config file" << std::endl;
            print_usage();
            return EXIT_FAILURE;
        }
        json_config = std::string(argv[2]);
        arg_offset = 3;
    }
    
    // Parse options based on the argument offset
    if (app_mode != AppMode::RECONSTRUCT) {
        // Parse arguments starting from the appropriate offset
        for (int i = arg_offset; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg == "--duration" && i + 1 < argc) {
                recording_duration = std::atoi(argv[++i]);
            } else if (arg == "--output-dir" && i + 1 < argc) {
                output_dir = std::string(argv[++i]);
            } else if (arg == "--resolution" && i + 1 < argc) {
                std::string res_str(argv[++i]);
                if (res_str == "720") {
                    resolution = sl::RESOLUTION::HD720;
                    std::cout << "Using HD720 resolution" << std::endl;
                } else if (res_str == "1080") {
                    resolution = sl::RESOLUTION::HD1080;
                    std::cout << "Using HD1080 resolution" << std::endl;
                } else {
                    std::cout << "Invalid resolution '" << res_str << "'. Using default HD1080." << std::endl;
                    std::cout << "Valid options: 720, 1080" << std::endl;
                }
            } else if (arg == "--depth-mode" && i + 1 < argc) {
                std::string depth_str(argv[++i]);
                if (depth_str == "neural_light") {
                    depth_mode = sl::DEPTH_MODE::NEURAL_LIGHT;
                    std::cout << "Using NEURAL_LIGHT depth mode" << std::endl;
                } else if (depth_str == "neural") {
                    depth_mode = sl::DEPTH_MODE::NEURAL;
                    std::cout << "Using NEURAL depth mode" << std::endl;
                } else if (depth_str == "neural_plus") {
                    depth_mode = sl::DEPTH_MODE::NEURAL_PLUS;
                    std::cout << "Using NEURAL_PLUS depth mode" << std::endl;
                } else {
                    std::cout << "Invalid depth mode '" << depth_str << "'. Using default NEURAL_LIGHT." << std::endl;
                    std::cout << "Valid options: neural_light, neural, neural_plus" << std::endl;
                }
            } else if (arg == "--camera" && i + 1 < argc) {
                std::string camera_str(argv[++i]);
                camera_selection = parse_camera_selection(camera_str);
                if (camera_str == "zedx-mini") {
                    std::cout << "Using ZED-X-Mini camera only (SN: 57709210)" << std::endl;
                } else if (camera_str == "zedx") {
                    std::cout << "Using ZED-X camera only (SN: 47797222)" << std::endl;
                } else if (camera_str == "both") {
                    std::cout << "Using both cameras" << std::endl;
                } else {
                    std::cout << "Invalid camera selection '" << camera_str << "'. Using default 'both'." << std::endl;
                    std::cout << "Valid options: zedx-mini, zedx, both" << std::endl;
                }
            }
        }
    }

    // Defines the Coordinate system and unit used in this sample
    constexpr sl::COORDINATE_SYSTEM COORDINATE_SYSTEM = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    constexpr sl::UNIT UNIT = sl::UNIT::METER;

    std::vector<sl::FusionConfiguration> configurations;
    
    // Handle configuration loading based on mode and camera selection
    if (app_mode == AppMode::RECONSTRUCT) {
        // Reconstruct mode doesn't need configurations
        // Skip configuration loading entirely
    } else if (app_mode == AppMode::CAPTURE && (camera_selection == CameraSelection::ZEDX || camera_selection == CameraSelection::ZEDX_MINI)) {
        // Single camera mode - create configuration directly
        sl::FusionConfiguration config;
        uint64_t serial_number = (camera_selection == CameraSelection::ZEDX) ? 47797222 : 57709210;
        
        config.serial_number = serial_number;
        config.communication_parameters.setForLocalNetwork(30000); // Set for local network connection with port 30000
        config.input_type.setFromSerialNumber(serial_number);
        
        configurations.push_back(config);
        std::cout << "Created single camera configuration for " << get_camera_name(serial_number) << " (SN: " << serial_number << ")" << std::endl;
    } else {
        // Multi-camera mode or other modes - read JSON config
        if (json_config.empty()) {
            std::cout << "Error: Configuration file required for this mode" << std::endl;
            return EXIT_FAILURE;
        }
        
        // Read json file containing the configuration of your multicamera setup.    
        configurations = sl::readFusionConfigurationFile(json_config, COORDINATE_SYSTEM, UNIT);

        if (configurations.empty()) {
            std::cout << "Empty configuration File." << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    // Execute the appropriate mode
    switch (app_mode) {
        case AppMode::CAPTURE:
            return run_capture_mode(configurations, recording_duration, output_dir, resolution, depth_mode, camera_selection, argc, argv);
            
        case AppMode::RECONSTRUCT:
            return run_reconstruct_mode(svo_file_path);
            
        case AppMode::FUSION: {
            // For fusion mode, use SVO files from the svo_recordings directory
            auto svo_configs = create_svo_configurations(configurations, "./svo_recordings");
            if (svo_configs.empty()) {
                std::cout << "No SVO files found in ./svo_recordings/" << std::endl;
                std::cout << "Run capture mode first to record SVO files." << std::endl;
                return EXIT_FAILURE;
            }
            return run_fusion_mode(svo_configs, COORDINATE_SYSTEM, UNIT, argc, argv);
        }
        
        case AppMode::LIVE_FUSION:
            return run_fusion_mode(configurations, COORDINATE_SYSTEM, UNIT, argc, argv);
            
        default:
            std::cout << "Unknown application mode" << std::endl;
            return EXIT_FAILURE;
    }
}

