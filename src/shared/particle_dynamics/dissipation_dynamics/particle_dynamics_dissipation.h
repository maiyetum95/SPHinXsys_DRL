/* ------------------------------------------------------------------------- *
 *                                SPHinXsys                                  *
 * ------------------------------------------------------------------------- *
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for    *
 * physical accurate simulation and aims to model coupled industrial dynamic *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH   *
 * (smoothed particle hydrodynamics), a meshless computational method using  *
 * particle discretization.                                                  *
 *                                                                           *
 * SPHinXsys is partially funded by German Research Foundation               *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,            *
 *  HU1527/12-1 and HU1527/12-4.                                             *
 *                                                                           *
 * Portions copyright (c) 2017-2023 Technical University of Munich and       *
 * the authors' affiliations.                                                *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may   *
 * not use this file except in compliance with the License. You may obtain a *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
/**
 * @file 	particle_dynamics_dissipation.h
 * @brief A quantity damping by operator splitting schemes.
 * These methods modify the quantity directly.
 * Note that, if periodic boundary condition is applied,
 * the parallelized version of the method requires the one using ghost particles
 * because the splitting partition only works in this case.
 * Note that, currently, these classes works only in single resolution.
 * @author	Chi Zhang and Xiangyu Hu
 */

#ifndef PARTICLE_DYNAMICS_DISSIPATION_H
#define PARTICLE_DYNAMICS_DISSIPATION_H

#include "all_particle_dynamics.h"

namespace SPH
{
/* Base class to indicate the concept of operator splitting */
class OperatorSplitting
{
};

template <typename VariableType>
struct ErrorAndParameters
{
    VariableType error_;
    Real a_, c_;
    ErrorAndParameters()
        : error_(ZeroData<VariableType>::value),
          a_(0), c_(0){};
};

class FixedDampingRate
{
  public:
    FixedDampingRate(BaseParticles *particles, Real eta, Real kappa = 1.0)
        : eta_(particles->registerSingleVariable<Real>("DampingRate", eta)),
          kappa_(particles->registerSingleVariable<Real>("SpecificCapacity", kappa)){};
    virtual ~FixedDampingRate(){};

    Real &DampingRate(size_t index_i, size_t index_j) { return *eta_; };
    Real &SpecificCapacity(size_t index_i) { return *kappa_; };

  protected:
    Real *eta_;
    Real *kappa_;
};

template <typename... InteractionTypes>
class Damping;

template <typename VariableType, typename DampingRateType, class DataDelegationType>
class Damping<Base, DampingRateType, VariableType, DataDelegationType>
    : public LocalDynamics, public DataDelegationType, public OperatorSplitting
{
  public:
    template <class BaseRelationType, typename... Args>
    explicit Damping(BaseRelationType &base_relation, const std::string &variable_name, Args &&...args);
    template <typename BodyRelationType, typename... Args, size_t... Is>
    Damping(ConstructorArgs<BodyRelationType, Args...> parameters, std::index_sequence<Is...>)
        : Damping(parameters.body_relation_, std::get<Is>(parameters.others_)...){};
    template <class BodyRelationType, typename... Args>
    explicit Damping(ConstructorArgs<BodyRelationType, Args...> parameters)
        : Damping(parameters, std::make_index_sequence<std::tuple_size_v<decltype(parameters.others_)>>{}){};
    virtual ~Damping(){};

  protected:
    DampingRateType damping_;
    StdLargeVec<Real> &Vol_, &mass_;
    StdLargeVec<VariableType> &variable_;
};

class Projection;
template <typename VariableType, typename DampingRateType>
class Damping<Inner<Projection>, VariableType, DampingRateType>
    : public Damping<Base, DampingRateType, VariableType, DataDelegateInner>
{
  public:
    template <typename... Args>
    Damping(Args &&...args)
        : Damping<Base, DampingRateType, VariableType, DataDelegateInner>(std::forward<Args>(args)...){};
    virtual ~Damping(){};

    inline void interaction(size_t index_i, Real dt = 0.0);

  protected:
    ErrorAndParameters<VariableType> computeErrorAndParameters(size_t index_i, Real dt = 0.0);
    void updateStates(size_t index_i, Real dt, const ErrorAndParameters<VariableType> &error_and_parameters);
};
template <typename VariableType, typename DampingRateType>
using DampingProjectionInner = Damping<Inner<Projection>, VariableType, DampingRateType>;

/**
 * @class DampingWithRandomChoice
 * @brief A random choice method for obtaining static equilibrium state
 * Note that, if periodic boundary condition is applied,
 * the parallelized version of the method requires the one using ghost particles
 * because the splitting partition only works in this case.
 */
template <class DampingAlgorithmType>
class DampingWithRandomChoice : public DampingAlgorithmType
{
  protected:
    Real random_ratio_;
    bool RandomChoice();

  public:
    template <typename... Args>
    DampingWithRandomChoice(Real random_ratio, Args &&...args);
    virtual ~DampingWithRandomChoice(){};

    virtual void exec(Real dt = 0.0) override;
};
} // namespace SPH
#endif // PARTICLE_DYNAMICS_DISSIPATION_H