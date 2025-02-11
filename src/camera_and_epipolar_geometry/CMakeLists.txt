project(camera_and_epipolar_geometry)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-fPIC")

set(INC_DIR /root/mve/libs)
set(LINK_DIR /root/mve/libs)

include_directories(
        ${INC_DIR}
)

link_directories(
        ${LINK_DIR}
        ${LINK_DIR}/dmrecon
        ${LINK_DIR}/fssr
        ${LINK_DIR}/math
        ${LINK_DIR}/mve
        ${LINK_DIR}/ogl
        ${LINK_DIR}/sfm
        ${LINK_DIR}/util
)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
#camera

set(CAMERA_FILES
        camera.cpp)
add_executable(camera ${CAMERA_FILES})
target_link_libraries(camera
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#math_test
set(MATH_TEST_FILES
        math_test.cpp)
add_executable(math_test ${MATH_TEST_FILES})
target_link_libraries(math_test
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#fundamental_8_point
set(FUNDAMENTAL_8_POINT_FILES
        fundamental_8_point.cpp)
add_executable(fundamental_8_point ${FUNDAMENTAL_8_POINT_FILES})
target_link_libraries(fundamental_8_point
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#fundamental_ransac
set(FUNDAMENTAL_RANSAC_FILES
        fundamental_ransac.cpp)
add_executable(fundamental_ransac ${FUNDAMENTAL_RANSAC_FILES})
target_link_libraries(fundamental_ransac
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#pose_from_fundamental
set(POSE_FROM_FUNDAMENTAL_FILES
        pose_from_fundamental.cpp)
add_executable(pose_from_fundamental ${POSE_FROM_FUNDAMENTAL_FILES})
target_link_libraries(pose_from_fundamental
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#triangle
set(TRIANGLE_FILES
        triangle.cpp)
add_executable(triangle ${TRIANGLE_FILES})
target_link_libraries(triangle
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a)

#bundle_adjustment
set(BUNDLE_ADJUSTMENT_FILES
        bundle_adjustment.cpp)
add_executable(bundle_adjustment ${BUNDLE_ADJUSTMENT_FILES})
target_link_libraries(bundle_adjustment
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a
        Eigen3::Eigen)

FIND_PACKAGE( OpenMP REQUIRED)
if(OPENMP_FOUND)
    message("OPENMP FOUND")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()






