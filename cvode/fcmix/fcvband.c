/*
 * -----------------------------------------------------------------
 * $Revision: 1.6 $
 * $Date: 2004-06-21 23:07:12 $
 * ----------------------------------------------------------------- 
 * Programmers: Alan C. Hindmarsh and Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California
 * Produced at the Lawrence Livermore National Laboratory
 * All rights reserved
 * For details, see sundials/cvode/LICENSE
 * -----------------------------------------------------------------
 * Fortran/C interface routines for CVODE/CVBAND, for the case of 
 * a user-supplied Jacobian approximation routine.                
 * -----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "sundialstypes.h" /* definitions of type realtype                   */
#include "nvector.h"       /* definitions of type N_Vector and vector macros */
#include "fcvode.h"        /* actual function names, prototypes, global vars.*/
#include "cvband.h"        /* CVBand prototype                               */

/******************************************************************************/

/* Prototype of the Fortran routine */
void FCV_BJAC(long int*, long int*, long int*, long int*, 
              realtype*, realtype*, realtype*, realtype*,
              realtype*, realtype*, realtype*, realtype*);


/***************************************************************************/

void FCV_BANDSETJAC(int *flag, int *ier)
{
  if (*flag == 0) CVBandSetJacFn(CV_cvodemem, NULL);
  else            CVBandSetJacFn(CV_cvodemem, FCVBandJac);
}

/***************************************************************************/

/* C function CVBandJac interfaces between CVODE and a Fortran subroutine
   FCVBJAC for solution of a linear system with band Jacobian approximation.
   Addresses of arguments are passed to FCVBJAC, using the macro 
   BAND_COL from BAND and the routine N_VGetData from NVECTOR.
   The address passed for J is that of the element in column 0 with row 
   index -mupper.  An extended bandwith equal to (J->smu) + mlower + 1 is
   passed as the column dimension of the corresponding array.
   Auxiliary data is assumed to be communicated by Common. */

void FCVBandJac(long int N, long int mupper, long int mlower,
                BandMat J, realtype t,
                N_Vector y, N_Vector fy, void *jac_data,
                N_Vector vtemp1, N_Vector vtemp2, N_Vector vtemp3)
{
  N_Vector ewt;
  realtype *ydata, *fydata, *jacdata, *ewtdata, *v1data, *v2data, *v3data;
  long int eband;

  CVodeGetErrWeights(CV_cvodemem, &ewt);

  ydata   = (realtype *) N_VGetData(y);
  fydata  = (realtype *) N_VGetData(fy);
  v1data  = (realtype *) N_VGetData(vtemp1);
  v2data  = (realtype *) N_VGetData(vtemp2);
  v3data  = (realtype *) N_VGetData(vtemp3);
  ewtdata = (realtype *) N_VGetData(ewt);

  eband = (J->smu) + mlower + 1;
  jacdata = BAND_COL(J,0) - mupper;


  FCV_BJAC(&N, &mupper, &mlower, &eband, 
           &t, ydata, fydata, jacdata, 
           ewtdata, v1data, v2data, v3data);

}
