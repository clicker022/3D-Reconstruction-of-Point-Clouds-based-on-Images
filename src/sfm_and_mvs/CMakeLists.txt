project(sfm_and_mvs)
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

#incremental_sfm

set(INCREMENTAL_SFM_FILES
        incremental_sfm.cpp defines.h functions.h functions.cpp)
add_executable(incremental_sfm ${INCREMENTAL_SFM_FILES})
target_link_libraries(incremental_sfm
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_sfm.a
        libmve.a
        libmve_util.a)

FIND_PACKAGE( OpenMP REQUIRED)
if(OPENMP_FOUND)
    message("OPENMP FOUND")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


#dmrecon_multi_views

set(DMRECON_MULTI_VIEWS_FILES
        dmrecon_multi_views.cpp)
add_executable(dmrecon_multi_views ${DMRECON_MULTI_VIEWS_FILES})
target_link_libraries(dmrecon_multi_views
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_sfm.a
        libmve.a
        libmve_util.a)


#mvs_multi_views

set(MVS_MULTI_VIEWS_FILES
        mvs_multi_views.cpp)
add_executable(mvs_multi_views ${MVS_MULTI_VIEWS_FILES})
target_link_libraries(mvs_multi_views
        tiff
        jpeg
        png
        libmve_dmrecon.a
        libmve_fssr.a
        libmve_ogl.a
        libmve_sfm.a
        libmve.a
        libmve_util.a)

