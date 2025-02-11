#ifndef RECONSTRUCTION_FUNCTIONS_H
#define RECONSTRUCTION_FUNCTIONS_H


#include <util/timer.h>
#include "util/file_system.h"
#include "mve/scene.h"
#include "mve/image_tools.h"
#include "mve/view.h"
#include "mve/image.h"

mve::ByteImage::Ptr
load_8bit_image (std::string const& fname, std::string* exif);

mve::RawImage::Ptr
load_16bit_image (std::string const& fname);

mve::FloatImage::Ptr
load_float_image (std::string const& fname);

mve::ImageBase::Ptr
load_any_image (std::string const& fname, std::string* exif);

std::string
remove_file_extension (std::string const& filename);


/**
 * 模板类
 * @tparam T
 * @param img
 * @param max_pixels
 * @return
 */
template <class T>
typename mve::Image<T>::Ptr
limit_image_size (typename mve::Image<T>::Ptr img, int max_pixels)
{
    while (img->get_pixel_amount() > max_pixels)
        img = mve::image::rescale_half_size<T>(img);
    return img;
}

/**
 *
 * @param image
 * @param max_pixels
 * @return
 */
mve::ImageBase::Ptr
limit_image_size (mve::ImageBase::Ptr image, int max_pixels);

bool has_jpeg_extension (std::string const& filename);

std::string make_image_name (int id);

void  add_exif_to_view (mve::View::Ptr view, std::string const& exif);


#endif //RECONSTRUCTION_FUNCTIONS_H
