# GDB configuration for ZED Spatial Mapping Fusion debugging

# Enable pretty printing of STL containers and smart pointers
set print pretty on
set print object on
set print static-members on
set print vtbl on
set print demangle on
set demangle-style gnu-v3

# Show more context when displaying source code
set listsize 20

# History settings
set history save on
set history size 10000
set history filename ~/.gdb_history

# Enable colored output if available
set style enabled on

# Convenience settings for debugging
set confirm off
set pagination off

# ZED SDK specific settings
# Skip system libraries during stepping
skip -rfunction ^std::.*
skip -rfunction ^__.*

# Custom commands for ZED debugging
define print_zed_error
    if $arg0 == 0
        printf "ERROR_CODE::SUCCESS\n"
    else
        if $arg0 == 1
            printf "ERROR_CODE::FAILURE\n"
        else
            if $arg0 == 2
                printf "ERROR_CODE::NO_GPU_COMPATIBLE\n"
            else
                if $arg0 == 3
                    printf "ERROR_CODE::NOT_ENOUGH_GPU_MEMORY\n"
                else
                    printf "ERROR_CODE: %d\n", $arg0
                end
            end
        end
    end
end
document print_zed_error
Print ZED error code meaning
Usage: print_zed_error <error_code_value>
end

define print_fusion_error
    if $arg0 == 0
        printf "FUSION_ERROR_CODE::SUCCESS\n"
    else
        if $arg0 == 1
            printf "FUSION_ERROR_CODE::FAILURE\n"
        else
            if $arg0 == 2
                printf "FUSION_ERROR_CODE::INVALID_PARAMETER\n"
            else
                printf "FUSION_ERROR_CODE: %d\n", $arg0
            end
        end
    end
end
document print_fusion_error
Print Fusion error code meaning
Usage: print_fusion_error <fusion_error_code_value>
end

# Useful breakpoints for debugging
define setup_common_breakpoints
    break main
    break ClientPublisher::open
    break sl::Fusion::init
    break sl::Fusion::process
    printf "Common breakpoints set\n"
end
document setup_common_breakpoints
Set common breakpoints for ZED debugging
end

# Print current ZED camera parameters
define print_init_params
    printf "Init Parameters:\n"
    printf "  Resolution: %d\n", init_parameters.camera_resolution
    printf "  FPS: %d\n", init_parameters.camera_fps
    printf "  Depth Mode: %d\n", init_parameters.depth_mode
    printf "  Coordinate Units: %d\n", init_parameters.coordinate_units
    printf "  Coordinate System: %d\n", init_parameters.coordinate_system
end
document print_init_params
Print current initialization parameters
end

printf "ZED Spatial Mapping Fusion GDB configuration loaded\n"
printf "Available custom commands:\n"
printf "  setup_common_breakpoints - Set common debugging breakpoints\n"
printf "  print_zed_error <code>   - Print ZED error code meaning\n"
printf "  print_fusion_error <code> - Print Fusion error code meaning\n"
printf "  print_init_params        - Print initialization parameters\n"




