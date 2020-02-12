/*
*   compute the faces' centroids
*   by R. Falque
*   12/02/2020
*/

#ifndef COMPUTE_CENTROIDS_H
#define COMPUTE_CENTROIDS_H

#include <iostream>
#include <Eigen/Dense>

inline Eigen::MatrixXd compute_faces_centroids(Eigen::MatrixXd V, Eigen::MatrixXi F) {
    Eigen::MatrixXd F_centroids = Eigen::MatrixXd::Zero(3, F.cols());
    
    for (int i=0; i<F.cols(); i++)
        F_centroids.col(i) = ( V.col(F(0,i)) + V.col(F(1,i)) + V.col(F(2,i)) ) / 3;

    return F_centroids;
};

#endif
