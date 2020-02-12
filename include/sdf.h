/*
*   Generate SDF
*   by R. Falque
*   11/02/2020
*/

#ifndef SDF_H
#define SDF_H

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "EigenTools/getMinMax.h"
#include "EigenTools/nanoflannWrapper.h"
#include "sgn.h"
#include "IO/writePNG.h"
#include "IO/process_folder.h"

// polyscope wrapper
class SDF
{
    private:
        Eigen::MatrixXd vertices_; 
        Eigen::MatrixXd normals_;
        int grid_resolution_;
        double bounding_box_scale_;
        Eigen::Tensor<double, 3> SDF_;
        double grid_size_;
        Eigen::Vector3d source_;

    public:

        SDF(Eigen::MatrixXd & vertices, Eigen::MatrixXd & normals, int grid_resolution, double bounding_box_scale)
        {
            vertices_ = vertices;
            normals_ = normals;
            grid_resolution_ = grid_resolution;
            bounding_box_scale_ = bounding_box_scale;

            init();
        }

        // destructor
        ~SDF()
        {
        }

        //accessors
        inline Eigen::Tensor<double, 3> get_SDF(){return SDF_;};
        inline double get_grid_size(){return grid_size_;};
        inline Eigen::Vector3d get_source(){return source_;};

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

            SDF_.resize(number_of_bins(0), number_of_bins(1), number_of_bins(2));

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
                        double sign = ( vertices_.col(closest_point[0]) - point ).dot( normals_.col(closest_point[0]) );
                        sign /= abs(sign);

                        SDF_(x, y, z) = ( vertices_.col(closest_point[0]) - point ).norm() * sign;
                    }
                }
            
            grid_size_ = leaf_size;
            source_ = min_point;
        }

        inline bool generate_graph(Eigen::MatrixXd & vertices, Eigen::MatrixXi & edges)
        {
            std::vector< Eigen::Vector3d > vertices_vector;
            std::vector< Eigen::Vector2i > edges_vector;

            Eigen::Vector3d centroid;
            Eigen::Tensor<int, 3> grid_indices(SDF_.dimension(0), SDF_.dimension(1), SDF_.dimension(2));

            // build vertices
            for (int x = 0; x < SDF_.dimension(0); ++x)
                for (int y = 0; y < SDF_.dimension(1); ++y)
                    for (int z = 0; z < SDF_.dimension(2); ++z) {
                        centroid << x, y, z;
                        centroid *= grid_size_;
                        centroid += source_;
                        vertices_vector.push_back(centroid);
                        grid_indices(x, y, z) = vertices_vector.size();
                    }
            
            // build edges
            for (int x = 0; x < SDF_.dimension(0); ++x)
                for (int y = 0; y < SDF_.dimension(1); ++y)
                    for (int z = 0; z < SDF_.dimension(2); ++z) 
                    {
                            // case on x
                            if (x-1>=0) {
                                Eigen::Vector2i edge_temp;
                                edge_temp << grid_indices(x, y, z), grid_indices(x-1, y, z);
                                edges_vector.push_back(edge_temp);
                            }
                            if (x+1<SDF_.dimension(0)) {
                                Eigen::Vector2i edge_temp;
                                edge_temp << grid_indices(x, y, z), grid_indices(x+1, y, z);
                                edges_vector.push_back(edge_temp);
                            }
                            
                            // case on y
                            if (y-1>=0) {
                                Eigen::Vector2i edge_temp;
                                edge_temp << grid_indices(x, y, z), grid_indices(x, y-1, z);
                                edges_vector.push_back(edge_temp);
                            }
                            if (y+1<SDF_.dimension(1)) {
                                Eigen::Vector2i edge_temp;
                                edge_temp << grid_indices(x, y, z), grid_indices(x, y+1, z);
                                edges_vector.push_back(edge_temp);
                            }
                            
                            // case on z
                            if (z-1>=0) {
                                Eigen::Vector2i edge_temp;
                                edge_temp << grid_indices(x, y, z), grid_indices(x, y, z-1);
                                edges_vector.push_back(edge_temp);
                            }
                            if (z+1<SDF_.dimension(2)) {
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

        inline bool print_to_folder(std::string folder_name)
        {

            bool folder_exist = does_folder_exist(folder_name);
            if (!folder_exist) {
                std::cout << "Error: the folder does not exist\n";
                create_folder(folder_name);
            }
            
            empty_folder(folder_name);

            #pragma omp parallel for
            for (int i=0; i<SDF_.dimension(2); i++) {  
                Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>  slice;
                Eigen::Tensor<double, 2> tensor_slice;  

                Eigen::array<long int,3> offset = {0,0,i};         //Starting point
                Eigen::array<long int,3> extent = {SDF_.dimension(0),SDF_.dimension(1),0};       //Finish point   
                tensor_slice = SDF_.slice(offset, extent).reshape(Eigen::array<long int,2>{SDF_.dimension(0),SDF_.dimension(1)});
                slice = Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> (tensor_slice.data(), tensor_slice.dimension(0),tensor_slice.dimension(1));

                //slice /= slice.maxCoeff() ;

                Eigen::MatrixXd R, G, B;
                R = slice;
                R = R.cwiseMax(0);
                R /= R.maxCoeff();

                G = -slice;
                G = G.cwiseMax(0);
                G /= G.maxCoeff();
                B = G;
                
                std::stringstream ss;
                ss << std::setw(3) << std::setfill('0') << i;
                std::string s = ss.str();

                std::string file_name = folder_name + s + ".png";
                writePNG(R, G, B, file_name);
            }

            std::cout << "Progress: Stack of images written in :" << folder_name << std::endl;

            return true;
        };

};

#endif