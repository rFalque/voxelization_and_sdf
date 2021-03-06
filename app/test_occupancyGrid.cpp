#include <iostream>
#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include "IO/readPLY.h"
#include "IO/writePLY.h"

#include "visualization/plotMesh.h"
#include "visualization/plotTwoMeshes.h"

#include "mesh/computeNormals.h"
#include "mesh/computeFacesCentroids.h"
#include "occupancyGrid.h"

int main() {
    bool visualization = true;
    int grid_resolution = 100;          // grid_resolution is used to define the grid resolution in the maximum direction
    double bounding_box_scale = 1;

    // IO: load files
    std::cout << "Progress: load data\n";
    Eigen::MatrixXd V, cubes_V, faces_V;
    Eigen::MatrixXi F, cubes_F;
    Eigen::MatrixXd N, faces_N;
    Eigen::MatrixXi RGB;

    readPLY("../data/Lucy100k.ply", V, F, N, RGB);

    std::cout << "size of V: " << V.rows() << ", " << V.cols() << "\n";

    faces_V = compute_faces_centroids(V,F);
    faces_N = compute_faces_normals(V,F);

    if (visualization)
        plot_mesh(V,F);

    OccupancyGrid occupancy_grid(faces_V, faces_N, grid_resolution, bounding_box_scale);
    
    Eigen::MatrixXd graph_V;
    Eigen::MatrixXi graph_E;
    occupancy_grid.generate_graph(graph_V, graph_E);
    occupancy_grid.print_to_folder("../data/occupancy_grid/");
    occupancy_grid.print_to_yaml("../data/lucy");

    occupancy_grid.generate_mesh(cubes_V, cubes_F);

    if (visualization)
        plot_two_meshes(V,F,cubes_V,cubes_F);

}
