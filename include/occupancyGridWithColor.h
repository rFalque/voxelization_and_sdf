/*
*   Generate 3D occupancy grid from a 3D closed mesh
*   by R. Falque
*   07/02/2020
*/

#ifndef OCCUPANCY_GRID_H
#define OCCUPANCY_GRID_H

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "EigenTools/getMinMax.h"
#include "EigenTools/nanoflannWrapper.h"
#include "sgn.h"
#include "IO/writePNG.h"
#include "IO/process_folder.h"

// polyscope wrapper
class OccupancyGridWithColor
{
    private:
        Eigen::MatrixXd vertices_; 
        Eigen::MatrixXd normals_;
        Eigen::MatrixXd RGB_;
        int grid_resolution_;
        double bounding_box_scale_;
        Eigen::Tensor<bool, 3> occupancy_grid_;
        Eigen::Tensor<double, 3> R_;
        Eigen::Tensor<double, 3> G_;
        Eigen::Tensor<double, 3> B_;
        double grid_size_;
        Eigen::Vector3d source_;

    public:

        OccupancyGridWithColor(Eigen::MatrixXd & vertices, Eigen::MatrixXd & normals, int grid_resolution, double bounding_box_scale)
        {
          // store variables in private variables
            vertices_ = vertices;
            normals_ = normals;
            grid_resolution_ = grid_resolution;
            bounding_box_scale_ = bounding_box_scale;

            // create the occupancy grid
            init();
        }

        OccupancyGridWithColor(Eigen::MatrixXd & vertices, Eigen::MatrixXd & normals, Eigen::MatrixXd & RGB, int grid_resolution, double bounding_box_scale)
        {
          // store variables in private variables
            vertices_ = vertices;
            normals_ = normals;
            RGB_ =  RGB;
            grid_resolution_ = grid_resolution;
            bounding_box_scale_ = bounding_box_scale;

            // create the occupancy grid
            init();
        }

        // destructor
        ~OccupancyGridWithColor()
        {
        }

        //accessors
        inline Eigen::Tensor<bool, 3> get_occupancy_grid(){return occupancy_grid_;};
        inline double get_grid_size(){return grid_size_;};
        inline Eigen::Vector3d get_source(){return source_;};

        // Class functions

        // create the occupancy grid
        void init() {
            Eigen::Vector3d min_point, max_point;
            getMinMax(vertices_, min_point, max_point);

            //double bounding_box_size = (max_point - min_point).norm() * bounding_box_scale;
            double bounding_box_size = (max_point - min_point).maxCoeff() * bounding_box_scale_; // diagonal versus max direction
            double leaf_size = bounding_box_size/(grid_resolution_-1);
            double inv_leaf_size = 1.0/leaf_size;

            Eigen::Vector3i min_box, max_box, number_of_bins;
            min_box << floor(min_point(0)*inv_leaf_size), floor(min_point(1)*inv_leaf_size) , floor(min_point(2)*inv_leaf_size); 
            max_box << floor(max_point(0)*inv_leaf_size), floor(max_point(1)*inv_leaf_size) , floor(max_point(2)*inv_leaf_size); 
            number_of_bins << max_box(0) - min_box(0) + 1, max_box(1) - min_box(1) + 1, max_box(2) - min_box(2) + 1;
            
            occupancy_grid_.resize(number_of_bins(0), number_of_bins(1), number_of_bins(2));
            R_.resize(number_of_bins(0), number_of_bins(1), number_of_bins(2));
            G_.resize(number_of_bins(0), number_of_bins(1), number_of_bins(2));
            B_.resize(number_of_bins(0), number_of_bins(1), number_of_bins(2));
              
            nanoflann_wrapper tree(vertices_);
            for (int x = 0; x < number_of_bins(0); ++x)
                for (int y = 0; y < number_of_bins(1); ++y)
                {
                    #pragma omp parallel for
                    for (int z = 0; z < number_of_bins(2); ++z)
                    {
                        std::vector< int > closest_point;
                        Eigen::Vector3d point;
                        point << x, y, z;
                        point *= leaf_size;
                        point += min_point;
                        closest_point = tree.return_k_closest_points(point, 1);
                        
                        /* produce the outer shell only remove the next line
                        if ( (point - vertices.row(closest_point[0]).transpose()).norm() < leaf_size(0)*2 )
                            grid(x, y, z) = true;
                        else
                            grid(x, y, z) = false;
                        */
                        // here is the key function
                        occupancy_grid_(x, y, z) = is_positive( ( vertices_.col(closest_point[0]) - point ).dot( normals_.col(closest_point[0]) ) );
                        R_(x, y, z) = RGB_(0, closest_point[0]);
                        G_(x, y, z) = RGB_(1, closest_point[0]);
                        B_(x, y, z) = RGB_(2, closest_point[0]);
                    }
                }
              
            grid_size_ = leaf_size;
            source_ = min_point;
        }

        // build a graph from the occupied space (there is no garanty of connectivity)
        inline bool generate_graph(Eigen::MatrixXd & vertices, Eigen::MatrixXi & edges) {

            std::vector< Eigen::Vector3d > vertices_vector;
            std::vector< Eigen::Vector2i > edges_vector;

            Eigen::Vector3d centroid;
            Eigen::Tensor<int, 3> grid_indices(occupancy_grid_.dimension(0), occupancy_grid_.dimension(1), occupancy_grid_.dimension(2));

            // build vertices
            for (int x = 0; x < occupancy_grid_.dimension(0); ++x)
                for (int y = 0; y < occupancy_grid_.dimension(1); ++y)
                    for (int z = 0; z < occupancy_grid_.dimension(2); ++z) {
                        grid_indices(x, y, z) = -1;
                        if (occupancy_grid_(x, y, z) == 1) {
                            centroid << x, y, z;
                            centroid *= grid_size_;
                            centroid += source_;
                            vertices_vector.push_back(centroid);
                            grid_indices(x, y, z) = vertices_vector.size();
                        }
                    }
            
            // build edges
            for (int x = 0; x < occupancy_grid_.dimension(0); ++x)
                for (int y = 0; y < occupancy_grid_.dimension(1); ++y)
                    for (int z = 0; z < occupancy_grid_.dimension(2); ++z)
                        if (occupancy_grid_(x, y, z) == 1) {
                            // case on x
                            if (x-1>=0)
                                if (occupancy_grid_(x-1, y, z)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x-1, y, z);
                                    edges_vector.push_back(edge_temp);
                                }
                            if (x+1<occupancy_grid_.dimension(0))
                                if (occupancy_grid_(x+1, y, z)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x+1, y, z);
                                    edges_vector.push_back(edge_temp);
                                }
                            
                            // case on y
                            if (y-1>=0)
                                if (occupancy_grid_(x, y, z-1)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x, y-1, z);
                                    edges_vector.push_back(edge_temp);
                                }
                            if (y+1<occupancy_grid_.dimension(1))
                                if (occupancy_grid_(x, y, z+1)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x, y+1, z);
                                    edges_vector.push_back(edge_temp);
                                }
                            
                            // case on z
                            if (z-1>=0)
                                if (occupancy_grid_(x, y, z-1)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x, y, z-1);
                                    edges_vector.push_back(edge_temp);
                                }
                            if (z+1<occupancy_grid_.dimension(2))
                                if (occupancy_grid_(x, y, z+1)==1) {
                                    Eigen::Vector2i edge_temp;
                                    edge_temp << grid_indices(x, y, z), grid_indices(x, y, z+1);
                                    edges_vector.push_back(edge_temp);
                                }
                        }

            
            vertices.resize(3, vertices_vector.size());
            for (int i=0; i< vertices_vector.size(); i++)
                vertices.col(i) = vertices_vector[i];

            edges.resize(2, edges_vector.size());
            for (int i=0; i< edges_vector.size(); i++)
                edges.col(i) = edges_vector[i];
            
            return true;
        };

        // print each slice as an image in the provided folder path
        inline bool print_to_folder(std::string folder_name) {

            bool folder_exist = does_folder_exist(folder_name);
            if (!folder_exist) {
                std::cout << "Error: the folder does not exist\n";
                create_folder(folder_name);
            }
            
            empty_folder(folder_name);

            #pragma omp parallel for
            for (int i=0; i<occupancy_grid_.dimension(2); i++) {  
                Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>  slice;
                Eigen::Tensor<bool, 2> tensor_slice;  

                Eigen::array<long int,3> offset = {0,0,i};         //Starting point
                Eigen::array<long int,3> extent = {occupancy_grid_.dimension(0),occupancy_grid_.dimension(1),0};       //Finish point   
                tensor_slice = occupancy_grid_.slice(offset, extent).reshape(Eigen::array<long int,2>{occupancy_grid_.dimension(0),occupancy_grid_.dimension(1)});
                slice = Eigen::Map<const Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>> (tensor_slice.data(), tensor_slice.dimension(0),tensor_slice.dimension(1));
        
                std::stringstream ss;
                ss << std::setw(3) << std::setfill('0') << i;
                std::string s = ss.str();

                std::string file_name = folder_name + s + ".png";
                writePNG(slice, file_name);
            }

            std::cout << "Progress: Stack of images written in :" << folder_name << std::endl;

            return true;
        };

        // print the occupancy grid into a yaml file
        inline bool print_to_yaml(std::string filename) {
            std::ofstream out_file(filename);
            out_file << "? ''\n";
            out_file << ": - size: !list_int\n";
            out_file << "    - " + std::to_string(occupancy_grid_.dimension(0)) + "\n";
            out_file << "    - " + std::to_string(occupancy_grid_.dimension(1)) + "\n";
            out_file << "    - " + std::to_string(occupancy_grid_.dimension(2)) + "\n";
            out_file << "  - blocks: !list_compound\n";
            for (int x = 0; x < occupancy_grid_.dimension(0); ++x)
                for (int y = 0; y < occupancy_grid_.dimension(1); ++y)
                    for (int z = 0; z < occupancy_grid_.dimension(2); ++z)
                    {
                        if (occupancy_grid_(x, y, z) == 1) {
                            out_file << "    - - pos: !list_int\n";
                            out_file << "        - " + std::to_string(x) + "\n";
                            out_file << "        - " + std::to_string(y) + "\n";
                            out_file << "        - " + std::to_string(z) + "\n";
                            out_file << "      - state: 0\n";
                        }

                    }
            out_file << "  - author: rFalque\n";
            out_file << "  - palette: !list_compound\n";
            out_file << "    - - Properties:\n";
            out_file << "        - variant: smooth_andesite\n";
            out_file << "      - Name: minecraft:stone\n";
            out_file << "  - DataVersion: 1139\n";
            out_file << "  - ForgeDataVersion:\n";
            out_file << "    - minecraft: 1139\n";

            out_file.close();

            return true;
        };

        inline bool generate_mesh(Eigen::MatrixXd & vertices, Eigen::MatrixXi & faces, Eigen::MatrixXd & colors) {
            std::vector< Eigen::Vector3d > vertices_vector;
            std::vector< Eigen::Vector3i > faces_vector;
            std::vector< Eigen::Vector3d > color_vector;

            Eigen::Vector3d centroid, color;
            for (int x = 0; x < occupancy_grid_.dimension(0); ++x)
                for (int y = 0; y < occupancy_grid_.dimension(1); ++y)
                    for (int z = 0; z < occupancy_grid_.dimension(2); ++z)
                    {
                        if (occupancy_grid_(x, y, z) == 1) {
                            centroid << x, y, z;
                            centroid *= grid_size_;
                            centroid += source_;
                            color << R_(x,y,z), G_(x,y,z), B_(x,y,z);

                            make_cube(centroid, color, grid_size_, vertices_vector, faces_vector, color_vector);
                        }
                    }
            
            vertices.resize(3, vertices_vector.size());
            for (int i=0; i< vertices_vector.size(); i++)
                vertices.col(i) = vertices_vector[i];
            
            colors.resize(3, color_vector.size());
            for (int i=0; i< color_vector.size(); i++)
                colors.col(i) = color_vector[i];

            faces.resize(3, faces_vector.size());
            for (int i=0; i< faces_vector.size(); i++)
                faces.col(i) = faces_vector[i];
            
            return true;
        };

        inline bool make_cube(
            Eigen::Vector3d centroid, 
            Eigen::Vector3d color, 
            double size, 
            std::vector< Eigen::Vector3d > & vertices_vector, 
            std::vector< Eigen::Vector3i > & faces_vector, 
            std::vector< Eigen::Vector3d > & color_vector
        ) {
          
            Eigen::Vector3i vertices_offset = Eigen::Vector3i::Constant(vertices_vector.size());

            Eigen::Vector3d v_0 ( 0.5, 0.5, 0.5);
            Eigen::Vector3d v_1 ( 0.5, 0.5,-0.5);
            Eigen::Vector3d v_2 ( 0.5,-0.5, 0.5);
            Eigen::Vector3d v_3 ( 0.5,-0.5,-0.5);
            Eigen::Vector3d v_4 (-0.5, 0.5, 0.5);
            Eigen::Vector3d v_5 (-0.5, 0.5,-0.5);
            Eigen::Vector3d v_6 (-0.5,-0.5, 0.5);
            Eigen::Vector3d v_7 (-0.5,-0.5,-0.5);

            vertices_vector.push_back( v_0*size + centroid );
            vertices_vector.push_back( v_1*size + centroid );
            vertices_vector.push_back( v_2*size + centroid );
            vertices_vector.push_back( v_3*size + centroid );
            vertices_vector.push_back( v_4*size + centroid );
            vertices_vector.push_back( v_5*size + centroid );
            vertices_vector.push_back( v_6*size + centroid );
            vertices_vector.push_back( v_7*size + centroid );

            for (int i=0; i<8; i++)
                color_vector.push_back(color);

            Eigen::Vector3i f_0 (0, 3, 1);
            Eigen::Vector3i f_1 (0, 2, 3);
            Eigen::Vector3i f_2 (0, 1, 5);
            Eigen::Vector3i f_3 (0, 5, 4);
            Eigen::Vector3i f_4 (4, 5, 7);
            Eigen::Vector3i f_5 (4, 7, 6);
            Eigen::Vector3i f_6 (2, 6, 7);
            Eigen::Vector3i f_7 (2, 7, 3);
            Eigen::Vector3i f_8 (1, 3, 7);
            Eigen::Vector3i f_9 (1, 7, 5);
            Eigen::Vector3i f_10(0, 4, 2);
            Eigen::Vector3i f_11(2, 4, 6);

            faces_vector.push_back(f_0  + vertices_offset);
            faces_vector.push_back(f_1  + vertices_offset);
            faces_vector.push_back(f_2  + vertices_offset);
            faces_vector.push_back(f_3  + vertices_offset);
            faces_vector.push_back(f_4  + vertices_offset);
            faces_vector.push_back(f_5  + vertices_offset);
            faces_vector.push_back(f_6  + vertices_offset);
            faces_vector.push_back(f_7  + vertices_offset);
            faces_vector.push_back(f_8  + vertices_offset);
            faces_vector.push_back(f_9  + vertices_offset);
            faces_vector.push_back(f_10 + vertices_offset);
            faces_vector.push_back(f_11 + vertices_offset);

            return true;
        };

};

#endif