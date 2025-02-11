cmake_minimum_required(VERSION 3.6)
project(reconstruction)

set(CMAKE_CXX_STANDARD 14)

add_executable(reconstruction main.cpp detect_and_match/detect_features.cpp camera_and_epipolar_geometry/camera.cpp)

add_subdirectory(detect_and_match)

add_subdirectory(camera_and_epipolar_geometry)

add_subdirectory(sfm_and_mvs)