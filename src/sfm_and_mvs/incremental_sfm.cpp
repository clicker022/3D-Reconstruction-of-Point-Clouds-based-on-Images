#include <cassert>
#include "defines.h"
#include "functions.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_intrinsics.h"
#include "sfm/bundler_init_pair.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_incremental.h"
#include "mve/scene.h"
#include "util/timer.h"


#include "mve/bundle_io.h"
#include "mve/camera.h"
#include "util/file_system.h"
#include <fstream>
#include <iostream>

/**
 * @name make_scene
 * @description 创建一个场景
 * @param image_folder_path 图像文件夹路径
 * @param scene_path 场景路径
 * @return
 */
mve::Scene::Ptr
make_scene(const std::string & image_folder_path, const std::string & scene_path){

    util::WallTimer timer;

    // 创建文件夹
    const std::string views_path = util::fs::join_path(scene_path, "views/");

    util::fs::mkdir(scene_path.c_str());
    util::fs::mkdir(views_path.c_str());

    // 扫描文件夹，获取所有的图像文件路径
    util::fs::Directory dir;
    try {dir.scan(image_folder_path);
    }
    catch (std::exception&e){
        std::cerr << "Error scanning input dir: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Found " << dir.size() << " directory entries." << std::endl;
    mve::Scene::Ptr scene= mve::Scene::create(scene_path);

    // 开始加载图像
    std::sort(dir.begin(), dir.end());
    int num_imported = 0;
    for(std::size_t i=0; i< dir.size(); i++){
        // 是一个文件夹
        if(dir[i].is_dir){
            std::cout<<"Skipping directory "<<dir[i].name<<std::endl;
            continue;
        }

        std::string fname = dir[i].name;
        std::string afname = dir[i].get_absolute_name();

        // 从可交换信息文件中读取图像焦距
        std::string exif;
        mve::ImageBase::Ptr image = load_any_image(afname, & exif);
        if(image == nullptr){
            continue;
        }

        mve::View::Ptr view = mve::View::create();
        view->set_id(num_imported);
        view->set_name(remove_file_extension(fname));

        // 限制图像尺寸
        int orig_width = image->width();
        image = limit_image_size(image, MAX_PIXELS);
        if (orig_width == image->width() && has_jpeg_extension(fname))
            view->set_image_ref(afname, "original");
        else
            view->set_image(image, "original");

        add_exif_to_view(view, exif);

        scene->get_views().push_back(view);

        // 保存视角信息到本地
        std::string mve_fname = make_image_name(num_imported);
        std::cout << "Importing image: " << fname
                  << ", writing MVE view: " << mve_fname << "..." << std::endl;
        view->save_view_as(util::fs::join_path(views_path, mve_fname));

        num_imported+=1;
    }

    std::cout << "Imported " << num_imported << " input images, "
              << "took " << timer.get_elapsed() << " ms." << std::endl;

    return scene;
}

/**
 * @name features_and_matching
 * @description 特征检测和特征匹配
 * @param scene
 * @param viewports
 * @param pairwise_matching
 */
void
features_and_matching (mve::Scene::Ptr scene,
                       sfm::bundler::ViewportList* viewports,
                       sfm::bundler::PairwiseMatching* pairwise_matching){

    // 计算场景中图像的特征点
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = "original";
    feature_opts.max_image_size = MAX_PIXELS;
    feature_opts.feature_options.feature_types = sfm::FeatureSet::FEATURE_SIFT;

    std::cout << "Computing image features..." << std::endl;
    {
        util::WallTimer timer;
        sfm::bundler::Features bundler_features(feature_opts);
        bundler_features.compute(scene, viewports);

        std::cout << "Computing features took " << timer.get_elapsed()
                  << " ms." << std::endl;
        std::cout<<"Feature detection took " + util::string::get(timer.get_elapsed()) + "ms."<<std::endl;
    }

    // 视图两两匹配
    sfm::bundler::Matching::Options matching_opts;
    //matching_opts.ransac_opts.max_iterations = 1000;
    //matching_opts.ransac_opts.threshold = 0.0015;
    matching_opts.ransac_opts.verbose_output = false;
    matching_opts.use_lowres_matching = false;
    matching_opts.match_num_previous_frames = false;
    matching_opts.matcher_type = sfm::bundler::Matching::MATCHER_EXHAUSTIVE;

    std::cout << "Performing feature matching..." << std::endl;
    {
        util::WallTimer timer;
        sfm::bundler::Matching bundler_matching(matching_opts);
        bundler_matching.init(viewports);
        bundler_matching.compute(pairwise_matching);
        std::cout << "Matching took " << timer.get_elapsed()
                  << " ms." << std::endl;
        std::cout<< "Feature matching took "
                    + util::string::get(timer.get_elapsed()) + "ms."<<std::endl;
    }

    if (pairwise_matching->empty()) {
        std::cerr << "Error: No matching image pairs. Exiting." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


int main(int argc, char *argv[])
{

    if(argc < 4){
        std::cout<<"Usage: [input]image_dir [output]scene_dir [output]ply_dir"<<std::endl;
        return -1;
    }

    // 新建场景
    mve::Scene::Ptr scene = make_scene(argv[1], argv[2]);
    std::cout<<"Scene has "<<scene->get_views().size()<<" views. "<<std::endl;

    // 特征检测和特征匹配
    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching pairwise_matching;
    features_and_matching(scene, &viewports, &pairwise_matching );


    // 删除特征描述子
    scene->cache_cleanup();
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.clear_descriptors();

    // 检测匹配
    if (pairwise_matching.empty()) {
        std::cerr << "No matching image pairs. Exiting." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // 计算相机内参数，从Exif中读取
    {
        sfm::bundler::Intrinsics::Options intrinsics_opts;
        std::cout << "Initializing camera intrinsics..." << std::endl;
        sfm::bundler::Intrinsics intrinsics(intrinsics_opts);
        intrinsics.compute(scene, &viewports);
    }

    //开始增量的捆绑调整
    util::WallTimer timer;

    // 计算特征tracks
    sfm::bundler::TrackList tracks;
    {
        sfm::bundler::Tracks::Options tracks_options;
        tracks_options.verbose_output = true;

        sfm::bundler::Tracks bundler_tracks(tracks_options);
        std::cout << "Computing feature tracks..." << std::endl;
        bundler_tracks.compute(pairwise_matching, &viewports, &tracks);
        std::cout << "Created a total of " << tracks.size()
                  << " tracks." << std::endl;
    }

    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.colors.clear();
    pairwise_matching.clear();


    // 计算初始的匹配对
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair::Options init_pair_opts;
    init_pair_opts.homography_opts.verbose_output = false;
    init_pair_opts.max_homography_inliers = 0.8f;
    init_pair_opts.verbose_output = true;

    // 开始计算初始的匹配对
    sfm::bundler::InitialPair init_pair(init_pair_opts);
    init_pair.initialize(viewports, tracks);
    init_pair.compute_pair(&init_pair_result);
    std::cout << "Using views " << init_pair_result.view_1_id
              << " and " << init_pair_result.view_2_id
              << " as initial pair." << std::endl;

    if (init_pair_result.view_1_id < 0 || init_pair_result.view_2_id < 0
        || init_pair_result.view_1_id >= static_cast<int>(viewports.size())
        || init_pair_result.view_2_id >= static_cast<int>(viewports.size()))
    {
        std::cerr << "Error finding initial pair, exiting!" << std::endl;
        std::cerr << "Try manually specifying an initial pair." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Using views " << init_pair_result.view_1_id
              << " and " << init_pair_result.view_2_id
              << " as initial pair." << std::endl;


    sfm::bundler::Incremental::Options incremental_opts;
    incremental_opts.pose_p3p_opts.max_iterations = 1000;
    incremental_opts.pose_p3p_opts.threshold = 0.005f;
    incremental_opts.pose_p3p_opts.verbose_output = false;
    incremental_opts.track_error_threshold_factor = TRACK_ERROR_THRES_FACTOR;
    incremental_opts.new_track_error_threshold = NEW_TRACK_ERROR_THRES;
    incremental_opts.min_triangulation_angle = MATH_DEG2RAD(1.0);
    incremental_opts.ba_fixed_intrinsics = false;
    incremental_opts.verbose_output = true;
    incremental_opts.verbose_ba = true;

    /* Initialize viewports with initial pair. */
    viewports[init_pair_result.view_1_id].pose = init_pair_result.view_1_pose;
    viewports[init_pair_result.view_2_id].pose = init_pair_result.view_2_pose;

    /* Initialize the incremental bundler and reconstruct first tracks. */
    sfm::bundler::Incremental incremental(incremental_opts);
    incremental.initialize(&viewports, &tracks);

    // 对当前两个视角进行track重建，并且如果track存在外点，则将每个track的外点剥离成新的track
    incremental.triangulate_new_tracks(2);

    // 根据重投影误差进行筛选
    incremental.invalidate_large_error_tracks();

    // 捆绑调整
    std::cout << "Running full bundle adjustment..." << std::endl;
    incremental.bundle_adjustment_full();

    // 重建新视角
    int num_cameras_reconstructed = 2;
    int full_ba_num_skipped = 0;
    while (true)
    {
        // 获得新视角
        std::vector<int> next_views;
        incremental.find_next_views(&next_views);

        // 重建新视角
        int next_view_id = -1;
        for (std::size_t i = 0; i < next_views.size(); ++i)
        {
            std::cout << std::endl;
            std::cout << "Adding next view ID " << next_views[i]
                      << " (" << (num_cameras_reconstructed + 1) << " of "
                      << viewports.size() << ")..." << std::endl;
            if (incremental.reconstruct_next_view(next_views[i]))
            {
                next_view_id = next_views[i];
                break;
            }
        }

        if (next_view_id < 0) {
            if (full_ba_num_skipped == 0) {
                std::cout << "No valid next view." << std::endl;
                std::cout << "SfM reconstruction finished." << std::endl;
                break;
            }
            else
            {
                incremental.triangulate_new_tracks(MIN_VIEWS_PER_TRACK);
                std::cout << "Running full bundle adjustment..." << std::endl;
                incremental.invalidate_large_error_tracks();
                incremental.bundle_adjustment_full();
                full_ba_num_skipped = 0;
                continue;
            }
        }

        // 相机BA优化
        std::cout << "Running single camera bundle adjustment..." << std::endl;
        incremental.bundle_adjustment_single_cam(next_view_id);
        num_cameras_reconstructed += 1;

        // 全局BA优化
        int const full_ba_skip_views =  std::min(100, num_cameras_reconstructed / 10);
        if (full_ba_num_skipped < full_ba_skip_views)
        {
            std::cout << "Skipping full bundle adjustment (skipping "
                      << full_ba_skip_views << " views)." << std::endl;
            full_ba_num_skipped += 1;
        }
        else
        {
            incremental.triangulate_new_tracks(MIN_VIEWS_PER_TRACK);
            std::cout << "Running full bundle adjustment..." << std::endl;
            incremental.invalidate_large_error_tracks();
            incremental.bundle_adjustment_full();
            full_ba_num_skipped = 0;
        }
    }

    sfm::bundler::TrackList valid_tracks;
    for(int i=0; i<tracks.size(); i++){
        if(tracks[i].is_valid()){
            valid_tracks.push_back(tracks[i]);
        }
    }

    std::cout << "SfM reconstruction took " << timer.get_elapsed()
              << " ms." << std::endl;
    std::cout<< "SfM reconstruction took "
                + util::string::get(timer.get_elapsed()) + "ms."<<std::endl;


    //保存输出的点云结果
    std::ofstream out_file(argv[2]);
    assert(out_file.is_open());
    out_file<<"ply"<<std::endl;
    out_file<<"format ascii 1.0"<<std::endl;
    out_file<<"element vertex "<<valid_tracks.size()<<std::endl;
    out_file<<"property float x"<<std::endl;
    out_file<<"property float y"<<std::endl;
    out_file<<"property float z"<<std::endl;
    out_file<<"property uchar red"<<std::endl;
    out_file<<"property uchar green"<<std::endl;
    out_file<<"property uchar blue"<<std::endl;
    out_file<<"end_header"<<std::endl;

    for(int i=0; i< valid_tracks.size(); i++){
        out_file<<valid_tracks[i].pos[0]<<" "<< valid_tracks[i].pos[1]<<" "<<valid_tracks[i].pos[2]<<" "
                <<(int)valid_tracks[i].color[0]<<" "<<(int)valid_tracks[i].color[1]<<" "<<(int)valid_tracks[i].color[2]<<std::endl;
    }
    out_file.close();

    std::cout << "Creating bundle data structure..." << std::endl;
    mve::Bundle::Ptr bundle = incremental.create_bundle();
    mve::save_mve_bundle(bundle, std::string(argv[2]) + "/synth_0.out");

    mve::Bundle::Cameras const& bundle_cams = bundle->get_cameras();
    mve::Scene::ViewList const& views = scene->get_views();
    if (bundle_cams.size() != views.size())
    {
        std::cerr << "Error: Invalid number of cameras!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

#pragma omp parallel for schedule(dynamic,1)
    for (std::size_t i = 0; i < bundle_cams.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        mve::CameraInfo const& cam = bundle_cams[i];
        if (view == nullptr)
            continue;
        if (view->get_camera().flen == 0.0f && cam.flen == 0.0f)
            continue;

        view->set_camera(cam);

        /*   image. */
        if (!undistorted_name.empty())
        {
            mve::ByteImage::Ptr original
                    = view->get_byte_image(original_name);
            if (original == nullptr)
                continue;
            mve::ByteImage::Ptr undist
                    = mve::image::image_undistort_k2k4<uint8_t>
                            (original, cam.flen, cam.dist[0], cam.dist[1]);
            view->set_image(undist, undistorted_name);
        }

#pragma omp critical
        std::cout << "Saving view " << view->get_directory() << std::endl;
        view->save_view();
        view->cache_cleanup();
    }

    // log_message(conf, "SfM reconstruction done.\n");

    return 0;
}

