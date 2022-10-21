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
 * @file 	geometric_shape.h
 * @brief 	Here, we define shapes represented directly by geometric elements.
 * @author	Chi ZHang and Xiangyu Hu
 * @version	1.0
 *			Try to implement EIGEN libaary for base vector, matrix and 
 *			linear algebra operation.  
 *			-- Chi ZHANG
 */

#ifndef GEOMETRIC_SHAPE_H
#define GEOMETRIC_SHAPE_H

#include "base_geometry.h"
#include "simbody_middle.h"

namespace SPH
{
	class GeometricShape : public Shape
	{
	public:
		explicit GeometricShape(const std::string &shape_name)
			: Shape(shape_name), contact_geometry_(nullptr){};

		virtual bool checkContain(const Vecd &pnt, bool BOUNDARY_INCLUDED = true) override;
		virtual Vecd findClosestPoint(const Vecd &pnt) override;

		SimTK::ContactGeometry *getContactGeometry() { return contact_geometry_; };

	protected:
		SimTK::ContactGeometry *contact_geometry_;
	};

	class GeometricShapeBox : public GeometricShape
	{
	private:
		SimTK::ContactGeometry::Brick brick_;

	public:
		explicit GeometricShapeBox(const Vecd &halfsize,
								   const std::string &shape_name = "GeometricShapeBox");
		virtual ~GeometricShapeBox(){};

		virtual bool checkContain(const Vecd &pnt, bool BOUNDARY_INCLUDED = true) override;
		virtual Vecd findClosestPoint(const Vecd &pnt) override;

	protected:
		Vecd halfsize_;

		virtual BoundingBox findBounds() override;
	};

	class GeometricShapeBall : public GeometricShape
	{
	private:
		Vecd center_;
		SimTK::ContactGeometry::Sphere sphere_;

	public:
		explicit GeometricShapeBall(const Vecd &center, const Real &radius,
									const std::string &shape_name = "GeometricShapeBall");
		virtual ~GeometricShapeBall(){};

		virtual bool checkContain(const Vecd &pnt, bool BOUNDARY_INCLUDED = true) override;
		virtual Vecd findClosestPoint(const Vecd &pnt) override;

	protected:
		virtual BoundingBox findBounds() override;
	};

}

#endif // GEOMETRIC_SHAPE_H
