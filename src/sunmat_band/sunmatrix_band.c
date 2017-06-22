/*
 * -----------------------------------------------------------------
 * Programmer(s): Daniel Reynolds @ SMU
 *                David Gardner, Carol Woodward, Slaven Peles @ LLNL
 * -----------------------------------------------------------------
 * LLNS/SMU Copyright Start
 * Copyright (c) 2017, Southern Methodist University and 
 * Lawrence Livermore National Security
 *
 * This work was performed under the auspices of the U.S. Department 
 * of Energy by Southern Methodist University and Lawrence Livermore 
 * National Laboratory under Contract DE-AC52-07NA27344.
 * Produced at Southern Methodist University and the Lawrence 
 * Livermore National Laboratory.
 *
 * All rights reserved.
 * For details, see the LICENSE file.
 * LLNS/SMU Copyright End
 * -----------------------------------------------------------------
 * This is the implementation file for the band implementation of 
 * the SUNMATRIX package.
 * -----------------------------------------------------------------
 */ 

#include <stdio.h>
#include <stdlib.h>

#include <sunmatrix/sunmatrix_band.h>
#include <sundials/sundials_math.h>

#define ZERO RCONST(0.0)
#define ONE  RCONST(1.0)


/* Private function prototypes */
static booleantype SMCompatible_Band(SUNMatrix A, SUNMatrix B);
static booleantype SMCompatible2_Band(SUNMatrix A, N_Vector x, N_Vector y);


/*
 * -----------------------------------------------------------------
 * exported functions
 * -----------------------------------------------------------------
 */

/* ----------------------------------------------------------------
 * Returns the SUNMatrix type ID. Used to identify matrix 
 * implementations from the abstract SUNMatrix interface.
 */

SUNMatrix_ID SUNMatrixGetID_Band(SUNMatrix A)
{
  return SUNMATRIX_BAND;
}

/* ----------------------------------------------------------------------------
 * Function to create a new band matrix
 */

SUNMatrix SUNMatrixNew_Band(long int N, long int mu, long int ml, long int smu)
{
  SUNMatrix A;
  SUNMatrix_Ops ops;
  SUNMatrixContent_Band content;
  long int j, colSize;

  /* return with NULL matrix on illegal dimension input */
  if ( (N <= 0) || (smu < 0) || (ml < 0) ) return(NULL);

  /* Create matrix */
  A = NULL;
  A = (SUNMatrix) malloc(sizeof *A);
  if (A == NULL) return(NULL);
  
  /* Create matrix operation structure */
  ops = NULL;
  ops = (SUNMatrix_Ops) malloc(sizeof(struct _generic_SUNMatrix_Ops));
  if (ops == NULL) { free(A); return(NULL); }

  /* Attach operations */
  ops->getid       = SUNMatrixGetID_Band;
  ops->clone       = SUNMatrixClone_Band;
  ops->destroy     = SUNMatrixDestroy_Band;
  ops->zero        = SUNMatrixZero_Band;
  ops->scale       = SUNMatrixScale_Band;
  ops->copy        = SUNMatrixCopy_Band;
  ops->addidentity = SUNMatrixAddIdentity_Band;
  ops->add         = SUNMatrixAdd_Band;
  ops->matvec      = SUNMatrixMatvec_Band;

  /* Create content */
  content = NULL;
  content = (SUNMatrixContent_Band) malloc(sizeof(struct _SUNMatrixContent_Band));
  if (content == NULL) { free(ops); free(A); return(NULL); }

  /* Fill content */
  colSize = smu + ml + 1;
  content->M = N;
  content->N = N;
  content->mu = mu;
  content->ml = ml;
  content->s_mu = smu;
  content->ldim = colSize;
  content->ldata = N * colSize;
  content->data = NULL;
  content->data = (realtype *) malloc(N * colSize * sizeof(realtype));
  if (content->data == NULL) {
    free(content); free(ops); free(A); return(NULL);
  }
  content->cols = NULL;
  content->cols = (realtype **) malloc(N * sizeof(realtype *));
  if (content->cols == NULL) {
    free(content->data); free(content); free(ops); free(A); return(NULL);
  }
  for (j=0; j<N; j++) content->cols[j] = content->data + j * colSize;

  /* Attach content and ops */
  A->content = content;
  A->ops     = ops;

  return(A);
}

/* ----------------------------------------------------------------------------
 * Function to free a matrix created with SUNMatrixNew_Band
 */

void SUNMatrixDestroy_Band(SUNMatrix A)
{
  /* should not be called unless A is a band matrix; 
     otherwise return immediately */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return;

  /* perform operation */
  free(SM_DATA_B(A));  SM_DATA_B(A) = NULL;
  free(SM_CONTENT_B(A)->cols);  SM_CONTENT_B(A)->cols = NULL;
  free(A->content);  A->content = NULL;
  free(A->ops);  A->ops = NULL;
  free(A); A = NULL;
  return;
}

/* ----------------------------------------------------------------------------
 * Function to print the band matrix 
 */
 
void SUNMatrixPrint_Band(SUNMatrix A, FILE* outfile)
{
  long int i, j, start, finish;

  /* should not be called unless A is a band matrix; 
     otherwise return immediately */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return;

  /* perform operation */
  fprintf(outfile,"\n");
  for (i=0; i<SM_ROWS_B(A); i++) {
    start = SUNMAX(0, i-SM_LBAND_B(A));
    finish = SUNMIN(SM_COLUMNS_B(A)-1, i+SM_UBAND_B(A));
    for (j=0; j<start; j++)
      fprintf(outfile,"%12s  ","");
    for (j=start; j<=finish; j++) {
#if defined(SUNDIALS_EXTENDED_PRECISION)
      fprintf(outfile,"%12Lg  ", SM_ELEMENT_B(A,i,j));
#elif defined(SUNDIALS_DOUBLE_PRECISION)
      fprintf(outfile,"%12g  ", SM_ELEMENT_B(A,i,j));
#else
      fprintf(outfile,"%12g  ", SM_ELEMENT_B(A,i,j));
#endif
    }
    fprintf(outfile,"\n");
  }
  fprintf(outfile,"\n");
  return;
}

/* ----------------------------------------------------------------------------
 * Functions to access the contents of the band matrix structure
 */

long int SUNMatrixBand_Rows(SUNMatrix A)
{ return SM_ROWS_B(A); }
long int SUNMatrixBand_Columns(SUNMatrix A)
{ return SM_COLUMNS_B(A); }
long int SUNMatrixBand_LowerBandwidth(SUNMatrix A)
{ return SM_LBAND_B(A); }
long int SUNMatrixBand_UpperBandwidth(SUNMatrix A)
{ return SM_UBAND_B(A); }
long int SUNMatrixBand_StoredUpperBandwidth(SUNMatrix A)
{ return SM_SUBAND_B(A); }
realtype* SUNMatrixBand_Data(SUNMatrix A)
{ return SM_DATA_B(A); }
realtype* SUNMatrixBand_Column(SUNMatrix A, long int j)
{ return SM_COLUMN_B(A,j); }

/*
 * -----------------------------------------------------------------
 * implementation of matrix operations
 * -----------------------------------------------------------------
 */

SUNMatrix SUNMatrixClone_Band(SUNMatrix A)
{
  SUNMatrix B = SUNMatrixNew_Band(SM_COLUMNS_B(A), SM_UBAND_B(A),
                                  SM_LBAND_B(A), SM_SUBAND_B(A));
  return(B);
}

int SUNMatrixZero_Band(SUNMatrix A)
{
  long int i;
  realtype *Adata;

  /* Verify that A is a band matrix */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return 1;

  /* Perform operation */
  Adata = SM_DATA_B(A);
  for (i=0; i<SM_LDATA_B(A); i++)
    Adata[i] = ZERO;
  return 0;
}

int SUNMatrixScale_Band(realtype c, SUNMatrix A)
{
  long int i;
  realtype *Adata;

  /* Verify that A is a band matrix */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return 1;

  /* Perform operation */
  Adata = SM_DATA_B(A);
  for (i=0; i<SM_LDATA_B(A); i++)
    Adata[i] *= c;
  return 0;
}

int SUNMatrixCopy_BAND(SUNMatrix A, SUNMatrix B)
{
  long int i, j, copysize;
  realtype *A_colj, *B_colj;

  /* Verify that A and B are compatible */
  if (!SMCompatible_Band(A, B))
    return 1;

  /* Perform operation */
  copysize = SM_UBAND_B(A) + SM_LBAND_B(A) + 1;
  for (j=0; j<SM_COLUMNS_B(A); j++) {
    A_colj = SM_COLUMN_B(A,j)+SM_SUBAND_B(A)-SM_UBAND_B(A);
    B_colj = SM_COLUMN_B(B,j)+SM_SUBAND_B(B)-SM_UBAND_B(B);
    for (i=0; i<copysize; i++)
      A_colj[i] = B_colj[i];
  }
  return 0;
}

int SUNMatrixAddIdentity_Band(SUNMatrix A)
{
  long int i;

  /* Verify that A is a band matrix */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return 1;

  /* Perform operation */
  for (i=0; i<SM_COLUMNS_B(A); i++)
    SM_ELEMENT_B(A,i,i) += ONE;
  return 0;
}

int SUNMatrixAdd_Band(SUNMatrix A, SUNMatrix B)
{
  long int i, j, addsize;
  realtype *A_colj, *B_colj;

  /* Verify that A and B are compatible */
  if (!SMCompatible_Band(A, B))
    return 1;

  /* Perform operation */
  addsize = SM_UBAND_B(A) + SM_LBAND_B(A) + 1;
  for (j=0; j<SM_COLUMNS_B(A); j++) {
    A_colj = SM_COLUMN_B(A,j)+SM_SUBAND_B(A)-SM_UBAND_B(A);
    B_colj = SM_COLUMN_B(B,j)+SM_SUBAND_B(B)-SM_UBAND_B(B);
    for (i=0; i<addsize; i++)
      A_colj[i] += B_colj[i];
  }
  return 0;
}

int SUNMatrixMatvec_Band(SUNMatrix A, N_Vector x, N_Vector y)
{
  long int i, j, is, ie;
  realtype *col_j, *xd, *yd;
  
  /* Verify that A, x and y are compatible */
  if (!SMCompatible2_Band(A, x, y))
    return 1;

  /* access vector data (return if failure) */
  xd = N_VGetArrayPointer(x);
  yd = N_VGetArrayPointer(y);
  if ((xd == NULL) || (yd == NULL))
    return 1;

  /* Perform operation */
  for (i=0; i<SM_ROWS_B(A); i++)
    yd[i] = ZERO;
  for(j=0; j<SM_COLUMNS_B(A); j++) {
    col_j = SM_COLUMN_B(A,j);
    is = SUNMAX(0, j-SM_UBAND_B(A));
    ie = SUNMIN(SM_ROWS_B(A)-1, j+SM_LBAND_B(A));
    for (i=is; i<=ie; i++)
      yd[i] += col_j[i-j]*xd[j];
  }
  return 0;
}


/*
 * -----------------------------------------------------------------
 * private functions
 * -----------------------------------------------------------------
 */

static booleantype SMCompatible_Band(SUNMatrix A, SUNMatrix B)
{
  /* both matrices must be SUNMATRIX_BAND */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return FALSE;
  if (SUNMatrixGetID(B) != SUNMATRIX_BAND)
    return FALSE;

  /* both matrices must have the same shape 
     (note that we do not require the same ldim or smu, 
      only the active bandwidth) */
  if (SM_ROWS_B(A) != SM_ROWS_B(B))
    return FALSE;
  if (SM_COLUMNS_B(A) != SM_COLUMNS_B(B))
    return FALSE;
  if (SM_UBAND_B(A) != SM_UBAND_B(B))
    return FALSE;
  if (SM_LBAND_B(A) != SM_LBAND_B(B))
    return FALSE;

  return TRUE;
}


static booleantype SMCompatible2_Band(SUNMatrix A, N_Vector x, N_Vector y)
{
  /*   matrix must be SUNMATRIX_BAND */
  if (SUNMatrixGetID(A) != SUNMATRIX_BAND)
    return FALSE;

  /*   vectors must be one of {SERIAL, OPENMP, PTHREADS}, and 
       have compatible dimensions */ 
  if (N_VGetVectorID(x) == SUNDIALS_NVEC_SERIAL) {
    if (NV_LENGTH_S(x) != SM_COLUMNS_B(A))
      return FALSE;
  } else if (N_VGetVectorID(x) == SUNDIALS_NVEC_OPENMP) {
    if (NV_LENGTH_OMP(x) != SM_COLUMNS_B(A))
      return FALSE;
  } else if (N_VGetVectorID(x) == SUNDIALS_NVEC_PTHREADS) {
    if (NV_LENGTH_PT(x) != SM_COLUMNS_B(A))
      return FALSE;
  } else {   /* incompatible type */
    return FALSE;
  }
  if (N_VGetVectorID(y) == SUNDIALS_NVEC_SERIAL) {
    if (NV_LENGTH_S(y) != SM_ROWS_B(A))
      return FALSE;
  } else if (N_VGetVectorID(y) == SUNDIALS_NVEC_OPENMP) {
    if (NV_LENGTH_OMP(y) != SM_ROWS_B(A))
      return FALSE;
  } else if (N_VGetVectorID(y) == SUNDIALS_NVEC_PTHREADS) {
    if (NV_LENGTH_PT(y) != SM_ROWS_B(A))
      return FALSE;
  } else {   /* incompatible type */
    return FALSE;
  }

  return TRUE;
}

