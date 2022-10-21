/* -------------------------------------------------------------------------*
 *								SPHinXsys									*
 * -------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle*
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
 * physical accurate simulation and aims to model coupled industrial dynamic*
 * systems including fluid, solid, multi-body dynamics and beyond with SPH	*
 * (smoothed particle hydrodynamics), a meshless computational method using	*
 * particle discretization.													*
 *																			*
 * SPHinXsys is partially funded by German Research Foundation				*
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,			*
 *  HU1527/12-1 and Hu1527/12-4												*
 *                                                                          *
 * Portions copyright (c) 2017-2020 Technical University of Munich and		*
 * the authors' affiliations.												*
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may  *
 * not use this file except in compliance with the License. You may obtain a*
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.       *
 *                                                                          *
 * ------------------------------------------------------------------------*/
/**
* @file 	triangle_mesh_shape.h
* @brief 	Here, we define the 3D geometric algorithms. they are based on the polymesh. 
* @details 	The idea is to define complex geometry by passing stl, obj or other 
*			polymesh files. TODO: the translation needs to be generalized into transform.
* @author	Chi ZHang and Xiangyu Hu
 * @version	1.0
 *			Try to implement EIGEN libaary for base vector, matrix and 
 *			linear algebra operation.  
 *			-- Chi ZHANG
 */

#ifndef TRIANGULAR_MESH_SHAPE_H
#define TRIANGULAR_MESH_SHAPE_H

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include "base_geometry.h"
#include "simbody_middle.h"

#include <iostream>
#include <string>
#include <fstream>

/** Macro for APPLE compilers*/
#ifdef __APPLE__
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace SPH
{
	/**
	 * @class TriangleMeshShape
	 * @brief Drived class for triangle shape processing. 
	 */
	class TriangleMeshShape : public Shape
	{
	private:
		UniquePtrKeeper<SimTK::ContactGeometry::TriangleMesh> triangle_mesh_ptr_keeper_;

	public:
		explicit TriangleMeshShape(const std::string &shape_name, const SimTK::PolygonalMesh* mesh = nullptr)
			: Shape(shape_name), triangle_mesh_(nullptr){
                if(mesh)
                    triangle_mesh_ = generateTriangleMesh(*mesh);

            };

		virtual bool checkContain(const Vecd &pnt, bool BOUNDARY_INCLUDED = true) override;
		virtual Vecd findClosestPoint(const Vecd &probe_point) override;

		SimTK::ContactGeometry::TriangleMesh *getTriangleMesh() { return triangle_mesh_; };

	protected:
		SimTK::ContactGeometry::TriangleMesh *triangle_mesh_;

		SimTK::ContactGeometry::TriangleMesh *generateTriangleMesh(const SimTK::PolygonalMesh &poly_mesh);
		virtual BoundingBox findBounds() override;
	};

	/**
	 * @class TriangleMeshShapeSTL
	 * @brief Input triangle mesh with stl file. 
	 */
	class TriangleMeshShapeSTL : public TriangleMeshShape
	{
	public:
		explicit TriangleMeshShapeSTL(const std::string &file_path_name, Vecd translation, Real scale_factor,
									  const std::string &shape_name = "TriangleMeshShapeSTL");
		explicit TriangleMeshShapeSTL(const std::string &file_path_name, Mat3d rotation, Vecd translation,
										Real scale_factor, const std::string &shape_name = "TriangleMeshShapeSTL");
		#ifdef __EMSCRIPTEN__
		TriangleMeshShapeSTL(const uint8_t* buffer, Vecd translation, Real scale_factor, const std::string &shape_name = "TriangleMeshShapeSTL");
		#endif
		virtual ~TriangleMeshShapeSTL(){};
	};

	/**
	 * @class TriangleMeshShapeBrick
	 * @brief Generat a brick trianle meshe using SIMBODy default shape.
	 */
	class TriangleMeshShapeBrick : public TriangleMeshShape
	{
	public:
		class ShapeParameters 
		{
		public:
			ShapeParameters() : halfsize_(Vecd::Zero()), resolution_(0), translation_(Vecd::Zero()) {};
			Vecd halfsize_;
			Vecd translation_;
			int resolution_;
		};
		explicit TriangleMeshShapeBrick(Vecd halfsize, int resolution, Vecd translation,
										const std::string &shape_name = "TriangleMeshShapeBrick");
		explicit TriangleMeshShapeBrick(const TriangleMeshShapeBrick::ShapeParameters &shape_parameters,
										const std::string &shape_name = "TriangleMeshShapeBrick");
		virtual ~TriangleMeshShapeBrick(){};
	};

	/**
	 * @class TriangleMeshShapeSphere
	 * @brief Generat a sphere trianle meshe using SIMBODy default shape.
	 */
	class TriangleMeshShapeSphere : public TriangleMeshShape
	{
	public:
		explicit TriangleMeshShapeSphere(Real radius, int resolution, Vecd translation,
										const std::string &shape_name = "TriangleMeshShapeSphere");
		virtual ~TriangleMeshShapeSphere(){};
	};

	/**
	 * @class TriangleMeshShapeCylinder
	 * @brief Generat a cylinder trianle meshe using SIMBODy default shape.
	 */
	class TriangleMeshShapeCylinder : public TriangleMeshShape
	{
	public:
		explicit TriangleMeshShapeCylinder(SimTK::UnitVec3 axis, Real radius, Real halflength, int resolution, Vecd translation,
										   const std::string &shape_name = "TriangleMeshShapeCylinder");
		virtual ~TriangleMeshShapeCylinder(){};
	};
}

#endif //TRIANGULAR_MESH_SHAPE_H
