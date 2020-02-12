#ifndef GETMINMAX_HPP
#define GETMINMAX_HPP

#include <Eigen/Core>

static inline void getMinMax(Eigen::MatrixXd & in_cloud, Eigen::Vector3d & min_point, Eigen::Vector3d & max_point){
    max_point = in_cloud.rowwise().maxCoeff();
    min_point = in_cloud.rowwise().minCoeff();
};

inline void getScale(Eigen::MatrixXd in_cloud, double & scale){
    Eigen::Vector3d min_point;
    Eigen::Vector3d max_point;

    getMinMax(in_cloud, min_point, max_point);

    scale = (max_point - min_point).norm();
};

#endif
