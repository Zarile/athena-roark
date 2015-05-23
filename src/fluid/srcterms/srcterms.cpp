//======================================================================================
// Athena++ astrophysical MHD code
// Copyright (C) 2014 James M. Stone  <jmstone@princeton.edu>
//
// This program is free software: you can redistribute and/or modify it under the terms
// of the GNU General Public License (GPL) as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of GNU GPL in the file LICENSE included in the code
//
//======================================================================================

// Primary header
#include "srcterms.hpp"

// Athena headers
#include "../../athena.hpp"          // Real
#include "../../athena_arrays.hpp"   // AthenaArray
#include "../../mesh.hpp"            // MeshBlock
#include "../../coordinates/coordinates.hpp"  // src_terms_i_
#include "../fluid.hpp"              // Fluid
#include "../../parameter_input.hpp" // ParameterInput

//======================================================================================
//! \file srcterms.cpp
//  \brief implements functions that compute physical source terms in the fluid
//======================================================================================

// FluidSourceTerms constructor - sets function pointers for each of the physical source
// terms to be included in the calculation.

FluidSourceTerms::FluidSourceTerms(Fluid *pf, ParameterInput *pin)
{
  pmy_fluid_ = pf;
  gm_ = pin->GetOrAddReal("problem","GM",0.0);
  g1_ = pin->GetOrAddReal("fluid","grav_acc1",0.0);
  g2_ = pin->GetOrAddReal("fluid","grav_acc2",0.0);
  g3_ = pin->GetOrAddReal("fluid","grav_acc3",0.0);
  UserSourceTerm = NULL;
}

// destructor

FluidSourceTerms::~FluidSourceTerms()
{
}

//--------------------------------------------------------------------------------------
//! \fn
//  \brief

void FluidSourceTerms::PhysicalSourceTermsX1(int k, int j, const Real dt,
  const AthenaArray<Real> &flx,
  const AthenaArray<Real> &prim, AthenaArray<Real> &cons)
{
  if (gm_==0.0 && g1_==0.0 && g2_==0.0 && g3_==0.0) return;

// Source terms due to point mass gravity

  MeshBlock *pmb = pmy_fluid_->pmy_block;
#pragma simd
    for (int i=pmb->is; i<=pmb->ie; ++i) {
      Real den = prim(IDN,k,j,i);

      if (gm_!=0.0) {
        Real src = dt*den*pmb->pcoord->coord_src1_i_(i)*gm_/pmb->x1v(i);
        cons(IM1,k,j,i) -= src;
        if (NON_BAROTROPIC_EOS) cons(IEN,k,j,i) -= dt*0.5*(pmb->pcoord->phy_src1_i_(i)*flx(IDN,i)*gm_
                                                           +pmb->pcoord->phy_src2_i_(i)*flx(IDN,i+1)*gm_);
      }

      if (g1_!=0.0) {
        Real src = dt*den*g1_;
        cons(IM1,k,j,i) += src;
        if (NON_BAROTROPIC_EOS) cons(IEN,k,j,i) += src*prim(IVX,k,j,i);
      }

      if (g2_!=0.0) {
        Real src = dt*den*g2_;
        cons(IM2,k,j,i) += src;
        if (NON_BAROTROPIC_EOS) cons(IEN,k,j,i) += src*prim(IVY,k,j,i);
      }

      if (g3_!=0.0) {
        Real src = dt*den*g3_;
        cons(IM3,k,j,i) += src;
        if (NON_BAROTROPIC_EOS) cons(IEN,k,j,i) += src*prim(IVZ,k,j,i);
      }

    }

  return;
}

void FluidSourceTerms::PhysicalSourceTermsX2(int k, int j, const Real dt,
  const AthenaArray<Real> &flx, const AthenaArray<Real> &flx_p1,
  const AthenaArray<Real> &prim, AthenaArray<Real> &cons)
{
  return;
}

void FluidSourceTerms::PhysicalSourceTermsX3(int k, int j, const Real dt,
  const AthenaArray<Real> &flx, const AthenaArray<Real> &flx_p1,
  const AthenaArray<Real> &prim, AthenaArray<Real> &cons)
{
  return;
}


//--------------------------------------------------------------------------------------
//! \fn
//  \brief

void FluidSourceTerms::EnrollSrcTermFunction(SrcTermFunc_t my_func)
{
  UserSourceTerm = my_func;
  return;
}
