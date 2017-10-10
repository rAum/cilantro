#include <cilantro/iterative_closest_point.hpp>
#include <cilantro/ply_io.hpp>
#include <cilantro/voxel_grid.hpp>
#include <cilantro/visualizer.hpp>

bool proceed = false;

void callback(Visualizer &viz, int key, void *cookie) {
    if (key == 'a') proceed = true;
}

int main(int argc, char ** argv) {

    Visualizer viz("win", "disp");
    viz.registerKeyboardCallback(std::vector<int>(1,'a'), callback, NULL);

    PointCloud dst, src;
    readPointCloudFromPLYFile(argv[1], dst);

    dst = VoxelGrid(dst, 0.005).getDownsampledCloud();

    src = dst;
    for (size_t i = 0; i < src.size(); i++) {
        src.points[i] += 0.005f*Eigen::Vector3f::Random();
    }

    PointCloud dst2;
    for (size_t i = 0; i < dst.size(); i++) {
        if (dst.points[i][0] > -0.4) {
            dst2.points.push_back(dst.points[i]);
            dst2.normals.push_back(dst.normals[i]);
        }
    }
    dst = dst2;

    Eigen::Matrix3f R_ref;
    R_ref = Eigen::AngleAxisf(-0.10 ,Eigen::Vector3f::UnitZ()) *
            Eigen::AngleAxisf(0.01, Eigen::Vector3f::UnitY()) *
            Eigen::AngleAxisf(-0.07, Eigen::Vector3f::UnitX());
    Eigen::Vector3f t_ref(-0.20, -0.05, 0.09);

    src.pointsMatrixMap() = (R_ref*src.pointsMatrixMap()).colwise() + t_ref;
    src.normalsMatrixMap() = R_ref*src.normalsMatrixMap();

//    std::vector<size_t> ind(dst.size());
//    for (size_t i = 0; i < ind.size(); i++) {
//        ind[i] = i;
//    }
//    std::random_shuffle(ind.begin(), ind.end());

    viz.addPointCloud("dst", dst, RenderingProperties().setDrawingColor(0,0,1));
    viz.addPointCloud("src", src, RenderingProperties().setDrawingColor(1,0,0));

    while (!proceed) {
        viz.spinOnce();
    }
    proceed = false;

    auto start = std::chrono::high_resolution_clock::now();

    Eigen::Matrix3f R_est;
    Eigen::Vector3f t_est;
//    estimateRigidTransformPointToPoint(dst.points, src.points, ind, ind, R_est, t_est);
//    estimateRigidTransformPointToPlane(dst, src, ind, ind, R_est, t_est, 10, 1e-4f);
//    IterativeClosestPoint icp(dst, src, IterativeClosestPoint::Metric::POINT_TO_POINT);
    IterativeClosestPoint icp(dst, src, IterativeClosestPoint::Metric::POINT_TO_PLANE);
    icp.setMaxCorrespondenceDistance(0.05f).setConvergenceTolerance(1e-3f).setMaxNumberOfIterations(200).setMaxNumberOfPointToPlaneIterations(1);
//    icp.setInitialTransformation(R_ref.transpose(), (-R_ref.transpose()*t_ref));
    icp.getTransformation(R_est, t_est);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << "ms" << std::endl;

    std::cout << "Iterations performed: " << icp.getPerformedIterationsCount() << std::endl;
    std::cout << "Has converged: " << icp.hasConverged() << std::endl;

    std::cout << "TRUE transformation R:" << std::endl << R_ref.transpose() << std::endl;
    std::cout << "TRUE transformation t:" << std::endl << (-R_ref.transpose()*t_ref).transpose() << std::endl;
    std::cout << "ESTIMATED transformation R:" << std::endl << R_est << std::endl;
    std::cout << "ESTIMATED transformation t:" << std::endl << t_est.transpose() << std::endl;

    PointCloud src_trans(src);

    src_trans.pointsMatrixMap() = (R_est*src_trans.pointsMatrixMap()).colwise() + t_est;
    src_trans.normalsMatrixMap() = R_est*src_trans.normalsMatrixMap();

    viz.addPointCloud("dst", dst, RenderingProperties().setDrawingColor(0,0,1));
    viz.addPointCloud("src", src_trans, RenderingProperties().setDrawingColor(1,0,0));

    while (!proceed) {
        viz.spinOnce();
    }
    proceed = false;

    viz.clear();
    viz.addPointCloud("src", src_trans).addPointCloudValues("src", icp.getResiduals(IterativeClosestPoint::Metric::POINT_TO_POINT));

    while (!proceed) {
        viz.spinOnce();
    }
    proceed = false;

    return 0;
}
