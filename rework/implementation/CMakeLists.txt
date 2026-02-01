cmake_minimum_required(VERSION 3.10)
project(AudioFilter C)

# Require C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add executable with new name
add_executable(audio_filter
    main.c
    fir.c
    iir.c
    wav.c
)

# Link math library (for sin, pow, etc.)
target_link_libraries(audio_filter m)

