#include <cassert>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdlib>

#include "mve/depthmap.h"
#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "mve/scene.h"
#include "mve/view.h"


struct AppSettings
{
    std::string scenedir;
    std::string outmesh;
    std::string dmname = "depth-L0";
    std::string imagename = "undistorted";
    std::string mask;
    std::string aabb;
    bool with_normals = true;
    bool with_scale = true;
    bool with_conf = true;
    bool poisson_normals = false;
    float min_valid_fraction = 0.0f;
    float scale_factor = 2.5f; /* "Radius" of MVS patch (usually 5x5). */
    std::vector<int> ids;
    int view_id = -1;
    float dd_factor = 5.0f;
    int scale = 0;
};

int
main (int argc, char** argv)
{

    if(argc<4){
        std::cout<<"usage: scendir outmeshdir(.ply) scale"<<std::endl;
        std::cout<<"Example: ../../images/plant_scene ../../ply/plant.ply 0"<<std::endl;
        return -1;
    }

    AppSettings conf;

    // 场景文件夹
    conf.scenedir = argv[1];
    // 输出网格文件
    conf.outmesh= argv[2];
    // 获取图像尺度
    std::stringstream stream(argv[3]);
    stream>>conf.scale;

    conf.dmname = std::string("depth-L") + argv[3];
    conf.imagename = (conf.scale == 0)
                     ? "undistorted"
                     : std::string("undist-L") + argv[3];

    std::cout << "Using depthmap \"" << conf.dmname
              << "\" and color image \"" << conf.imagename << "\"" << std::endl;

    mve::TriangleMesh::Ptr pset(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(pset->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(pset->get_vertex_normals());
    mve::TriangleMesh::ColorList& vcolor(pset->get_vertex_colors());
    mve::TriangleMesh::ValueList& vvalues(pset->get_vertex_values()); // scale
    mve::TriangleMesh::ConfidenceList& vconfs(pset->get_vertex_confidences());

    // 加载场景
    mve::Scene::Ptr scene = mve::Scene::create(conf.scenedir);

    // 迭代所有的视图，从视图中获得三维点
    mve::Scene::ViewList& views(scene->get_views());

    for (std::size_t i = 0; i < views.size(); ++i) {
        mve::View::Ptr view = views[i];
        if (view == nullptr)
            continue;

        std::size_t view_id = view->get_id();

        // 保证存在相机参数
        mve::CameraInfo const& cam = view->get_camera();
        if (cam.flen == 0.0f)
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.dmname);
        if (dm == nullptr)
            continue;

        mve::ByteImage::Ptr ci;
        if (!conf.imagename.empty())
            ci = view->get_byte_image(conf.imagename);

        std::cout << "Processing view \"" << view->get_name()
                  << "\"" << (ci != nullptr ? " (with colors)" : "")
                  << "..." << std::endl;

        // 使用深度图还原
        mve::TriangleMesh::Ptr mesh;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam);
        mve::TriangleMesh::VertexList const& mverts(mesh->get_vertices());
        mve::TriangleMesh::NormalList const& mnorms(mesh->get_vertex_normals());
        mve::TriangleMesh::ColorList const& mvcol(mesh->get_vertex_colors());
        mve::TriangleMesh::ConfidenceList& mconfs(mesh->get_vertex_confidences());


        if (conf.with_normals)
            mesh->ensure_normals();

        if (conf.with_conf) {
            /* Per-vertex confidence down-weighting boundaries. */
            mve::geom::depthmap_mesh_confidences(mesh, 4);
        }

        std::vector<float> mvscale;
        if (conf.with_scale) {
            mvscale.resize(mverts.size(), 0.0f);
            mve::MeshInfo minfo = mve::MeshInfo(mesh);
            for (std::size_t j = 0; j < minfo.size(); ++j)
            {
                for (std::size_t k = 0; k < minfo[j].verts.size(); ++k)
                    mvscale[j] += (mverts[j] - mverts[minfo[j].verts[k]]).norm();
                mvscale[j] /= static_cast<float>(minfo[j].verts.size());
                mvscale[j] *= conf.scale_factor;
            }
        }

        // 将所有视角的三维点云集中到一起
        // 顶点坐标
        verts.insert(verts.end(), mverts.begin(), mverts.end());
        // 顶点颜色
        if (!mvcol.empty())
            vcolor.insert(vcolor.end(), mvcol.begin(), mvcol.end());
        // 顶点法向量
        if (conf.with_normals)
            vnorm.insert(vnorm.end(), mnorms.begin(), mnorms.end());

        // 顶点尺度(分辨率）
        if (conf.with_scale)
            vvalues.insert(vvalues.end(), mvscale.begin(), mvscale.end());

        // 顶点置信度
        if (conf.with_conf)
            vconfs.insert(vconfs.end(), mconfs.begin(), mconfs.end());

        dm.reset();
        ci.reset();
        view->cache_cleanup();
    }

    // 保存mesh
    std::cout << "Writing final point set (" << verts.size() << " points)..." << std::endl;
    assert(util::string::right(conf.outmesh, 4) == ".ply");
    {
        mve::geom::SavePLYOptions opts;
        opts.write_vertex_normals = conf.with_normals;
        opts.write_vertex_values = conf.with_scale;
        opts.write_vertex_confidences = conf.with_conf;
        mve::geom::save_ply_mesh(pset, conf.outmesh, opts);
    }


    return EXIT_SUCCESS;
}
