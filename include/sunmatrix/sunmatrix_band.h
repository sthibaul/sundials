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
 * This is the header file for the band implementation of the 
 * SUNMATRIX module.
 * 
 * Part I contains declarations specific to the band implementation
 * of the supplied SUNMATRIX module.
 * 
 * Part II defines accessor macros that allow the user to 
 * efficiently use this SUNMatrix type without making explicit
 * references to the underlying data structure.
 *
 * Part III contains the prototype for the constructor 
 * SUNMatrixNew_Band as well as implementation-specific prototypes 
 * for various useful matrix operations.
 *
 * Notes:
 *
 *   - The definition of the generic SUNMatrix structure can be found
 *     in the header file sundials_matrix.h.
 *
 *   - The definition of the type 'realtype' can be found in the
 *     header file sundials_types.h, and it may be changed (at the 
 *     configuration stage) according to the user's needs. 
 *     The sundials_types.h file also contains the definition
 *     for the type 'booleantype' and 'indextype'.
 *
 * -----------------------------------------------------------------
 */

#ifndef _SUNMATRIX_BAND_H
#define _SUNMATRIX_BAND_H

#include <sundials/sundials_matrix.h>
#include <nvector/nvector_serial.h>
#include <nvector/nvector_openmp.h>
#include <nvector/nvector_pthreads.h>

#ifdef __cplusplus  /* wrapper to enable C++ usage */
extern "C" {
#endif

/*
 * -----------------------------------------------------------------
 * PART I: Band implementation of SUNMatrix
 *
 * The band implementation of the SUNMatrix 'content' structure
 * contains:
 *   M     - number of rows
 *   N     - number of columns
 *   mu    - upper bandwidth, 0 <= mu <= min(M,N)
 *   ml    - lower bandwidth, 0 <= ml <= min(M,N)
 *   s_mu  - storage upper bandwidth, mu <= s_mu <= N-1.
 *           The dgbtrf routine writes the LU factors into the storage 
 *           for A. The upper triangular factor U, however, may have 
 *           an upper bandwidth as big as MIN(N-1,mu+ml) because of 
 *           partial pivoting. The s_mu field holds the upper 
 *           bandwidth allocated for A.
 *   ldim  - leading dimension (ldim >= s_mu)
 *   data  - pointer to a contiguous block of realtype variables
 *   ldata - length of the data array = ldim*(s_mu+ml+1)
 *   cols  - array of pointers. cols[j] points to the first element 
 *           of the j-th column of the matrix in the array data.
 * The elements of a band matrix are stored columnwise (i.e. columns 
 * are stored one on top of the other in memory); i.e. if A is a 
 * SUNMatrix_Band object, then the (i,j)th element of A (with 
 * 0 <= i < M and 0 <= j < N) is given by ???
 *
 * -----------------------------------------------------------------
 */
  
struct _SUNMatrixContent_Band {
  long int M;
  long int N;
  long int ldim;
  long int mu;
  long int ml;
  long int s_mu;
  realtype *data;
  long int ldata;
  realtype **cols;
};

typedef struct _SUNMatrixContent_Band *SUNMatrixContent_Band;

  
/*
 * -----------------------------------------------------------------
 * PART II: macros SM_CONTENT_B, SM_DATA_B, SM_ROWS_B, SM_COLUMNS_B, 
 *          SM_UBAND_B, SM_LBAND_B, SM_SUBAND_B, SM_COLUMN_B, 
 *          SM_COLUMN_ELEMENT_B, and SM_ELEMENT_B
 * -----------------------------------------------------------------
 * In the descriptions below, the following user declarations
 * are assumed:
 *
 * SUNMatrix A;
 * SUNMatrixContent_Band A_cont;
 * realtype *A_col_j, *A_data, A_ij;
 * long int i, j, A_rows, A_cols, A_ldata;
 *
 * (1) SM_CONTENT_B
 *
 *     This macro gives access to the contents of the band
 *     SUNMatrix
 *
 *     The assignment A_cont = SM_CONTENT_B(A) sets A_cont to be
 *     a pointer to the band SUNMatrix content structure.
 *
 * (2) SM_DATA_B, SM_ROWS_B, SM_COLUMNS_B, SM_LDATA_B, 
 *     SM_UBAND_B, SM_LBAND_B and SM_SUBAND_B
 *
 *     These macros give access to the individual parts of
 *     the content structure of a band SUNMatrix.
 *
 *     The assignment A_data = SM_DATA_B(A) sets A_data to be
 *     a pointer to the first component of A. 
 *
 *     The assignment A_rows = SM_ROWS_B(A) sets A_rows to be
 *     the number of rows in A.
 *
 *     The assignment A_cols = SM_COLUMNS_B(A) sets A_cols to be
 *     the number of columns in A.
 *
 *     The assignment A_ldata = SM_LDATA_B(A) sets A_ldata to be
 *     the length of the data array for A.
 *
 *     The assignment A_mu = SM_UBAND_B(A) sets A_mu to be
 *     the upper bandwidth of A.
 *
 *     The assignment A_ml = SM_LBAND_B(A) sets A_lu to be
 *     the lower bandwidth of A.
 *
 *     The assignment A_smu = SM_SUBAND_B(A) sets A_smu to be
 *     the storage upper bandwidth of A.
 *
 * (3) SM_COLUMN_B, SM_COLUMN_ELEMENT_B and SM_ELEMENT_B
 *
 *     These macros give access to the individual columns and 
 *     elements of a band SUNMatrix, respectively.  In the 
 *     following, the entries of a SUNMatrix are indexed (i,j) 
 *     where i=0,...,M-1 and j=0,...,N-1.
 *     
 *     The assignment A_col_j = SM_COLUMN_B(A,j) sets A_col_j to 
 *     be a pointer to the jth column of the M-by-N band
 *     matrix A, 0 <= j < N.  After the assignment, A_col_j may 
 *     be treated as an array indexed from -mu to ml.
 *     The (i,j)-th element of A is thus referenced by col_j[i-j].
 *
 *     The assignment A_ij = SM_COLUMN_ELEMENT_B(SM_COLUMN_B(A),i,j) 
 *     sets A_ij to the value of the (i,j)th element of the band 
 *     M-by-N matrix A, when used in conjunction with SM_COLUMN_B.  
 *     The index (i,j) should satisfy j-mu <= i <= j+ml, with 
 *     0 <= i < M and 0 <= j < N.  Similarly, the assignment  
 *     SM_COLUMN_ELEMENT_B(SM_COLUMN_B(A),i,j) = A_ij sets the value 
 *     of A_ij into the (i,j) location of the matrix A.
 *
 *     The assignment A_ij = SM_ELEMENT_B(A,i,j) sets A_ij to 
 *     the value of the (i,j)th element of the band M-by-N matrix
 *     A.  The location (i,j) should satisfy j-mu <= i <= j+ml, 
 *     with 0 <= i < M ; 0 <= j < N.  Similarly, the assignment 
 *     SM_ELEMENT_B(A,i,j) = A_ij sets the value of A_ij into the 
 *     (i,j) location of the matrix A.
 *
 * -----------------------------------------------------------------
 */

#define SM_CONTENT_B(A)     ( (SUNMatrixContent_Band)(A->content) )

#define SM_ROWS_B(A)        ( SM_CONTENT_B(A)->M )

#define SM_COLUMNS_B(A)     ( SM_CONTENT_B(A)->N )

#define SM_LDATA_B(A)       ( SM_CONTENT_B(A)->ldata )

#define SM_UBAND_B(A)       ( SM_CONTENT_B(A)->mu )

#define SM_LBAND_B(A)       ( SM_CONTENT_B(A)->ml )

#define SM_SUBAND_B(A)      ( SM_CONTENT_B(A)->s_mu )

#define SM_DATA_B(A)        ( SM_CONTENT_B(A)->data )

#define SM_COLUMN_B(A,j)    ( ((SM_CONTENT_B(A)->cols)[j])+SM_SUBAND_B(A) )

#define SM_COLUMN_ELEMENT_B(col_j,i,j) (col_j[(i)-(j)])

#define SM_ELEMENT_B(A,i,j) ( (SM_CONTENT_B(A)->cols)[j][(i)-(j)+SM_SUBAND_B(A)] )


/*
 * -----------------------------------------------------------------
 * PART III: functions exported by sunmatrix_band
 * 
 * CONSTRUCTORS:
 *    SUNMatrixNew_Band
 * DESTRUCTORS:
 *    SUNMatrixDestroy_Band
 * OTHER:
 *    SUNMatrixPrint_Band
 *    SUNMatrixBand_Rows
 *    SUNMatrixBand_Columns
 *    SUNMatrixBand_LowerBandwidth
 *    SUNMatrixBand_UpperBandwidth
 *    SUNMatrixBand_StoredUpperBandwidth
 *    SUNMatrixBand_Data
 *    SUNMatrixBand_Column
 * -----------------------------------------------------------------
 */


/*
 * -----------------------------------------------------------------
 * Function: SUNMatrixNew_Band
 * -----------------------------------------------------------------
 * SUNMatrixNew_Band creates and allocates memory for an M-by-N 
 * band matrix with upper bandwidth mu, lower bandwidth ml, and 
 * storage upper bandwidth smu. Pass smu as follows depending on 
 * whether A will be LU factored:
 *
 * (1) Pass smu = mu if A will not be factored.
 *
 * (2) Pass smu = MIN(N-1,mu+ml) if A will be factored.
 * -----------------------------------------------------------------
 */

SUNDIALS_EXPORT SUNMatrix SUNMatrixNew_Band(long int N, long int mu,
                                            long int ml, long int smu);

/*
 * -----------------------------------------------------------------
 * Functions: SUNMatrixPrint_Band
 * -----------------------------------------------------------------
 * This function prints the content of a M-by-N band matrix A to
 * a file pointer as it would normally appear on paper.
 * It is intended as debugging tools with small values of M and N.
 * The elements are printed using the %g/%lg/%Lg option. 
 * A blank line is printed before and after the matrix.
 * -----------------------------------------------------------------
 */

SUNDIALS_EXPORT void SUNMatrixPrint_Band(SUNMatrix A, FILE* outfile);


/*
 * -----------------------------------------------------------------
 * Accessor Functions: 
 *
 * SUNMatrixBand_Rows
 *    Returns the number of rows in the banded matrix
 *
 * SUNMatrixBand_Columns
 *    Returns the number of columns in the banded matrix
 *
 * SUNMatrixBand_LowerBandwidth
 *    Returns the number of lower bands in the banded matrix
 *
 * SUNMatrixBand_UpperBandwidth
 *    Returns the number of upper bands in the banded matrix
 *
 * SUNMatrixBand_StoredUpperBandwidth
 *    Returns the number of stored upper bands in the banded matrix
 *
 * SUNMatrixBand_Data
 *    Returns a pointer to the data array for the sparse matrix
 *
 * SUNMatrixBand_Column
 *    Returns a pointer to the diagonal entry of jth column of the
 *    banded matrix.  The resulting pointer should be indexed over 
 *    the range -mu to ml.
 * -----------------------------------------------------------------
 */

SUNDIALS_EXPORT long int SUNMatrixBand_Rows(SUNMatrix A);
SUNDIALS_EXPORT long int SUNMatrixBand_Columns(SUNMatrix A);
SUNDIALS_EXPORT long int SUNMatrixBand_LowerBandwidth(SUNMatrix A);
SUNDIALS_EXPORT long int SUNMatrixBand_UpperBandwidth(SUNMatrix A);
SUNDIALS_EXPORT long int SUNMatrixBand_StoredUpperBandwidth(SUNMatrix A);
SUNDIALS_EXPORT realtype* SUNMatrixBand_Data(SUNMatrix A);
SUNDIALS_EXPORT realtype* SUNMatrixBand_Column(SUNMatrix A, long int j);

/*
 * -----------------------------------------------------------------
 * band implementations of various useful matrix operations
 * -----------------------------------------------------------------
 */

SUNDIALS_EXPORT SUNMatrix_ID SUNMatrixGetID_Band(SUNMatrix A);
SUNDIALS_EXPORT SUNMatrix SUNMatrixClone_Band(SUNMatrix A);
SUNDIALS_EXPORT void SUNMatrixDestroy_Band(SUNMatrix A);
SUNDIALS_EXPORT int SUNMatrixZero_Band(SUNMatrix A);
SUNDIALS_EXPORT int SUNMatrixScale_Band(realtype c, SUNMatrix A);
SUNDIALS_EXPORT int SUNMatrixCopy_Band(SUNMatrix A, SUNMatrix B);
SUNDIALS_EXPORT int SUNMatrixAddIdentity_Band(SUNMatrix A);
SUNDIALS_EXPORT int SUNMatrixAdd_Band(SUNMatrix A, SUNMatrix B);
SUNDIALS_EXPORT int SUNMatrixMatvec_Band(SUNMatrix A, N_Vector x, N_Vector y);
  
#ifdef __cplusplus
}
#endif

#endif
