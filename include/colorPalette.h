/**
 * Author: R. Falque
 * 
 * create a color palette for minecraft export: https://minecraft.gamepedia.com/Map_item_format
 * by R. Falque
 * 26/09/2019
 **/

#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <Eigen/Core>

#include <string>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include "EigenTools/nanoflannWrapper.h"

// polyscope wrapper
class ColorPalette
{
    private:
        Eigen::MatrixXd available_colors_; 
        nanoflann_wrapper *color_kdtree;

    public:

        ColorPalette()
        {
            init();
        }

        // destructor
        ~ColorPalette()
        {
        }

        void init() {

            // store variables in private variables
            available_colors_.resize(3,16);
            available_colors_.col( 0) << 255, 255, 255;
            available_colors_.col( 1) << 216, 127, 51;
            available_colors_.col( 2) << 178, 76, 216;
            available_colors_.col( 3) << 102, 153, 216;
            available_colors_.col( 4) << 229, 229, 51;
            available_colors_.col( 5) << 127, 204, 25;
            available_colors_.col( 6) << 242, 127, 165;
            available_colors_.col( 7) << 76, 76, 76;
            available_colors_.col( 8) << 153, 153, 153;
            available_colors_.col( 9) << 76, 127, 153;
            available_colors_.col(10) << 127, 63, 178;
            available_colors_.col(11) << 51, 76, 178;
            available_colors_.col(12) << 102, 76, 51;
            available_colors_.col(13) << 102, 127, 51;
            available_colors_.col(14) << 153, 51, 51;
            available_colors_.col(15) << 25, 25, 25;
            color_kdtree = new nanoflann_wrapper(available_colors_);

        }

        int get_closest_color_id(Eigen::Vector3d queried_color) {
            std::vector <int> color_id;
            color_id = color_kdtree->return_k_closest_points(queried_color, 1);
            return color_id[0];
        }

        // print the occupancy grid into a yaml file
        inline bool print_palette(std::ofstream & out_file) {

            out_file << "  - palette: !list_compound\n";
            out_file << "    - - Name: minecraft:white_concrete\n";
            out_file << "    - - Name: minecraft:orange_concrete\n";
            out_file << "    - - Name: minecraft:magenta_concrete\n";
            out_file << "    - - Name: minecraft:light_blue_concrete\n";
            out_file << "    - - Name: minecraft:yellow_concrete\n";
            out_file << "    - - Name: minecraft:lime_concrete\n";
            out_file << "    - - Name: minecraft:pink_concrete\n";
            out_file << "    - - Name: minecraft:gray_concrete\n";
            out_file << "    - - Name: minecraft:light_gray_concrete\n";
            out_file << "    - - Name: minecraft:cyan_concrete\n";
            out_file << "    - - Name: minecraft:purple_concrete\n";
            out_file << "    - - Name: minecraft:blue_concrete\n";
            out_file << "    - - Name: minecraft:brown_concrete\n";
            out_file << "    - - Name: minecraft:green_concrete\n";
            out_file << "    - - Name: minecraft:red_concrete\n";
            out_file << "    - - Name: minecraft:black_concrete\n";

            return true;
        }
};


#endif
