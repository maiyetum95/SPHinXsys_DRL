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
 * @file 	fluid_dynamics_complex.hpp
 * @brief 	Here, we define the algorithm classes for complex fluid dynamics,
 * 			which is involving with either solid walls (with suffix WithWall)
 * 			or/and other bodies treated as wall for the fluid (with suffix Complex).
 * @author	Chi ZHang and Xiangyu Hu
 * @version	1.0
 *			Try to implement EIGEN libaary for base vector, matrix and 
 *			linear algebra operation.  
 *			-- Chi ZHANG
 */
#pragma once

#include "fluid_dynamics_complex.h"

namespace SPH
{
	//=====================================================================================================//
	namespace fluid_dynamics
	{
		//=================================================================================================//
		template <class BaseRelaxationType>
		template <class BaseBodyRelationType>
		RelaxationWithWall<BaseRelaxationType>::RelaxationWithWall(BaseBodyRelationType &base_body_relation,
							   BaseBodyRelationContact &wall_contact_relation)
			: BaseRelaxationType(base_body_relation), FluidWallData(wall_contact_relation)
		{
			if (&base_body_relation.sph_body_ != &wall_contact_relation.sph_body_)
			{
				std::cout << "\n Error: the two body_relations do not have the same source body!" << std::endl;
				std::cout << __FILE__ << ':' << __LINE__ << std::endl;
				exit(1);
			}

			for (size_t k = 0; k != FluidWallData::contact_particles_.size(); ++k)
			{
				Real rho0_k = FluidWallData::contact_particles_[k]->rho0_;
				wall_inv_rho0_.push_back(1.0 / rho0_k);
				wall_mass_.push_back(&(FluidWallData::contact_particles_[k]->mass_));
				wall_Vol_.push_back(&(FluidWallData::contact_particles_[k]->Vol_));
				wall_vel_ave_.push_back(FluidWallData::contact_particles_[k]->AverageVelocity());
				wall_acc_ave_.push_back(FluidWallData::contact_particles_[k]->AverageAcceleration());
				wall_n_.push_back(&(FluidWallData::contact_particles_[k]->n_));
			}
		}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		DensitySummation<DensitySummationInnerType>::DensitySummation(BaseBodyRelationInner &inner_relation, 
																	  BaseBodyRelationContact &contact_relation)
			: ParticleDynamicsComplex<DensitySummationInnerType, FluidContactData>(inner_relation, contact_relation)
		{
			prepareContactData();
		}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		DensitySummation<DensitySummationInnerType>::DensitySummation(ComplexBodyRelation &complex_relation)
			: DensitySummation(complex_relation.inner_relation_, complex_relation.contact_relation_) {}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		DensitySummation<DensitySummationInnerType>::DensitySummation(ComplexBodyRelation &complex_relation, 
																	  BaseBodyRelationContact &extra_contact_relation)
			: ParticleDynamicsComplex<DensitySummationInnerType, FluidContactData>(complex_relation, extra_contact_relation)
		{
			prepareContactData();
		}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		void DensitySummation<DensitySummationInnerType>::prepareContactData()
		{
			for (size_t k = 0; k != this->contact_particles_.size(); ++k)
			{
				Real rho0_k = this->contact_particles_[k]->rho0_;
				contact_inv_rho0_.push_back(1.0 / rho0_k);
				contact_mass_.push_back(&(this->contact_particles_[k]->mass_));
			}
		}
		//=================================================================================================//
		template <class DensitySummationInnerType>
		void DensitySummation<DensitySummationInnerType>::interaction(size_t index_i, Real dt)
		{
			DensitySummationInnerType::interaction(index_i, dt);

			/** Contact interaction. */
			Real sigma(0.0);
			Real inv_Vol_0_i = this->rho0_ / this->mass_[index_i];
			for (size_t k = 0; k < this->contact_configuration_.size(); ++k)
			{
				StdLargeVec<Real> &contact_mass_k = *(this->contact_mass_[k]);
				Real contact_inv_rho0_k = contact_inv_rho0_[k];
				Neighborhood &contact_neighborhood = (*this->contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					sigma += contact_neighborhood.W_ij_[n] * inv_Vol_0_i * contact_inv_rho0_k * contact_mass_k[contact_neighborhood.j_[n]];
				}
			}
			this->rho_sum_[index_i] += sigma * this->rho0_ * this->inv_sigma0_;
		}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		template <class BaseBodyRelationType>
		ViscousWithWall<ViscousAccelerationInnerType>::ViscousWithWall(BaseBodyRelationType &base_body_relation,
																 	   BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<ViscousAccelerationInnerType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		void ViscousWithWall<ViscousAccelerationInnerType>::interaction(size_t index_i, Real dt)
		{
			ViscousAccelerationInnerType::interaction(index_i, dt);

			Real rho_i = this->rho_[index_i];
			const Vecd &vel_i = this->vel_[index_i];

			Vecd acceleration = Vecd::Zero();
			Vecd vel_derivative = Vecd::Zero();
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				StdLargeVec<Real> &Vol_k = *(this->wall_Vol_[k]);
				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				Neighborhood &contact_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					size_t index_j = contact_neighborhood.j_[n];
					Real r_ij = contact_neighborhood.r_ij_[n];

					vel_derivative = 2.0 * (vel_i - vel_ave_k[index_j]) / (r_ij + 0.01 * this->smoothing_length_);
					acceleration += 2.0 * this->mu_ * vel_derivative * contact_neighborhood.dW_ij_[n] * Vol_k[index_j] / rho_i;
				}
			}

			this->acc_prior_[index_i] += acceleration;
		}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		BaseViscousAccelerationWithWall<ViscousAccelerationInnerType>::
			BaseViscousAccelerationWithWall(ComplexBodyRelation &fluid_wall_relation)
			: ViscousAccelerationInnerType(fluid_wall_relation.inner_relation_,
										  fluid_wall_relation.contact_relation_) {}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		BaseViscousAccelerationWithWall<ViscousAccelerationInnerType>::
			BaseViscousAccelerationWithWall(BaseBodyRelationInner &fluid_inner_relation,
											BaseBodyRelationContact &wall_contact_relation)
			: ViscousAccelerationInnerType(fluid_inner_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class ViscousAccelerationInnerType>
		BaseViscousAccelerationWithWall<ViscousAccelerationInnerType>::
			BaseViscousAccelerationWithWall(ComplexBodyRelation &fluid_complex_relation,
											BaseBodyRelationContact &wall_contact_relation)
			: ViscousAccelerationInnerType(fluid_complex_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		template <class BaseBodyRelationType>
		PressureRelaxation<BasePressureRelaxationType>::
			PressureRelaxation(BaseBodyRelationType &base_body_relation,
							   BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<BasePressureRelaxationType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void PressureRelaxation<BasePressureRelaxationType>::interaction(size_t index_i, Real dt)
		{
			BasePressureRelaxationType::interaction(index_i, dt);

			FluidState state_i(this->rho_[index_i], this->vel_[index_i], this->p_[index_i]);
			Vecd acc_prior_i = computeNonConservativeAcceleration(index_i);

			Vecd acceleration = Vecd::Zero();
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				StdLargeVec<Real> &Vol_k = *(this->wall_Vol_[k]);
				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				StdLargeVec<Vecd> &acc_ave_k = *(this->wall_acc_ave_[k]);
				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real dW_ij = wall_neighborhood.dW_ij_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];

					Real face_wall_external_acceleration = (acc_prior_i - acc_ave_k[index_j]).dot(-e_ij);
					Vecd vel_in_wall = 2.0 * vel_ave_k[index_j] - state_i.vel_;
					Real p_in_wall = state_i.p_ + state_i.rho_ * r_ij * SMAX(0.0, face_wall_external_acceleration);
					Real rho_in_wall = this->material_->DensityFromPressure(p_in_wall);
					FluidState state_j(rho_in_wall, vel_in_wall, p_in_wall);
					Real p_star = this->riemann_solver_.getPStar(state_i, state_j, n_k[index_j]);
					acceleration -= 2.0 * p_star * e_ij * Vol_k[index_j] * dW_ij / state_i.rho_;
				}
			}
			this->acc_[index_i] += acceleration;
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		Vecd PressureRelaxation<BasePressureRelaxationType>::computeNonConservativeAcceleration(size_t index_i)
		{
			return this->acc_prior_[index_i];
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		template <class BaseBodyRelationType>
		ExtendPressureRelaxation<BasePressureRelaxationType>::
			ExtendPressureRelaxation(BaseBodyRelationType &base_body_relation,
									 BaseBodyRelationContact &wall_contact_relation, Real penalty_strength)
			: PressureRelaxation<BasePressureRelaxationType>(base_body_relation, wall_contact_relation),
			  penalty_strength_(penalty_strength)
		{
			this->particles_->registerVariable(non_cnsrv_acc_, "NonConservativeAcceleration");
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void ExtendPressureRelaxation<BasePressureRelaxationType>::initialization(size_t index_i, Real dt)
		{
			BasePressureRelaxationType::initialization(index_i, dt);
			non_cnsrv_acc_[index_i] = Vecd::Zero();
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		void ExtendPressureRelaxation<BasePressureRelaxationType>::interaction(size_t index_i, Real dt)
		{
			PressureRelaxation<BasePressureRelaxationType>::interaction(index_i, dt);

			Real rho_i = this->rho_[index_i];
			Real penalty_pressure = this->p_[index_i];
			Vecd acceleration = Vecd::Zero();
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				Real particle_spacing_j1 = 1.0 / FluidWallData::contact_bodies_[k]->sph_adaptation_->ReferenceSpacing();
				Real particle_spacing_ratio2 = 1.0 / (this->body_->sph_adaptation_->ReferenceSpacing() * particle_spacing_j1);
				particle_spacing_ratio2 *= 0.1 * particle_spacing_ratio2;

				StdLargeVec<Real> &Vol_k = *(this->wall_Vol_[k]);
				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real dW_ij = wall_neighborhood.dW_ij_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];
					Vecd &n_j = n_k[index_j];

					/** penalty method to prevent particle running into boundary */
					Real projection = e_ij.dot(n_j);
					Real delta = 2.0 * projection * r_ij * particle_spacing_j1;
					Real beta = delta < 1.0 ? (1.0 - delta) * (1.0 - delta) * particle_spacing_ratio2 : 0.0;
					//penalty must be positive so that the penalty force is pointed to fluid inner domain
					Real penalty = penalty_strength_ * beta * fabs(projection * penalty_pressure);

					//penalty force induced acceleration
					acceleration -= 2.0 * penalty * n_j * Vol_k[index_j] * dW_ij / rho_i;
				}
			}
			this->acc_[index_i] += acceleration;
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		Vecd ExtendPressureRelaxation<BasePressureRelaxationType>::
			computeNonConservativeAcceleration(size_t index_i)
		{
			Vecd acceleration = BasePressureRelaxationType::computeNonConservativeAcceleration(index_i);
			non_cnsrv_acc_[index_i] = acceleration;
			return acceleration;
		}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		BasePressureRelaxationWithWall<BasePressureRelaxationType>::
			BasePressureRelaxationWithWall(ComplexBodyRelation &fluid_wall_relation)
			: BasePressureRelaxationType(fluid_wall_relation.inner_relation_,
										 fluid_wall_relation.contact_relation_) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		BasePressureRelaxationWithWall<BasePressureRelaxationType>::
			BasePressureRelaxationWithWall(BaseBodyRelationInner &fluid_inner_relation,
										   BaseBodyRelationContact &wall_contact_relation)
			: BasePressureRelaxationType(fluid_inner_relation,
										 wall_contact_relation) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		BasePressureRelaxationWithWall<BasePressureRelaxationType>::
			BasePressureRelaxationWithWall(ComplexBodyRelation &fluid_complex_relation,
										   BaseBodyRelationContact &wall_contact_relation)
			: BasePressureRelaxationType(fluid_complex_relation,
										 wall_contact_relation) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		ExtendPressureRelaxationWithWall<BasePressureRelaxationType>::
			ExtendPressureRelaxationWithWall(ComplexBodyRelation &fluid_wall_relation, Real penalty_strength)
			: BasePressureRelaxationType(fluid_wall_relation.inner_relation_,
										 fluid_wall_relation.contact_relation_, penalty_strength) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		ExtendPressureRelaxationWithWall<BasePressureRelaxationType>::
			ExtendPressureRelaxationWithWall(BaseBodyRelationInner &fluid_inner_relation,
											 BaseBodyRelationContact &wall_contact_relation, Real penalty_strength)
			: BasePressureRelaxationType(fluid_inner_relation,
										 wall_contact_relation, penalty_strength) {}
		//=================================================================================================//
		template <class BasePressureRelaxationType>
		ExtendPressureRelaxationWithWall<BasePressureRelaxationType>::
			ExtendPressureRelaxationWithWall(ComplexBodyRelation &fluid_complex_relation,
											 BaseBodyRelationContact &wall_contact_relation, Real penalty_strength)
			: BasePressureRelaxationType(fluid_complex_relation,
										 wall_contact_relation, penalty_strength) {}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		template <class BaseBodyRelationType>
		DensityRelaxation<BaseDensityRelaxationType>::
			DensityRelaxation(BaseBodyRelationType &base_body_relation,
							  BaseBodyRelationContact &wall_contact_relation)
			: RelaxationWithWall<BaseDensityRelaxationType>(base_body_relation, wall_contact_relation) {}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		void DensityRelaxation<BaseDensityRelaxationType>::interaction(size_t index_i, Real dt)
		{
			BaseDensityRelaxationType::interaction(index_i, dt);

			FluidState state_i(this->rho_[index_i], this->vel_[index_i], this->p_[index_i]);
			Real density_change_rate = 0.0;
			for (size_t k = 0; k < FluidWallData::contact_configuration_.size(); ++k)
			{
				Vecd &acc_prior_i = this->acc_prior_[index_i];

				StdLargeVec<Real> &Vol_k = *(this->wall_Vol_[k]);
				StdLargeVec<Vecd> &vel_ave_k = *(this->wall_vel_ave_[k]);
				StdLargeVec<Vecd> &acc_ave_k = *(this->wall_acc_ave_[k]);
				StdLargeVec<Vecd> &n_k = *(this->wall_n_[k]);
				Neighborhood &wall_neighborhood = (*FluidWallData::contact_configuration_[k])[index_i];
				for (size_t n = 0; n != wall_neighborhood.current_size_; ++n)
				{
					size_t index_j = wall_neighborhood.j_[n];
					Vecd &e_ij = wall_neighborhood.e_ij_[n];
					Real r_ij = wall_neighborhood.r_ij_[n];
					Real dW_ij = wall_neighborhood.dW_ij_[n];

					Real face_wall_external_acceleration = (acc_prior_i - acc_ave_k[index_j]).dot(-e_ij);
					Vecd vel_in_wall = 2.0 * vel_ave_k[index_j] - state_i.vel_;
					Real p_in_wall = state_i.p_ + state_i.rho_ * r_ij * SMAX(0.0, face_wall_external_acceleration);
					Real rho_in_wall = this->material_->DensityFromPressure(p_in_wall);
					FluidState state_j(rho_in_wall, vel_in_wall, p_in_wall);
					Vecd vel_star = this->riemann_solver_.getVStar(state_i, state_j, n_k[index_j]);
					density_change_rate += 2.0 * state_i.rho_ * Vol_k[index_j] * (state_i.vel_ - vel_star).dot(e_ij) * dW_ij;
				}
			}
			this->drho_dt_[index_i] += density_change_rate;
		}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		BaseDensityRelaxationWithWall<BaseDensityRelaxationType>::
			BaseDensityRelaxationWithWall(ComplexBodyRelation &fluid_wall_relation)
			: DensityRelaxation<BaseDensityRelaxationType>(fluid_wall_relation.inner_relation_,
														   fluid_wall_relation.contact_relation_) {}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		BaseDensityRelaxationWithWall<BaseDensityRelaxationType>::
			BaseDensityRelaxationWithWall(BaseBodyRelationInner &fluid_inner_relation,
										  BaseBodyRelationContact &wall_contact_relation)
			: DensityRelaxation<BaseDensityRelaxationType>(fluid_inner_relation,
														   wall_contact_relation) {}
		//=================================================================================================//
		template <class BaseDensityRelaxationType>
		BaseDensityRelaxationWithWall<BaseDensityRelaxationType>::
			BaseDensityRelaxationWithWall(ComplexBodyRelation &fluid_complex_relation,
										  BaseBodyRelationContact &wall_contact_relation)
			: DensityRelaxation<BaseDensityRelaxationType>(fluid_complex_relation, wall_contact_relation) {}
		//=================================================================================================//
	}
	//=================================================================================================//
}
//=================================================================================================//