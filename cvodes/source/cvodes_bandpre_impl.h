/*
 * -----------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2006-01-11 21:13:51 $
 * ----------------------------------------------------------------- 
 * Programmer(s): Michael Wittman, Alan C. Hindmarsh and
 *                Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see sundials/cvodes/LICENSE.
 * -----------------------------------------------------------------
 * Implementation header file for the CVBANDPRE module.
 * -----------------------------------------------------------------
 */

#ifndef _CVBANDPRE_IMPL_H
#define _CVBANDPRE_IMPL_H

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

#include "cvodes_bandpre.h"
#include "sundials_band.h"

/*
 * -----------------------------------------------------------------
 * Type: CVBandPrecData
 * -----------------------------------------------------------------
 */

typedef struct {

  /* Data set by user in CVBandPrecAlloc */

  long int N;
  long int ml, mu;

  /* Data set by CVBandPrecSetup */

  BandMat savedJ;
  BandMat savedP;
  long int *pivots;

  /* Rhs calls */

  long int nfeBP;

  /* Pointer to cvode_mem */

  void *cvode_mem;

} *CVBandPrecData;

/*
 * -----------------------------------------------------------------
 * CVBANDPRE error messages
 * -----------------------------------------------------------------
 */

/* CVBandPrecAlloc error messages */

#define _CVBALLOC_        "CVBandPreAlloc-- "
#define MSGBP_CVMEM_NULL  _CVBALLOC_ "Integrator memory is NULL.\n\n"
#define MSGBP_BAD_NVECTOR _CVBALLOC_ "A required vector operation is not implemented.\n\n"

/* CVBandPrecGet* error message */

#define MSGBP_PDATA_NULL "CVBandPrecGet*-- BandPrecData is NULL.\n\n"

/* CVBPSp* error message */

#define MSGBP_NO_PDATA "CVBPSp*-- BandPrecData is NULL.\n\n"

#ifdef __cplusplus
}
#endif

#endif