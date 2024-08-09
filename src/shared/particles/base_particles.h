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
 * @file base_particles.h
 * @brief This is the base class of SPH particles containing data
 * and operation for all types of particles.
 * Note that there is no class of single particle.
 * TODO: It seems that I need to transfer the IO related functions to the IO classes.
 * TODO: Sorting related functions and data should be transferred to the sorting classes.
 * @author	Chi Zhang, Chenxi Zhao and Xiangyu Hu
 */

#ifndef BASE_PARTICLES_H
#define BASE_PARTICLES_H

#include "base_data_package.h"
#include "execution_policy.h"
#include "sphinxsys_containers.h"
#include "sphinxsys_entity.h"
#include "xml_parser.h"

#include <fstream>

namespace SPH
{

using namespace execution;
class SPHBody;
class BaseMaterial;
class BodySurface;

/**
 * @class BaseParticles
 * @brief Particles with essential (geometric and matter) data.
 * There are three groups of particles，all particles of a same type are saved with continuous memory segments.
 * The first is for real particles whose states are updated by particle dynamics.
 * One is buffer particles whose state are not updated by particle dynamics.
 * Buffer particles are saved behind real particles.
 * The global value of total_real_particles_ separate the real and buffer particles.
 * They may be switched from real particles or switch to real particles.
 * As the memory for both particles are continuous, such switch is achieved at the memory boundary sequentially.
 * The basic idea is swap the data of the last real particle with the one will be switched particle,
 * and then switch this swapped last particle as buffer particle by decrease the total_real_particles_ by one.
 * Switch from buffer particle to real particle is easy. One just need to assign expect state to
 * the first buffer particle and increase total_real_particles_ by one.
 * The third group is for ghost particles whose states are updated according to
 * boundary condition if their indices are included in the neighbor particle list.
 * Ghost particles whose states are updated according to
 * boundary condition if their indices are included in the neighbor particle list.
 * The ghost particles are saved behind the buffer particles in the form of one or more ghost bounds.
 * All particles are bounded by particle_bound_, which is the total number of particles in all types.
 * It will be initialized to zero before a time step.
 * In SPHinXsys, the discrete variables (state of each particle) registered in general particle data
 * (ParticleData) belong to a hierarchy of two layers.
 * The first is for the basic geometric and matter properties.
 * These variables are created of after particles are generated.
 * The second is for the local, dynamics-method-related variables, which are defined in specific methods,
 * and are only used by the relevant methods. Generally, a discrete variable is defined
 * and the corresponding data can use or redefined (with no change to the data) by other methods
 * using the function getVariableDataByName.
 */
class BaseParticles
{
  private:
    DataContainerUniquePtrAssemble<DiscreteVariable> all_discrete_variable_ptrs_;
    DataContainerUniquePtrAssemble<SingularVariable> all_global_variable_ptrs_;
    UniquePtrsKeeper<BaseEntity> unique_variable_ptrs_;

  public:
    explicit BaseParticles(SPHBody &sph_body, BaseMaterial *base_material);
    virtual ~BaseParticles(){};
    SPHBody &getSPHBody() { return sph_body_; };
    BaseMaterial &getBaseMaterial() { return base_material_; };

    //----------------------------------------------------------------------
    // Global information for defining particle groups
    // total_real_particles_ gives the run-time total number of real particles.
    // real_particles_bound_ gives the maximum possible number of real particles
    // which is allowed in the computation.
    // particles_bound_ gives the total number of particles in all groups.
    //----------------------------------------------------------------------
  protected:
    SingularVariable<UnsignedInt> *v_total_real_particles_;
    UnsignedInt real_particles_bound_;
    UnsignedInt particles_bound_;

  public:
    /** initialize basic variables after the particles generated by particle generator */
    virtual void initializeBasicParticleVariables();
    //----------------------------------------------------------------------
    // Generalized particle manipulation
    //----------------------------------------------------------------------
    UnsignedInt TotalRealParticles() { return *v_total_real_particles_->ValueAddress(); };
    void incrementTotalRealParticles(UnsignedInt increment = 1) { *v_total_real_particles_->ValueAddress() += increment; };
    void decrementTotalRealParticles(UnsignedInt decrement = 1) { *v_total_real_particles_->ValueAddress() -= decrement; };
    UnsignedInt RealParticlesBound() { return real_particles_bound_; };
    UnsignedInt ParticlesBound() { return particles_bound_; };
    void initializeAllParticlesBounds(size_t total_real_particles);
    void initializeAllParticlesBoundsFromReloadXml();
    void increaseAllParticlesBounds(size_t buffer_size);
    void copyFromAnotherParticle(size_t index, size_t another_index);
    size_t allocateGhostParticles(size_t ghost_size);
    void updateGhostParticle(size_t ghost_index, size_t index);
    void switchToBufferParticle(size_t index);
    void createRealParticleFrom(size_t index);
    //----------------------------------------------------------------------
    // Parameterized management on particle variables and data
    //----------------------------------------------------------------------
  private:
    template <typename DataType, template <typename T> class VariableType>
    DataType *initializeVariable(VariableType<DataType> *variable, DataType initial_value = ZeroData<DataType>::value);
    template <typename DataType, template <typename T> class VariableType, class InitializationFunction>
    DataType *initializeVariable(VariableType<DataType> *variable, const InitializationFunction &initialization);

  public:
    template <class DataType, typename... Args>
    DataType *addUniqueDiscreteVariable(const std::string &name, size_t data_size, Args &&...args);
    template <typename DataType, typename... Args>
    DataType *registerDiscreteVariable(const std::string &name, size_t data_size, Args &&...args);

    template <class DataType, typename... Args>
    DataType *addUniqueStateVariable(const std::string &name, Args &&...args);
    template <typename DataType, typename... Args>
    DataType *registerStateVariable(const std::string &name, Args &&...args);
    template <typename DataType>
    DataType *registerStateVariableFrom(const std::string &new_name, const std::string &old_name);
    template <typename DataType>
    DataType *registerStateVariableFrom(const std::string &name, const StdLargeVec<DataType> &geometric_data);
    template <typename DataType>
    DataType *registerStateVariableFromReload(const std::string &name);
    template <typename DataType>
    DiscreteVariable<DataType> *getVariableByName(const std::string &name);
    template <typename DataType>
    DataType *getVariableDataByName(const std::string &name);

    template <typename DataType>
    SingularVariable<DataType> *registerSingularVariable(const std::string &name, DataType initial_value = ZeroData<DataType>::value);
    template <typename DataType>
    SingularVariable<DataType> *getSingularVariableByName(const std::string &name);
    //----------------------------------------------------------------------
    // Manage subsets of particle variables
    //----------------------------------------------------------------------
    template <typename DataType>
    DiscreteVariable<DataType> *addVariableToList(ParticleVariables &variable_set, const std::string &name);
    template <typename DataType>
    void addVariableToWrite(const std::string &name);
    template <typename DataType>
    void addVariableToRestart(const std::string &name);

    inline const ParticleVariables &getVariablesToRestart() const { return variables_to_restart_; }
    template <typename DataType>
    void addVariableToReload(const std::string &name);
    inline const ParticleVariables &getVariablesToReload() const { return variables_to_reload_; }

    //----------------------------------------------------------------------
    // Particle data for sorting
    //----------------------------------------------------------------------
  protected:
    UnsignedInt *original_id_; /**< the original ids assigned just after particle is generated. */
    UnsignedInt *sorted_id_;   /**< the current sorted particle ids of particles from original ids. */
    ParticleData sortable_data_;
    ParticleVariables sortable_variables_;

  public:
    template <typename DataType>
    void addVariableToSort(const std::string &name);
    UnsignedInt *ParticleOriginalIds() { return original_id_; };
    UnsignedInt *ParticleSortedIds() { return sorted_id_; };
    ParticleData &SortableParticleData() { return sortable_data_; };
    ParticleVariables &SortableParticleVariables() { return sortable_variables_; };
    AssignIndex getAssignIndex() { return AssignIndex(); };

    //----------------------------------------------------------------------
    // Variable related functions for offloading computing
    //----------------------------------------------------------------------
    template <typename DataType, class ExecutionPolicy>
    DataType *getVariableDataByName(const ExecutionPolicy &execution_policy, const std::string &name);
    template <typename DataType>
    DataType *getVariableDataByName(const ParallelDevicePolicy &execution_policy, const std::string &name);

    template <typename DataType, class ExecutionPolicy, typename... Args>
    DataType *registerDiscreteVariable(const ExecutionPolicy &execution_policy,
                                       const std::string &name, size_t data_size, Args &&...args);
    template <typename DataType, class ExecutionPolicy, typename... Args>
    DataType *registerStateVariable(const ExecutionPolicy &execution_policy, const std::string &name, Args &&...args);

    template <typename DataType, class ExecutionPolicy>
    SingularVariable<DataType> *getSingularVariableByName(const ExecutionPolicy &execution_policy, const std::string &name);
    template <typename DataType>
    SingularVariable<DataType> *getSingularVariableByName(const ParallelDevicePolicy &execution_policy, const std::string &name);

    template <typename DataType, typename... Args>
    SingularVariable<DataType> *registerSingularVariable(const ParallelDevicePolicy &execution_policy, const std::string &name, Args &&...args);

    template <typename DataType, class ExecutionPolicy>
    void addVariableToRestart(const ExecutionPolicy execution_policy, const std::string &name)
    {
        addVariableToRestart<DataType>(name);
    };
    template <typename DataType>
    void addVariableToRestart(const ParallelDevicePolicy execution_policy, const std::string &name);

    template <typename DataType, class ExecutionPolicy>
    void addVariableToSort(const ExecutionPolicy execution_policy, const std::string &name)
    {
        addVariableToSort<DataType>(name);
    };
    template <typename DataType>
    void addVariableToSort(const ParallelDevicePolicy execution_policy, const std::string &name);

    //----------------------------------------------------------------------
    // Particle data ouput functions
    //----------------------------------------------------------------------
    void writeParticlesToPltFile(std::ofstream &output_file);
    void resizeXmlDocForParticles(XmlParser &xml_parser);
    void writeParticlesToXmlForRestart(std::string &filefullpath);
    void readParticleFromXmlForRestart(std::string &filefullpath);
    void writeToXmlForReloadParticle(std::string &filefullpath);
    XmlParser &readReloadXmlFile(const std::string &filefullpath);
    template <typename OwnerType>
    void checkReloadFileRead(OwnerType *owner);
    //----------------------------------------------------------------------
    // Function related to geometric variables and their relations
    //----------------------------------------------------------------------
    void registerPositionAndVolumetricMeasure(StdLargeVec<Vecd> &pos, StdLargeVec<Real> &Vol);
    void registerPositionAndVolumetricMeasureFromReload();
    Vecd *ParticlePositions() { return pos_; }
    Real *VolumetricMeasures() { return Vol_; }
    virtual Real ParticleVolume(size_t index) { return Vol_[index]; }
    virtual Real ParticleSpacing(size_t index) { return std::pow(Vol_[index], 1.0 / Real(Dimensions)); }

  protected:
    Vecd *pos_;  /**< Position */
    Real *Vol_;  /**< Volumetric measure, also area and length of surface and linear particle */
    Real *rho_;  /**< Density as a fundamental property of phyiscal matter */
    Real *mass_; /**< Mass as another fundamental property of physical matter */

    SPHBody &sph_body_;
    std::string body_name_;
    BaseMaterial &base_material_;
    XmlParser restart_xml_parser_;
    XmlParser reload_xml_parser_;
    ParticleData all_state_data_; /**< all discrete variable data except those on particle IDs  */
    ParticleVariables all_discrete_variables_;
    SingularVariables all_singular_variables_;
    ParticleVariables variables_to_write_;
    ParticleVariables variables_to_restart_;
    ParticleVariables variables_to_reload_;
    bool is_reload_file_read_ = false;

  public:
    ParticleVariables &VariablesToWrite() { return variables_to_write_; };
    //----------------------------------------------------------------------
    // Small structs for generalize particle operations on
    // assembled variables and data sets
    //----------------------------------------------------------------------
  protected:
    struct CopyParticleState
    {
        template <typename DataType>
        void operator()(DataContainerKeeper<AllocatedData<DataType>> &data_keeper, size_t index, size_t another_index);
    };

    struct WriteAParticleVariableToXml
    {
        XmlParser &xml_parser_;
        WriteAParticleVariableToXml(XmlParser &xml_parser) : xml_parser_(xml_parser){};

        template <typename DataType>
        void operator()(DataContainerAddressKeeper<DiscreteVariable<DataType>> &variables);
    };

    struct ReadAParticleVariableFromXml
    {
        XmlParser &xml_parser_;
        ReadAParticleVariableFromXml(XmlParser &xml_parser) : xml_parser_(xml_parser){};

        template <typename DataType>
        void operator()(DataContainerAddressKeeper<DiscreteVariable<DataType>> &variables, BaseParticles *base_particles);
    };

    OperationOnDataAssemble<ParticleData, CopyParticleState> copy_particle_state_;
    OperationOnDataAssemble<ParticleVariables, WriteAParticleVariableToXml> write_restart_variable_to_xml_, write_reload_variable_to_xml_;
    OperationOnDataAssemble<ParticleVariables, ReadAParticleVariableFromXml> read_restart_variable_from_xml_;
};
} // namespace SPH
#endif // BASE_PARTICLES_H
