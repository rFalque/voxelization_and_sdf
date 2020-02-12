#include <iostream>
#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include "IO/readPLY.h"
#include "IO/writePLY.h"

#include "visualization/plotMesh.h"
#include "visualization/plotTwoMeshes.h"

#include "mesh/computeNormals.h"
#include "mesh/computeFacesCentroids.h"
#include "sdf.h"

int main() {
    bool visualization = true;
    int grid_resolution = 100;
    double bounding_box_scale = 1;

    // IO: load files
    std::cout << "Progress: load data\n";
    Eigen::MatrixXd V, faces_V;
    Eigen::MatrixXi F;
    Eigen::MatrixXd N, faces_N;
    Eigen::MatrixXi RGB;

    readPLY("../data/Lucy100k.ply", V, F, N, RGB);

    faces_V = compute_faces_centroids(V,F);
    faces_N = compute_faces_normals(V,F);

    if (visualization)
        plot_mesh(V,F);

    // grid_resolution is used to define the number of grids

    SDF sdf(faces_V, faces_N, grid_resolution, bounding_box_scale);
    
    Eigen::MatrixXd graph_V;
    Eigen::MatrixXi graph_E;
    sdf.generate_graph(graph_V, graph_E);
    sdf.print_to_folder("../data/sdf/");

}
