/*
 * Copyright 2011, Nicolas Mansard, LAAS-CNRS
 *
 * This file is part of sot-dyninv.
 * sot-dyninv is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * sot-dyninv is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.  You should
 * have received a copy of the GNU Lesser General Public License along
 * with sot-dyninv.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __sot_dyninv_SolverDynReduced_H__
#define __sot_dyninv_SolverDynReduced_H__
/* --------------------------------------------------------------------- */
/* --- API ------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

#if defined (WIN32)
#  if defined (solver_dyn_reduced_EXPORTS)
#    define SOTSOLVERDYNREDUCED_EXPORT __declspec(dllexport)
#  else
#    define SOTSOLVERDYNREDUCED_EXPORT __declspec(dllimport)
#  endif
#else
#  define SOTSOLVERDYNREDUCED_EXPORT
#endif

/* --------------------------------------------------------------------- */
/* --- INCLUDE --------------------------------------------------------- */
/* --------------------------------------------------------------------- */


/* SOT */
#include <sot-dyninv/signal-helper.h>
#include <sot-dyninv/entity-helper.h>
#include <sot-dyninv/stack-template.h>
#include <sot-dyninv/task-dyn-pd.h>
#include <soth/HCOD.hpp>
#include <Eigen/QR>
//#include <Eigen/SVD>
#include <sot-dyninv/col-piv-qr-solve-in-place.h>

namespace dynamicgraph {
  namespace sot {
    namespace dyninv {

      /* --------------------------------------------------------------------- */
      /* --- CLASS ----------------------------------------------------------- */
      /* --------------------------------------------------------------------- */

      class SOTSOLVERDYNREDUCED_EXPORT SolverDynReduced
	:public ::dynamicgraph::Entity
	,public ::dynamicgraph::EntityHelper<SolverDynReduced>
	,public sot::Stack< TaskDynPD >
	{
	  DYNAMIC_GRAPH_ENTITY_DECL();
	public: /* --- CONSTRUCTOR ---- */

	  SolverDynReduced( const std::string & name );

	public: /* --- STACK INHERITANCE --- */

	  typedef sot::Stack<TaskDynPD> stack_t;
	  using stack_t::TaskDependancyList_t;
	  using stack_t::StackIterator_t;
	  using stack_t::StackConstIterator_t;
	  using stack_t::stack;

	  virtual TaskDependancyList_t getTaskDependancyList( const TaskDynPD& task );
	  virtual void addDependancy( const TaskDependancyList_t& depList );
	  virtual void removeDependancy( const TaskDependancyList_t& depList );
	  virtual void resetReady( void );

	public: /* --- ENTITY INHERITANCE --- */

	  virtual void display( std::ostream& os ) const;

	public:  /* --- SIGNALS --- */

	  DECLARE_SIGNAL_IN(matrixInertia,ml::Matrix);
	  DECLARE_SIGNAL_IN(inertiaSqroot,ml::Matrix);
	  DECLARE_SIGNAL_IN(inertiaSqrootInv,ml::Matrix);
	  DECLARE_SIGNAL_IN(velocity,ml::Vector);
	  DECLARE_SIGNAL_IN(dyndrift,ml::Vector);
	  DECLARE_SIGNAL_IN(damping,double);
	  DECLARE_SIGNAL_IN(breakFactor,double);
	  DECLARE_SIGNAL_IN(posture,ml::Vector);
	  DECLARE_SIGNAL_IN(position,ml::Vector);

	  DECLARE_SIGNAL_OUT(precompute,int);

	  DECLARE_SIGNAL_OUT(inertiaSqrootOut,ml::Matrix);
	  DECLARE_SIGNAL_OUT(inertiaSqrootInvOut,ml::Matrix);

	  DECLARE_SIGNAL_OUT(sizeForcePoint,int);
	  DECLARE_SIGNAL_OUT(sizeForceSpatial,int);
	  DECLARE_SIGNAL_OUT(sizeConfiguration,int);

	  DECLARE_SIGNAL_OUT(Jc,ml::Matrix);
	  DECLARE_SIGNAL_OUT(forceGenerator,ml::Matrix);
	  DECLARE_SIGNAL_OUT(freeMotionBase,ml::Matrix);
	  DECLARE_SIGNAL_OUT(freeForceBase,ml::Matrix);
	  DECLARE_SIGNAL_OUT(driftContact,ml::Vector);
	  DECLARE_SIGNAL_OUT(sizeMotion,int);
	  DECLARE_SIGNAL_OUT(sizeActuation,int);

	  DECLARE_SIGNAL_OUT(solution,ml::Vector);
	  DECLARE_SIGNAL_OUT(reducedControl,ml::Vector);
	  DECLARE_SIGNAL_OUT(reducedForce,ml::Vector);
	  DECLARE_SIGNAL_OUT(acceleration,ml::Vector);
	  DECLARE_SIGNAL_OUT(forces,ml::Vector);
	  DECLARE_SIGNAL_OUT(torque,ml::Vector);

	  DECLARE_SIGNAL_OUT(forcesNormal,ml::Vector);
	  DECLARE_SIGNAL_OUT(activeForces,ml::Vector);


	  /* Temporary time-dependant shared variables. */
	  DECLARE_SIGNAL(Jcdot,OUT,ml::Matrix);

	private:  /* --- CONTACT POINTS --- */

	  typedef boost::shared_ptr<dynamicgraph::SignalPtr<ml::Matrix,int> > matrixSINPtr;
	  typedef boost::shared_ptr<dynamicgraph::SignalPtr<ml::Vector,int> > vectorSINPtr;
	  typedef boost::shared_ptr<dynamicgraph::Signal<ml::Vector,int> > vectorSOUTPtr;
	  struct Contact
	  {
	    matrixSINPtr jacobianSIN;
	    matrixSINPtr JdotSIN;
	    matrixSINPtr supportSIN;
	    vectorSINPtr correctorSIN;
	    vectorSOUTPtr forceSOUT,fnSOUT;
	    int position;
	    std::pair<int,int> range;
	  };
	  typedef std::map< std::string,Contact > contacts_t;
	  contacts_t contactMap;

	public:
	  void addContact( const std::string & name,
			   dynamicgraph::Signal<ml::Matrix,int> * jacobianSignal,
			   dynamicgraph::Signal<ml::Matrix,int> * JdotSignal,
			   dynamicgraph::Signal<ml::Vector,int> * corrSignal,
			   dynamicgraph::Signal<ml::Matrix,int> * contactPointsSignal );
	  void addContactFromTask( const std::string & taskName, const std::string & contactName );
	  void removeContact( const std::string & name );
	  void dispContacts( std::ostream& os ) const;

	  matrixSINPtr getSupportSIN( const std::string & contacName );


	public: /* --- COMMANDS --- */
	  void debugOnce( void );


	private: /* --- INTERNAL COMPUTATIONS --- */
	  void refreshTaskTime( int time );
	  void resizeSolver( void );
	  void computeSizesForce( int t );

	private:
	  typedef boost::shared_ptr<soth::HCOD> hcod_ptr_t;
	  hcod_ptr_t hsolver;

	  int G_rank;
	  Eigen::ColPivQRSolveInPlace X_qr,Gt_qr;


	  Eigen::MatrixXd Cforce,Czero;
	  soth::VectorBound bforce,bzero;
	  std::vector< Eigen::MatrixXd > Ctasks;
	  std::vector< soth::VectorBound > btasks;

	  Eigen::MatrixXd BV;

	  /* Force drift = xddot^* - Jdot qdot. */
	  Eigen::VectorXd solution,forceDrift;

	}; // class SolverDynReduced

    } // namespace dyninv
  } // namespace sot
} // namespace dynamicgraph



#endif // #ifndef __sot_dyninv_SolverDynReduced_H__
