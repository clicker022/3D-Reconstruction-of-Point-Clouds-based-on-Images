#include <iostream>
#include "sfm/sift.h"
#include "util/timer.h"
#include "mve/image_io.h"
#include "sfm/visualizer.h"

bool
sift_compare (sfm::Sift::Descriptor const& d1, sfm::Sift::Descriptor const& d2)
{
    return d1.scale > d2.scale;
}

int
main (int argc, char** argv)
{
    /*
     * 加载图像
     * 将image_filename路径更改为自己的路径
     */
    mve::ByteImage::Ptr image;
    std::string image_filename = "./tmp/hh.jpg";

    try
    {
        std::cout << "Loading " << image_filename << "..." << std::endl;
        image = mve::image::load_file(image_filename);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    /*
     * SIFT特征检测
     * Sift::Keypoints SIFT特征关键点
     * Sift::Descriptors 特征描述子
     * timer 计时器，获取SIFT计算时间
     * sift_options.verbose_output和debug_output代表输出更详细的信息
     */
    sfm::Sift::Descriptors sift_descr;
    sfm::Sift::Keypoints sift_keypoints;
    {
        sfm::Sift::Options sift_options;
        sift_options.verbose_output = true;
        sift_options.debug_output = true;
        sfm::Sift sift(sift_options);
        sift.set_image(image);

        util::WallTimer timer;
        sift.process();
        std::cout << "Computed SIFT features in "
                  << timer.get_elapsed() << "ms." << std::endl;

        sift_descr = sift.get_descriptors();
        sift_keypoints = sift.get_keypoints();
    }

    /*
     * 根据特征点的尺度对特征点进行排序
     */
    std::sort(sift_descr.begin(), sift_descr.end(), sift_compare);

    /*
     * 绘画出特征点
     */
    std::vector<sfm::Visualizer::Keypoint> sift_drawing;
    for (std::size_t i = 0; i < sift_descr.size(); ++i)
    {
        sfm::Visualizer::Keypoint kp;
        kp.orientation = sift_descr[i].orientation;
        kp.radius = sift_descr[i].scale;
        kp.x = sift_descr[i].x;
        kp.y = sift_descr[i].y;
        sift_drawing.push_back(kp);
    }

    mve::ByteImage::Ptr sift_image = sfm::Visualizer::draw_keypoints(image,
                                                                     sift_drawing, sfm::Visualizer::RADIUS_BOX_ORIENTATION);

    /*
     * 保存图像
     */
    std::string sift_out_fname = "C:/Users/cao/Desktop/code/reconstruction/tmp/hwu.jpg";
    std::cout << "保存图像: " << sift_out_fname << std::endl;
    mve::image::save_file(sift_image, sift_out_fname);

    return 0;
}
