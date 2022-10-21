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
 * @file 	cell_linked_list.h
 * @brief 	Here gives the classes for managing cell linked lists. This is the basic class
 * 			for building the particle configurations.
 * @details The cell linked list saves for each body a list of particles
 * 			located within the cell.
 * @author	Chi ZHang, Yongchuan and Xiangyu Hu
 * @version	1.0
 *			Try to implement EIGEN libaary for base vector, matrix and 
 *			linear algebra operation.  
 *			-- Chi ZHANG
 */

#ifndef MESH_CELL_LINKED_LIST_H
#define MESH_CELL_LINKED_LIST_H

#include "base_mesh.h"
#include "neighbor_relation.h"

namespace SPH
{

	class SPHSystem;
	class SPHBody;
	class BaseParticles;
	class Kernel;
	class SPHAdaptation;
	/**
	 * @class CellList
	 * @brief Struct of Cell list for data store. 
	 */
	class CellList
	{
	public:
		/** using concurrent vectors due to writing conflicts when building the list */
		ConcurrentIndexVector concurrent_particle_indexes_;
		/** non-concurrent cell linked list rewritten for building neighbor list */
		ListDataVector cell_list_data_;
		/** the index vector for real particles. */
		IndexVector real_particle_indexes_;

		CellList();
		~CellList(){};
	};

	/**
	 * @class BaseCellLinkedList
	 * @brief The Abstract class for mesh cell linked list direved from BaseMeshFied. 
	 */
	class BaseCellLinkedList : public BaseMeshField
	{
	protected:
		RealBody &real_body_; 	/** The ptr to the relevant SPH body. */
		Kernel &kernel_;		/** The ptr to the relevant Kernel function. */
		BaseParticles *base_particles_;	/** The ptr to the relevant particles. */

		/** clear split cell lists in this mesh*/
		virtual void clearSplitCellLists(SplitCellLists &split_cell_lists);
		/** update split particle list in this mesh */
		virtual void updateSplitCellLists(SplitCellLists &split_cell_lists) = 0;

	public:
		BaseCellLinkedList(RealBody &real_body, SPHAdaptation &sph_adaptation);
		virtual ~BaseCellLinkedList(){};

		/** Assign base particles to the mesh cell linked list,
		 * and is important because particles are not defined in the constructor.  */
		virtual void assignBaseParticles(BaseParticles *base_particles) = 0;
		/** update the cell lists */
		virtual void UpdateCellLists() = 0;
		/** Insert a cell-linked_list entry to the concurrent index list. */
		virtual void insertACellLinkedParticleIndex(size_t particle_index, const Vecd &particle_position) = 0;
		/** Insert a cell-linked_list entry of the index and particle position pair. */
		virtual void InsertACellLinkedListDataEntry(size_t particle_index, const Vecd &particle_position) = 0;
		/** find the nearest list data entry */
		virtual ListData findNearestListDataEntry(const Vecd &position) = 0;
		/** computing the sequence which indicate the order of sorted particle data */
		virtual void computingSequence(StdLargeVec<size_t> &sequence) = 0;
		/** Tag body part by cell, call by body part */
		virtual void tagBodyPartByCell(CellLists &cell_lists, std::function<bool(Vecd, Real)> &check_included) = 0;
		/** Tag domain bounding cells in an axis direction, called by domain bounding classes */
		virtual void tagBoundingCells(StdVec<CellLists> &cell_lists, BoundingBox &bounding_bounds, int axis) = 0;
		/** Tag domain bounding cells in one side, called by mirror boundary condition */
		virtual void tagOneSideBoundingCells(CellLists &cell_lists, BoundingBox &bounding_bounds, int axis, bool positive) = 0;
	};

	/**
	 * @class CellLinkedList
	 * @brief Defining a mesh cell linked list for a body.
	 * 		  The meshes for all bodies share the same global coordinates.
	 */
	class CellLinkedList : public BaseCellLinkedList, public Mesh
	{
	protected:
		/** The array for of mesh cells, i.e. mesh data.
		 * Within each cell, a list is saved with the indexes of particles.*/
		MeshDataMatrix<CellList> cell_linked_lists_;
		/** Update the cell link lists. */
		virtual void updateSplitCellLists(SplitCellLists &split_cell_lists) override;

	public:
		CellLinkedList(BoundingBox tentative_bounds, Real grid_spacing, RealBody &real_body, SPHAdaptation &sph_adaptation);
		virtual ~CellLinkedList() { deleteMeshDataMatrix(); };

		void allocateMeshDataMatrix(); /**< allocate memories for addresses of data packages. */
		void deleteMeshDataMatrix();   /**< delete memories for addresses of data packages. */
		/** Assign the relevant particles. */
		virtual void assignBaseParticles(BaseParticles *base_particles) override;
		/** Clear the lists. */
		void clearCellLists();
		/** Update the list data. */
		void UpdateCellListData();
		/** Update the cell lists. */
		virtual void UpdateCellLists() override;
		/** Insert a cell-linked_list entry to the concurrent index list. */
		void insertACellLinkedParticleIndex(size_t particle_index, const Vecd &particle_position) override;
		/** Insert a cell-linked_list entry of the index and particle position pair. */
		void InsertACellLinkedListDataEntry(size_t particle_index, const Vecd &particle_position) override;
		/** find the nearest list data entry */
		virtual ListData findNearestListDataEntry(const Vecd &position) override;
		/** computing the sequence which indicate the order of sorted particle data */
		virtual void computingSequence(StdLargeVec<size_t> &sequence) override;
		/** Tag body part by cell, call by body part */
		virtual void tagBodyPartByCell(CellLists &cell_lists, std::function<bool(Vecd, Real)> &check_included) override;
		/** Tag domain bounding cells in an axis direction, called by domain bounding classes */
		virtual void tagBoundingCells(StdVec<CellLists> &cell_lists, BoundingBox &bounding_bounds, int axis) override;
		/** Tag domain bounding cells in one side, called by mirror boundary condition */
		virtual void tagOneSideBoundingCells(CellLists &cell_lists, BoundingBox &bounding_bounds, int axis, bool positive) override;
		/** Write the mesh field data to plt format. */
		virtual void writeMeshFieldToPlt(std::ofstream &output_file) override;

		/** Generalized particle search algorithm */
		template <typename GetParticleIndex, typename GetSearchDepth, typename GetNeighborRelation>
		void searchNeighborsByParticles(size_t total_real_particles, BaseParticles &source_particles,
										ParticleConfiguration &particle_configuration, GetParticleIndex &get_particle_index,
										GetSearchDepth &get_search_depth, GetNeighborRelation &get_neighbor_relation);

		/** Generalized particle search algorithm for searching body part */
		template <typename GetParticleIndex, typename GetSearchDepth, typename GetNeighborRelation, typename PartParticleCheck>
		void searchNeighborPartsByParticles(size_t total_real_particles, BaseParticles &source_particles,
											ParticleConfiguration &particle_configuration, GetParticleIndex &get_particle_index,
											GetSearchDepth &get_search_depth, GetNeighborRelation &get_neighbor_relation,
											PartParticleCheck &part_check);
		/** Return the Cell list. */
		MeshDataMatrix<CellList> getCellLists() const { return cell_linked_lists_; }
	};

	/**
	 * @class MultilevelCellLinkedList
	 * @brief Defining a multilevel mesh cell linked list for a body
	 * 		  for multi-resolution particle configuration.
	 */
	class MultilevelCellLinkedList : public MultilevelMesh<BaseCellLinkedList, CellLinkedList, RefinedMesh<CellLinkedList>>
	{
	protected:
		StdLargeVec<Real> &h_ratio_;
		virtual void updateSplitCellLists(SplitCellLists &split_cell_lists) override{};
		/** determine mesh level from particle cutoff radius */
		inline size_t getMeshLevel(Real particle_cutoff_radius);

	public:
		MultilevelCellLinkedList(BoundingBox tentative_bounds, Real reference_grid_spacing,
								 size_t total_levels, RealBody &real_body, SPHAdaptation &sph_adaptation);
		virtual ~MultilevelCellLinkedList(){};
		/** Assign the relevant particles. */
		virtual void assignBaseParticles(BaseParticles *base_particles) override;
		/** Update the lists. */
		virtual void UpdateCellLists() override;
		/** Insert a cell-linked_list entry to the concurrent index list. */
		void insertACellLinkedParticleIndex(size_t particle_index, const Vecd &particle_position) override;
		/** Insert a cell-linked_list entry of the index and particle position pair. */
		void InsertACellLinkedListDataEntry(size_t particle_index, const Vecd &particle_position) override;
		/** find the nearest list data entry */
		virtual ListData findNearestListDataEntry(const Vecd &position) override { return ListData(0, Vecd::Zero()); };
		/** computing the sequence which indicate the order of sorted particle data */
		virtual void computingSequence(StdLargeVec<size_t> &sequence) override{};
		/** Tag body part by cell, call by body part */
		virtual void tagBodyPartByCell(CellLists &cell_lists, std::function<bool(Vecd, Real)> &check_included) override;
		/** Tag domain bounding cells in an axis direction, called by domain bounding classes */
		virtual void tagBoundingCells(StdVec<CellLists> &cell_lists, BoundingBox &bounding_bounds, int axis) override{};
		/** Tag domain bounding cells in one side, called by mirror boundary condition */
		virtual void tagOneSideBoundingCells(CellLists &cell_lists, BoundingBox &bounding_bounds, int axis, bool positive) override{};
	};
}
#endif // MESH_CELL_LINKED_LIST_H