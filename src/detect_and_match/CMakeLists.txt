project(detect_and_match)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-fPIC")

set(INC_DIR /root/mve/libs)
set(LINK_DIR /root/mve/libs)




set(FEATURES_FILES
        detect_features.cpp)

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

add_executable(detect_features ${FEATURES_FILES})
target_link_libraries(detect_features
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a
        )

set(MATCHING_FILES
        matching.cpp matching.cpp)


add_executable(matching ${MATCHING_FILES} matching.cpp)
target_link_libraries(matching
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_util.a
        libmve_sfm.a
        libmve.a
        )
