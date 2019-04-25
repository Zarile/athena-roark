//========================================================================================
// Athena++ astrophysical MHD code
// Copyright(C) 2014 James M. Stone <jmstone@princeton.edu> and other code contributors
// Licensed under the 3-clause BSD License, see LICENSE file for details
//========================================================================================
//! \file scalars.cpp
//  \brief implementation of functions in class PassiveScalars

// C headers

// C++ headers
#include <algorithm>
#include <string>
#include <vector>

// Athena++ headers
#include "../athena.hpp"
#include "../athena_arrays.hpp"
#include "../coordinates/coordinates.hpp"
#include "../eos/eos.hpp"
#include "../mesh/mesh.hpp"
#include "../reconstruct/reconstruction.hpp"
#include "scalars.hpp"

// constructor, initializes data structures and parameters

PassiveScalars::PassiveScalars(MeshBlock *pmb, ParameterInput *pin)  :
    s(NSCALARS, pmb->ncells3, pmb->ncells2, pmb->ncells1),
    s1(NSCALARS, pmb->ncells3, pmb->ncells2, pmb->ncells1),
    s_flux{ {NSCALARS, pmb->ncells3, pmb->ncells2, pmb->ncells1+1},
            {NSCALARS, pmb->ncells3, pmb->ncells2+1, pmb->ncells1,
             (pmb->pmy_mesh->f2 ? AthenaArray<Real>::DataStatus::allocated :
              AthenaArray<Real>::DataStatus::empty)},
            {NSCALARS, pmb->ncells3+1, pmb->ncells2, pmb->ncells1,
             (pmb->pmy_mesh->f3 ? AthenaArray<Real>::DataStatus::allocated :
              AthenaArray<Real>::DataStatus::empty)}
    },
    coarse_s_(NSCALARS, pmb->ncc3, pmb->ncc2, pmb->ncc1,
              (pmb->pmy_mesh->multilevel ? AthenaArray<Real>::DataStatus::allocated :
               AthenaArray<Real>::DataStatus::empty)),
    sbvar(pmb, &s, &coarse_s_, s_flux),
    pmy_block(pmb) {
  int nc1 = pmb->ncells1, nc2 = pmb->ncells2, nc3 = pmb->ncells3;
  Mesh *pm = pmy_block->pmy_mesh;

  pmb->RegisterMeshBlockData(s);

  // Allocate optional passive scalar variable memory registers for time-integrator
  if (pmb->precon->xorder == 4) {
    // fourth-order cell-centered approximations
    s_cc.NewAthenaArray(NSCALARS, nc3, nc2, nc1);
  }

  // If user-requested time integrator is type 3S*, allocate additional memory registers
  std::string integrator = pin->GetOrAddString("time", "integrator", "vl2");
  if (integrator == "ssprk5_4" || STS_ENABLED)
    // future extension may add "int nregister" to Hydro class
    s2.NewAthenaArray(NSCALARS, nc3, nc2, nc1);

  // "Enroll" in SMR/AMR by adding to vector of pointers in MeshRefinement class
  if (pm->multilevel) {
    pmy_block->pmr->AddToRefinement(&s, &coarse_s_);
  }

  // enroll CellCenteredBoundaryVariable object
  sbvar.bvar_index = pmb->pbval->bvars.size();
  pmb->pbval->bvars.push_back(&sbvar);
  pmb->pbval->bvars_main_int.push_back(&sbvar);

  // Allocate memory for scratch arrays
  dxw_.NewAthenaArray(nc1);
  sl_.NewAthenaArray(NSCALARS, nc1);
  sr_.NewAthenaArray(NSCALARS, nc1);
  slb_.NewAthenaArray(NSCALARS, nc1);
  x1face_area_.NewAthenaArray(nc1+1);
  if (pm->f2) {
    x2face_area_.NewAthenaArray(nc1);
    x2face_area_p1_.NewAthenaArray(nc1);
  }
  if (pm->f3) {
    x3face_area_.NewAthenaArray(nc1);
    x3face_area_p1_.NewAthenaArray(nc1);
  }
  cell_volume_.NewAthenaArray(nc1);
  dflx_.NewAthenaArray(NHYDRO, nc1);

  // fourth-order 4D scratch arrays
  sl3d_.NewAthenaArray(NSCALARS, nc3, nc2, nc1);
  sr3d_.NewAthenaArray(NSCALARS, nc3, nc2, nc1);
  scr1_nkji_.NewAthenaArray(NSCALARS, nc3, nc2, nc1);
  scr2_nkji_.NewAthenaArray(NSCALARS, nc3, nc2, nc1);
  laplacian_l_fc_.NewAthenaArray(nc1);
  laplacian_r_fc_.NewAthenaArray(nc1);
}
