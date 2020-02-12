/*
*   compute the normals using the faces
*   by R. Falque
*   16/01/2020
*/

#ifndef COMPUTE_NORMALS_H
#define COMPUTE_NORMALS_H

#include <iostream>
#include <Eigen/Dense>

inline Eigen::MatrixXd compute_vertices_normals(Eigen::MatrixXd V, Eigen::MatrixXi F) {
    Eigen::MatrixXd normals = Eigen::MatrixXd::Zero(3, V.cols());
    Eigen::Vector3d normal_temp, v1, v2;

    for (int i=0; i<F.cols(); i++) {
        v1 = V.col(F(1,i)) - V.col(F(0,i));
        v2 = V.col(F(2,i)) - V.col(F(0,i));
        normal_temp = v1.cross(v2);
        normals.col(F(0, i)) += normal_temp;
        normals.col(F(1, i)) += normal_temp;
        normals.col(F(2, i)) += normal_temp;
    }

    for (int i=0; i<normals.cols(); i++) {
        if (normals.col(i).norm() != 0) 
        {
            normals.col(i) = normals.col(i).normalized();
        }
        else
        {
            std::cout << "zero values\n";
        }
    }

    return normals;
};

inline Eigen::MatrixXd compute_faces_normals(Eigen::MatrixXd V, Eigen::MatrixXi F) {
    Eigen::MatrixXd normals = Eigen::MatrixXd::Zero(3, F.cols());
    Eigen::Vector3d v1, v2;

    for (int i=0; i<F.cols(); i++) {
        v1 = V.col(F(1,i)) - V.col(F(0,i));
        v2 = V.col(F(2,i)) - V.col(F(0,i));
        normals.col(i) = ( v1.cross(v2) ).normalized();
    }

    return normals;
};

#endif
