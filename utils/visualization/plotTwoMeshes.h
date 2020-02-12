/**
 * visualize correspondences
 * 
 * by R. Falque
 * 17/01/2020
 **/

#ifndef PLOT_TWO_MESH_H
#define PLOT_TWO_MESH_H

#include <iostream>
#include <Eigen/Core>


#include "polyscope/polyscope.h"
#include "polyscope/messages.h"
#include "polyscope/surface_mesh.h"

#ifndef POLYSCOPE_IS_INITIALIZED
#define POLYSCOPE_IS_INITIALIZED
int polyscope_is_initialized { 0 };
#endif

inline bool plot_two_meshes(const Eigen::MatrixXd & source_V,
                                const Eigen::MatrixXi & source_F,
                                const Eigen::MatrixXd & target_V,
                                const Eigen::MatrixXi & target_F)
{

    if (!polyscope_is_initialized) {
        polyscope::init();
        polyscope_is_initialized = 1;
    }

    polyscope::options::autocenterStructures = false;
    polyscope::view::style = polyscope::view::NavigateStyle::Free;
    polyscope::view::upDir = polyscope::view::UpDir::ZUp;

    polyscope::view::windowWidth = 1024;
    polyscope::view::windowHeight = 1024;

    polyscope::registerSurfaceMesh("first mesh", source_V.transpose(), source_F.transpose());
    polyscope::getSurfaceMesh("first mesh")->setSurfaceColor(glm::vec3{0.1, 0.1, 1});

    polyscope::registerSurfaceMesh("second mesh", target_V.transpose(), target_F.transpose());
    polyscope::getSurfaceMesh("second mesh")->setSurfaceColor(glm::vec3{1, 0.1, 0.1});

    polyscope::show();

    polyscope::removeAllStructures();
    return true;
};


#endif
