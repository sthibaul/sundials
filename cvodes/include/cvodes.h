/*******************************************************************
 * File          : cvodes.h                                        *
 * Programmers   : Scott D. Cohen, Alan C. Hindmarsh, Radu Serban  *
 *                 and Dan Shumaker @ LLNL                         *
 * Version of    : 07 February 2004                                *
 *-----------------------------------------------------------------*
 * Copyright (c) 2002, The Regents of the University of California * 
 * Produced at the Lawrence Livermore National Laboratory          *
 * All rights reserved                                             *
 * For details, see sundials/cvodes/LICENSE                        *
 *-----------------------------------------------------------------*
 * This is the interface file for the main CVODES integrator.      *
 *******************************************************************/

#ifdef __cplusplus     /* wrapper to enable C++ usage */
extern "C" {
#endif

#ifndef _cvodes_h
#define _cvodes_h

#include <stdio.h>
#include "sundialstypes.h"
#include "nvector.h"


/*================================================================*
 * CVODES is used to solve numerically the ordinary initial value *
 * problem :                                                      *
 *                                                                *
 *                 y' = f(t,y),                                   *
 *                 y(t0) = y0,                                    *
 *                                                                *
 * where t0, y0 in R^N, and f: R x R^N -> R^N are given.          *
 *                                                                *
 * Optionally, CVODES can perform forward sensitivity analysis    *
 * to find sensitivities of the solution y with respect to        *
 * parameters in the right hand side f and/or in the initial      *
 * conditions y0.                                                 * 
 *================================================================*/


/*----------------------------------------------------------------*
 * Enumerations for inputs to CVodeCreate, CVodeMalloc,           *
 * CVodeReInit, CVodeSensMalloc, CVodeSensReInit, CvodeQuadMalloc,*
 * CVodeQuadReInit, CVodeSet*, and CVode.                         *
 *----------------------------------------------------------------*
 * Symbolic constants for the lmm, iter, and itol input           *
 * parameters to CVodeMalloc and CVodeReInit, as well as the      *
 * input parameter itask to CVode, are given below.               *
 *                                                                *
 * lmm:   The user of the CVODES package specifies whether to use *
 *        the ADAMS or BDF (backward differentiation formula)     *
 *        linear multistep method. The BDF method is recommended  *
 *        for stiff problems, and the ADAMS method is recommended *
 *        for nonstiff problems.                                  *
 *                                                                *
 * iter:  At each internal time step, a nonlinear equation must   *
 *        be solved. The user can specify either FUNCTIONAL       *
 *        iteration, which does not require linear algebra, or a  *
 *        NEWTON iteration, which requires the solution of linear *
 *        systems. In the NEWTON case, the user also specifies a  *
 *        CVODE linear solver. NEWTON is recommended in case of   *
 *        stiff problems.                                         *
 *                                                                *
 * itol:  This parameter specifies the relative and absolute      *
 *        tolerance types to be used. The SS tolerance type means *
 *        a scalar relative and absolute tolerance, while the SV  *
 *        tolerance type means a scalar relative tolerance and a  *
 *        vector absolute tolerance (a potentially different      *
 *        absolute tolerance for each vector component).          *
 *                                                                *
 * itolQ: Same as itol for quadrature variables.                  *
 *                                                                *
 * ism:   This parameter specifies the sensitivity corrector type *
 *        to be used. In the SIMULTANEOUS case, the nonlinear     *
 *        systems for states and all sensitivities are solved     *
 *        simultaneously. In the STAGGERED case, the nonlinear    *
 *        system for states is solved first and then, the         *
 *        nonlinear systems for all sensitivities are solved      *
 *        at the same time. Finally, in the STAGGERED1 approach   *     
 *        all nonlinear systems are solved in a sequence.         *
 *                                                                *
 * ifS:   Type of the function returning the sensitivity right    *
 *        hand side. ifS can be either ALLSENS if the function    *
 *        (of type SensRhsFn) returns right hand sides for all    *
 *        sensitivity systems at once, or ONESENS if the function *
 *        (of type SensRhs1Fn) returns the right hand side of one *
 *        sensitivity system at a time.                           *
 *                                                                *
 * itask: The itask input parameter to CVode indicates the job    *
 *        of the solver for the next user step. The NORMAL        *
 *        itask is to have the solver take internal steps until   *
 *        it has reached or just passed the user specified tout   *
 *        parameter. The solver then interpolates in order to     *
 *        return an approximate value of y(tout). The ONE_STEP    *
 *        option tells the solver to just take one internal step  *
 *        and return the solution at the point reached by that    *
 *        step. The NORMAL_TSTOP and ONE_STEP_TSTOP modes are     *
 *        similar to NORMAL and ONE_STEP, respectively, except    *
 *        that the integration never proceeds past the value      *
 *        tstop (specified through the routine CVodeSetStopTime). *
 *----------------------------------------------------------------*/

enum { ADAMS, BDF };                                      /* lmm */

enum { FUNCTIONAL, NEWTON };                             /* iter */

enum { SS, SV };                                  /* itol, itolQ */

enum { SIMULTANEOUS, STAGGERED, STAGGERED1 };             /* ism */

enum { NORMAL, ONE_STEP, NORMAL_TSTOP, ONE_STEP_TSTOP}; /* itask */

enum { ALLSENS, ONESENS };                                /* ifS */

/*================================================================*
 *                                                                *
 *              F U N C T I O N   T Y P E S                       *
 *                                                                *
 *================================================================*/

/*----------------------------------------------------------------*
 * Type : RhsFn                                                   *
 *----------------------------------------------------------------*        
 * The f function which defines the right hand side of the ODE    *
 * system y' = f(t,y) must have type RhsFn.                       *
 * f takes as input the independent variable value t, and the     *
 * dependent variable vector y.  It stores the result of f(t,y)   *
 * in the vector ydot.  The y and ydot arguments are of type      *
 * N_Vector.                                                      *
 * (Allocation of memory for ydot is handled within CVODES)       *
 * The f_data parameter is the same as the f_data                 *
 * parameter set by the user through the CVodeSetFdata routine.   *
 * This user-supplied pointer is passed to the user's f function  *
 * every time it is called.                                       *
 * A RhsFn f does not have a return value.                        *
 *----------------------------------------------------------------*/

typedef void (*RhsFn)(realtype t, N_Vector y, 
                      N_Vector ydot, void *f_data);

/*----------------------------------------------------------------*
 * Type : SensRhsFn                                               *
 *----------------------------------------------------------------*  
 * The fS function which defines the right hand side of the       *
 * sensitivity ODE systems s' = f_y * s + f_p must have type      *
 * SensRhsFn.                                                     *
 * fS takes as input the number of sensitivities Ns, the          *
 * independent variable value t, the states y and the             *
 * corresponding value of f(t,y) in ydot, and the dependent       *
 * sensitivity vectors yS. It stores the result of fS in ySdot.   *
 * (Allocation of memory for ySdot is handled within CVODES)      *
 * The fS_data parameter is the same as the fS_data parameter     *
 * set by the user through the CVodeSetSensFdata routine and is   *
 * passed to the fS function every time it is called.             *
 * A SensRhsFn function does not have a return value.             *
 *----------------------------------------------------------------*/

typedef void (*SensRhsFn)(int Ns, realtype t, 
                          N_Vector y, N_Vector ydot, 
                          N_Vector *yS, N_Vector *ySdot, 
                          void *fS_data,  
                          N_Vector tmp1, N_Vector tmp2);

/*----------------------------------------------------------------*
 * Type : SensRhs1Fn                                              *
 *----------------------------------------------------------------*  
 * The fS1 function which defines the right hand side of the i-th *
 * sensitivity ODE system s_i' = f_y * s_i + f_p must have type   *
 * SensRhs1Fn.                                                    *
 * fS1 takes as input the number of sensitivities Ns, the current *
 * sensitivity iS, the independent variable value t, the states y *
 * and the corresponding value of f(t,y) in ydot, and the         *
 * dependent sensitivity vector yS. It stores the result of fS in *
 * ySdot.                                                         *
 * (Allocation of memory for ySdot is handled within CVODES)      *
 * The fS_data parameter is the same as the fS_data parameter     *
 * set by the user through the CVodeSetSensFdata routine and is   *
 * passed to the fS1 function every time it is called.            *
 * A SensRhs1Fn function does not have a return value.            *
 *----------------------------------------------------------------*/

typedef void (*SensRhs1Fn)(int Ns, realtype t, 
                           N_Vector y, N_Vector ydot, 
                           int iS, N_Vector yS, N_Vector ySdot, 
                           void *fS_data,
                           N_Vector tmp1, N_Vector tmp2);

/*----------------------------------------------------------------*
 * Type : QuadRhsFn                                               *
 *----------------------------------------------------------------*        
 * The fQ function which defines the right hand side of the       *
 * quadrature equations yQ' = fQ(t,y) must have type QuadRhsFn.   *
 * fQ takes as input the value of the independent variable t,     *
 * the vector of states y and must store the result of fQ in qdot.*
 * (Allocation of memory for qdot is handled within CVODES)       *
 * The fQ_data parameter is the same as the fQ_data parameter     *
 * set by the user through the CVodeSetQuadFdata routine and is   *
 * passed to the fQ function every time it is called.             *
 * A QuadRhsFn function does not have a return value.             *
 *----------------------------------------------------------------*/

typedef void (*QuadRhsFn)(realtype t, N_Vector y, N_Vector qdot, 
                          void *fQ_data);

/*================================================================*
 *                                                                *
 *          U S E R - C A L L A B L E   R O U T I N E S           *
 *                                                                *
 *================================================================*/

/*----------------------------------------------------------------*
 * Function : CVodeCreate                                         *
 *----------------------------------------------------------------*
 * CVodeCreate creates an internal memory block for a problem to  *
 * be solved by CVODES.                                           *
 *                                                                *
 * lmm     is the type of linear multistep method to be used.     *
 *            The legal values are ADAMS and BDF (see previous    *
 *            description).                                       *
 *                                                                *
 * iter    is the type of iteration used to solve the nonlinear   *
 *            system that arises during each internal time step.  *
 *            The legal values are FUNCTIONAL and NEWTON.         *
 *                                                                *
 * If successful, CVodeCreate returns a pointer to initialized    *
 * problem memory. This pointer should be passed to CVodeMalloc.  *
 * If an initialization error occurs, CVodeCreate prints an error *
 * message to standard err and returns NULL.                      *
 *----------------------------------------------------------------*/

void *CVodeCreate(int lmm, int iter);

/*----------------------------------------------------------------*
 * Function : CVodeResetIterType                                  *
 *----------------------------------------------------------------*
 * CVodeResetIterType changes the cuurent nonlinear iteration     *
 * type. The legal values for iter are FUNCTIONAL or NEWTON.      *
 *                                                                *
 * If successful, CVodeResetIterType returns SUCCESS. Otherwise,  *
 * it returns one of the error values defined below for the       *
 * optional input specification routines.                         *
 *----------------------------------------------------------------*/

int CVodeResetIterType(void *cvode_mem, int iter);

/*----------------------------------------------------------------*
 * Integrator optional input specification functions              *
 *----------------------------------------------------------------*
 * The following functions can be called to set optional inputs   *
 * to values other than the defaults given below:                 *
 *                                                                *
 * Function             |  Optional input / [ default value ]     *
 *                      |                                         * 
 * -------------------------------------------------------------- *
 *                      |                                         * 
 * CVodeSetFdata        | a pointer to user data that will be     *
 *                      | passed to the user's f function every   *
 *                      | time f is called.                       *
 *                      | [NULL]                                  *
 *                      |                                         * 
 * CVodeSetErrFile      | the file pointer for an error file      *
 *                      | where all CVODES warning and error      *
 *                      | messages will be written. This parameter*
 *                      | can be stdout (standard output), stderr *
 *                      | (standard error), a file pointer        *
 *                      | (corresponding to a user error file     *
 *                      | opened for writing) returned by fopen.  *
 *                      | If not called, then all messages will   *
 *                      | be written to standard output.          *
 *                      | [NULL]                                  *
 *                      |                                         * 
 * CVodeSetMaxOrd       | maximum lmm order to be used by the     *
 *                      | solver.                                 *
 *                      | [12 for Adams , 5 for BDF]              * 
 *                      |                                         * 
 * CVodeSetMaxNumSteps  | maximum number of internal steps to be  *
 *                      | taken by the solver in its attempt to   *
 *                      | reach tout.                             *
 *                      | [500]                                   *
 *                      |                                         * 
 * CVodeSetMaxHnilWarns | maximum number of warning messages      *
 *                      | issued by the solver that t+h==t on the *
 *                      | next internal step. A value of -1 means *
 *                      | no such messages are issued.            *
 *                      | [10]                                    * 
 *                      |                                         * 
 * CVodeSetStabLimDet   | flag to turn on/off stability limit     *
 *                      | detection (TRUE = on, FALSE = off).     *
 *                      | When BDF is used and order is 3 or      *
 *                      | greater, CVsldet is called to detect    *
 *                      | stability limit.  If limit is detected, *
 *                      | the order is reduced.                   *
 *                      | [FALSE]                                 *
 *                      |                                         * 
 * CVodeSetInitStep     | initial step size.                      *
 *                      | [estimated by CVODES]                   * 
 *                      |                                         * 
 * CVodeSetMinStep      | minimum absolute value of step size     *
 *                      | allowed.                                *
 *                      | [0.0]                                   *
 *                      |                                         * 
 * CVodeSetMaxStep      | maximum absolute value of step size     *
 *                      | allowed.                                *
 *                      | [infinity]                              *
 *                      |                                         * 
 * CVodeSetStopTime     | the independent variable value past     *
 *                      | which the solution is not to proceed.   *
 *                      | [infinity]                              *
 *                       \                                        * 
 *                        \                                       * 
 * CVodeSetMaxErrTestFails | Maximum number of error test failures*
 *                         | in attempting one step.              *
 *                         | [7]                                  *
 *                         |                                      *
 * CVodeSetMaxNonlinIters  | Maximum number of nonlinear solver   *
 *                         | iterations at one solution.          *
 *                         | [3]                                  *
 *                         |                                      *
 * CVodeSetMaxConvFails    | Maximum number of allowable conv.    *
 *                         | failures in attempting one step.     *
 *                         | [10]                                 *
 *                         |                                      *
 * CVodeSetNonlinConvCoef  | Coeficient in the nonlinear conv.    *
 *                         | test.                                *
 *                         | [0.1]                                *
 *                         |                                      *
 * -------------------------------------------------------------- *
 * If successful, these functions return SUCCESS. If an argument  *
 * has an illegal value, they print an error message to the       *
 * file specified by errfp and return one of the error flags      *  
 * defined below.                                                 *
 *----------------------------------------------------------------*/

int CVodeSetFdata(void *cvode_mem, void *f_data);
int CVodeSetErrFile(void *cvode_mem, FILE *errfp);
int CVodeSetMaxOrd(void *cvode_mem, int maxord);
int CVodeSetMaxNumSteps(void *cvode_mem, long int mxsteps);
int CVodeSetMaxHnilWarns(void *cvode_mem, int mxhnil);
int CVodeSetStabLimDet(void *cvode_mem, booleantype stldet);
int CVodeSetInitStep(void *cvode_mem, realtype hin);
int CVodeSetMinStep(void *cvode_mem, realtype hmin);
int CVodeSetMaxStep(void *cvode_mem, realtype hmax);
int CVodeSetStopTime(void *cvode_mem, realtype tstop);
int CVodeSetMaxErrTestFails(void *cvode_mem, int maxnef);
int CVodeSetMaxNonlinIters(void *cvode_mem, int maxcor);
int CVodeSetMaxConvFails(void *cvode_mem, int maxncf);
int CVodeSetNonlinConvCoef(void *cvode_mem, realtype nlscoef);

/* Error return values for CVodeSet* functions */
/* SUCCESS = 0*/
enum {CVS_NO_MEM = -1, CVS_ILL_INPUT = -2};

/*----------------------------------------------------------------*
 * Function : CVodeMalloc                                         *
 *----------------------------------------------------------------*
 * CVodeMalloc allocates and initializes memory for a problem to  *
 * to be solved by CVODES.                                        *
 *                                                                *
 * cvode_mem is pointer to CVODES memory returned by CVodeCreate. *
 *                                                                *
 * f       is the right hand side function in y' = f(t,y).        *          
 *                                                                *
 * t0      is the initial value of t.                             *
 *                                                                *
 * y0      is the initial condition vector y(t0).                 *
 *                                                                *
 * itol    is the type of tolerances to be used.                  *
 *            The legal values are:                               *
 *               SS (scalar relative and absolute  tolerances),   *
 *               SV (scalar relative tolerance and vector         *
 *                   absolute tolerance).                         *
 *                                                                *
 * reltol  is a pointer to the relative tolerance scalar.         *
 *                                                                *
 * abstol  is a pointer to the absolute tolerance scalar or       *
 *            an N_Vector of absolute tolerances.                 *
 *                                                                *
 * nvspec  is a pointer to a vector specification structure       *
 *                                                                *
 * The parameters itol, reltol, and abstol define a vector of     *
 * error weights, ewt, with components                            *
 *   ewt[i] = 1/(reltol*abs(y[i]) + abstol)   (if itol = SS), or  *
 *   ewt[i] = 1/(reltol*abs(y[i]) + abstol[i])   (if itol = SV).  *
 * This vector is used in all error and convergence tests, which  *
 * use a weighted RMS norm on all error-like vectors v:           *
 *    WRMSnorm(v) = sqrt( (1/N) sum(i=1..N) (v[i]*ewt[i])^2 ),    *
 * where N is the problem dimension.                              *
 *                                                                *
 * Note: The tolerance values may be changed in between calls to  *
 *       CVode for the same problem. These values refer to        *
 *       (*reltol) and either (*abstol), for a scalar absolute    *
 *       tolerance, or the components of abstol, for a vector     *
 *       absolute tolerance.                                      *
 *                                                                * 
 * If successful, CVodeMalloc returns SUCCESS. If an argument has *
 * an illegal value, CVodeMalloc prints an error message to the   *
 * file specified by errfp and returns one of the error flags     *  
 * defined below.                                                 *
 *----------------------------------------------------------------*/

int CVodeMalloc(void *cvode_mem, RhsFn f,
                realtype t0, N_Vector y0, 
                int itol, realtype *reltol, void *abstol, 
                NV_Spec nvspec);

/* Error return values for CVodeMalloc */
/* SUCCESS = 0 */
enum {CVM_NO_MEM = -1, CVM_MEM_FAIL=-2, CVM_ILL_INPUT = -3};

/*----------------------------------------------------------------*
 * Function : CVodeReInit                                         *
 *----------------------------------------------------------------*
 * CVodeReInit re-initializes CVode for the solution of a problem,*
 * where a prior call to CVodeMalloc has been made with the same  *
 * problem size N. CVodeReInit performs the same input checking   *
 * and initializations that CVodeMalloc does.                     *
 * But it does no memory allocation, assuming that the existing   *
 * internal memory is sufficient for the new problem.             *
 *                                                                *
 * The use of CVodeReInit requires that the maximum method order, *
 * maxord, is no larger for the new problem than for the problem  *
 * specified in the last call to CVodeMalloc.  This condition is  *
 * automatically fulfilled if the multistep method parameter lmm  *
 * is unchanged (or changed from ADAMS to BDF) and the default    *
 * value for maxord is specified.                                 *
 *                                                                *
 * The first argument to CVodeReInit is:                          *
 *                                                                *
 * cvode_mem = pointer to CVODES memory returned by CVodeMalloc.  *
 *                                                                *
 * All the remaining arguments to CVodeReInit have names and      *
 * meanings identical to those of CVodeMalloc.                    *
 *                                                                *
 * The return value of CVodeReInit is equal to SUCCESS = 0 if     *
 * there were no errors; otherwise it is a negative int equal to: *
 *   CVREI_NO_MEM     indicating cvode_mem was NULL (i.e.,        *
 *                    CVodeCreate has not been called).           *
 *   CVREI_NO_MALLOC  indicating that cvode_mem has not been      *
 *                    allocated (i.e., CVodeMalloc has not been   *
 *                    called).                                    *
 *   CVREI_ILL_INPUT  indicating an input argument was illegal    *
 *                    (including an attempt to increase maxord).  *
 * In case of an error return, an error message is also printed.  *
 *----------------------------------------------------------------*/

int CVodeReInit(void *cvode_mem, RhsFn f,
                realtype t0, N_Vector y0, 
                int itol, realtype *reltol, void *abstol);

/* Error return values for CVodeReInit */
/* SUCCESS = 0 */ 
enum {CVREI_NO_MEM = -1, CVREI_NO_MALLOC = -2, CVREI_ILL_INPUT = -3};

/*----------------------------------------------------------------*
 * Quadrature optional input specification functions              *
 *----------------------------------------------------------------*
 * The following functions can be called to set optional inputs   *
 * to values other than the defaults given below:                 *
 *                                                                *
 *                      |                                         * 
 * Function             |  Optional input / [ default value ]     *
 *                      |                                         * 
 * -------------------------------------------------------------- *
 *                      |                                         *
 * CVodeSetQuadErrCon   | are quadrature variables considered in  *
 *                      | the error control?                      *
 *                      | [TRUE]                                  *
 *                      |                                         *
 * CVodeSetQuadFdata    | a pointer to user data that will be     *
 *                      | passed to the user's fQ function every  *
 *                      | time fQ is called.                      *
 *                      | [NULL]                                  *
 *                      |                                         *
 * -------------------------------------------------------------- *
 * If successful, these functions return SUCCESS. If an argument  *
 * has an illegal value, they print an error message to the       *
 * file specified by errfp and return one of the error flags      *  
 * defined for the CVodeSet* routines.                            *
 *----------------------------------------------------------------*/

int CVodeSetQuadErrCon(void *cvode_mem, booleantype errconQ);
int CVodeSetQuadFdata(void *cvode_mem, void *fQ_data);

/*----------------------------------------------------------------*
 * Function : CVodeQuadMalloc                                     *
 *----------------------------------------------------------------*
 * CVodeQuadMalloc allocates and initializes memory related to    *
 * quadrature integration.                                        *
 *                                                                *
 * cvode_mem is a pointer to CVODES memory returned by CVodeCreate*
 *                                                                *
 * fQ        is the user-provided integrand routine.              *
 *                                                                *
 * itolQ   is the type of tolerances to be used.                  *
 *            The legal values are:                               *
 *               SS (scalar relative and absolute  tolerances),   *
 *               SV (scalar relative tolerance and vector         *
 *                   absolute tolerance).                         *
 *                                                                *
 * reltolQ   is a pointer to the quadrature relative tolerance    *
 *           scalar.                                              *
 *                                                                *
 * abstolQ   is a pointer to the absolute tolerance scalar or     *
 *           an N_Vector of absolute tolerances for quadrature    *
 *           variables.                                           *
 *                                                                *
 * nvspecQ   is a pointer to a vector specification structure     *
 *           for N_Vectors containing quadrature variables.       *
 *                                                                *
 * If successful, CVodeQuadMalloc returns SUCCESS. If an          *
 * initialization error occurs, CVodeQuadMalloc prints an error   *
 * message to the file specified by errfp and returns one of      *
 * the error flags defined below.                                 *
 *----------------------------------------------------------------*/

int CVodeQuadMalloc(void *cvode_mem, QuadRhsFn fQ,
                    int itolQ, realtype *reltolQ, void *abstolQ,
                    NV_Spec nvspecQ);
    
enum {QCVM_NO_MEM = -1, QCVM_ILL_INPUT = -2, QCVM_MEM_FAIL = -3};

/*----------------------------------------------------------------*
 * Function : CVodeQuadReInit                                     *
 *----------------------------------------------------------------*
 * CVodeQuadReInit re-initializes CVODES's quadrature related     *
 * memory for a problem, assuming it has already been allocated   *
 * in prior calls to CVodeMalloc and CvodeQuadMalloc.             *
 *                                                                *
 * All problem specification inputs are checked for errors.       *
 * The number of quadratures Nq is assumed to be unchanged        *
 * since the previous call to CVodeQuadMalloc.                    *
 * If any error occurs during initialization, it is reported to   *
 * the file whose file pointer is errfp, and one of the error     *
 * flags defined below is returned.                               *
 *----------------------------------------------------------------*/

int CVodeQuadReInit(void *cvode_mem, QuadRhsFn fQ,
                    int itolQ, realtype *reltolQ, void *abstolQ); 

enum {QCVREI_NO_MEM = -1, QCVREI_NO_QUAD = -2, QCVREI_ILL_INPUT = -3};

/*----------------------------------------------------------------*
 * Forward sensitivity optional input specification functions     *
 *----------------------------------------------------------------*
 * The following functions can be called to set optional inputs   *
 * to other values than the defaults given below:                 *
 *                                                                *
 * Function             |  Optional input / [ default value ]     *
 *                      |                                         * 
 * -------------------------------------------------------------- *
 *                      |                                         *
 * CVodeSetSensRhsFn    | sensitivity right hand side function.   *
 *                      | This function must compute right hand   *
 *                      | sides for all sensitivity equations.    *
 *                      | [CVODES difference quotient approx.]    *
 *                      |                                         *
 * CVodeSetSensRhs1Fn   | the sensitivity right hand side.        *
 *                      | This function must compute right hand   *
 *                      | sides for one sensitivity equation at a *
 *                      | time.                                   *
 *                      | [CVODES difference quotient approx.]    *
 *                      |                                         *
 * CVodeSetSensErrCon   | are sensitivity variables considered in *
 *                      | the error control?                      *
 *                      | [TRUE]                                  *
 *                      |                                         *
 * CVodeSetSensRho      | controls the selection of finite        *
 *                      | difference schemes used in evaluating   *
 *                      | the sensitivity right hand sides.       *
 *                      | [0.0]                                   *
 *                      |                                         *
 * CVodeSetSensPbar     | a pointer to scaling factors used in    *
 *                      | computing sensitivity absolute          *
 *                      | tolerances as well as by the CVODES     *
 *                      | difference quotient routines for        *
 *                      | sensitivty right hand sides. pbar[i]    *
 *                      | must give the order of magnitude of     *
 *                      | parameter p[i]. Typically, if p[i] is   *
 *                      | nonzero, pbar[i]=p[i].                  *
 *                      | [NULL]                                  *
 *                      |                                         *
 * CVodeSetSensReltol   | a pointer to the sensitivity relative   *
 *                      | tolerance scalar.                       *
 *                      | [same as for states]                    *
 *                      |                                         *
 * CVodeSetSensAbstol   | a pointer to the array of sensitivity   *
 *                      | absolute tolerance scalars or a pointer *
 *                      | to the array of N_Vector sensitivity    *
 *                      | absolute tolerances.                    *
 *                      | [estimated by CVODES]                   *
 *                      |                                         *
 * CVodeSetSensFdata    | a pointer to user data that will be     *
 *                      | passed to the user's fS function every  *
 *                      | time fS is called.                      *
 *                      | [NULL]                                  *
 *                                                                *
 * CVodeSetSensMaxNonlinIters | Maximum number of nonlinear solver*
 *                            | iterations at one solution.       *
 *                            | [3]                               *
 *                            |                                   *
 * -------------------------------------------------------------- *
 * If successful, these functions return SUCCESS. If an argument  *
 * has an illegal value, they print an error message to the       *
 * file specified by errfp and return one of the error flags      *  
 * defined for the CVodeSet* routines.                            *
 *----------------------------------------------------------------*/

int CVodeSetSensRhsFn(void *cvode_mem, SensRhsFn fS);
int CVodeSetSensRhs1Fn(void *cvode_mem, SensRhs1Fn fS);
int CVodeSetSensErrCon(void *cvode_mem, booleantype errconS);
int CVodeSetSensRho(void *cvode_mem, realtype rho);
int CVodeSetSensPbar(void *cvode_mem, realtype *pbar);
int CVodeSetSensReltol(void *cvode_mem, realtype *reltolS);
int CVodeSetSensAbstol(void *cvode_mem, void *abstolS);
int CVodeSetSensFdata(void *cvode_mem, void *fS_data);
int CVodeSetSensMaxNonlinIters(void *cvode_mem, int maxcorS);

/*----------------------------------------------------------------*
 * Function : CVodeSensMalloc                                     *
 *----------------------------------------------------------------*
 * CVodeSensMalloc allocates and initializes memory related to    *
 * sensitivity computations.                                      *
 *                                                                *
 * cvode_mem is a pointer to CVODES memory returned by CVodeMalloc*
 *                                                                *
 * Ns        is the number of sensitivities to be computed.       *
 *                                                                *
 * ism       is the type of corrector used in sensitivity         *
 *           analysis. The legal values are: SIMULTANEOUS,        *
 *           STAGGERED, and STAGGERED1 (see previous description) *
 *                                                                *
 * p         is a pointer to problem parameters with respect to   *
 *           which sensitivities may be computed (see description *
 *           of plist below). If the right hand sides of the      *
 *           sensitivity equations are to be evaluated by the     *
 *           difference quotient routines provided with CVODES,   *
 *           then p must also be a field in the user data         *
 *           structure pointed to by f_data.                      *
 *                                                                *
 * plist     is a pointer to a list of parameters with respect to *
 *           which sensitivities are to be computed.              *
 *           If plist[j]=i, then sensitivities with respect to    *
 *           the i-th parameter (i.e. p[i-1]) will be computed.   *
 *           A negative plist entry also indicates that the       *
 *           corresponding parameter affects only the initial     *
 *           conditions of the ODE and not its right hand side.   *
 *                                                                *
 * yS0       is the array of initial condition vectors for        *
 *           sensitivity variables.                               * 
 *                                                                *
 * If successful, CVodeSensMalloc returns SUCCESS. If an          *
 * initialization error occurs, CVodeSensMalloc prints an error   *
 * message to the file specified by errfp and returns one of      *
 * the error flags defined below.                                 *
 *----------------------------------------------------------------*/

int CVodeSensMalloc(void *cvode_mem, int Ns, int ism, 
                    realtype *p, int *plist, N_Vector *yS0);
    
enum {SCVM_NO_MEM = -1, SCVM_ILL_INPUT = -2, SCVM_MEM_FAIL = -3};

/*----------------------------------------------------------------*
 * Function : CVodeSensReInit                                     *
 *----------------------------------------------------------------*
 * CVodeSensReInit re-initializes CVODES's sensitivity related    *
 * memory for a problem, assuming it has already been allocated   *
 * in prior calls to CVodeMalloc and CvodeSensMalloc.             *
 *                                                                *
 * All problem specification inputs are checked for errors.       *
 * The number of sensitivities Ns is assumed to be unchanged      *
 * since the previous call to CVodeSensMalloc.                    *
 * If any error occurs during initialization, it is reported to   *
 * the file whose file pointer is errfp.                          *
 *                                                                *
 * CVodeSensReInit potentially does some minimal memory allocation*
 * (for the sensitivity absolute tolerance and for arrays of      *
 * counters used by the STAGGERED1 method).                       *
 *                                                                *
 * The return value is equal to SUCCESS = 0 if there were no      *
 * errors; otherwise it is a negative int equal to:               *
 *   SCVREI_NO_MEM    indicating cvode_mem was NULL, or           *
 *   SCVREI_NO_SENSI  indicating there was not a prior call to    *
 *                    CVodeSensMalloc.                            *   
 *   SCVREI_ILL_INPUT indicating an input argument was illegal    *
 *                    (including an attempt to increase maxord).  *
 *   SCVREI_MEM_FAIL  indicating a memory request failed.         *
 * In case of an error return, an error message is also printed.  *
 *----------------------------------------------------------------*/

int CVodeSensReInit(void *cvode_mem, int ism,
                    realtype *p, int *plist, N_Vector *yS0);
  
enum {SCVREI_NO_MEM    = -1, SCVREI_NO_SENSI = -2, 
      SCVREI_ILL_INPUT = -3, SCVREI_MEM_FAIL = -4};

/*----------------------------------------------------------------*
 * Function : CVode                                               *
 *----------------------------------------------------------------*
 * CVode integrates the ODE over an interval in t.                *
 * If itask is NORMAL, then the solver integrates from its        *
 * current internal t value to a point at or beyond tout, then    *
 * interpolates to t = tout and returns y(tout) in the user-      *
 * allocated vector yout. If itask is ONE_STEP, then the solver   *
 * takes one internal time step and returns in yout the value of  *
 * y at the new internal time. In this case, tout is used only    *
 * during the first call to CVode to determine the direction of   *
 * integration and the rough scale of the problem. In either      *
 * case, the time reached by the solver is placed in (*t). The    *
 * user is responsible for allocating the memory for this value.  *
 *                                                                *
 * cvode_mem is the pointer to CVODES memory returned by          *
 *              CVodeMalloc.                                      *
 *                                                                *
 * tout  is the next time at which a computed solution is desired.*
 *                                                                *
 * yout  is the computed solution vector. In NORMAL mode with no  *
 *          errors, yout=y(tout).                                 *
 *                                                                *
 * tret  is a pointer to a real location. CVode sets (*t) to the  *
 *          time reached by the solver and returns yout=y(*t).    *
 *                                                                *
 * itask is NORMAL, ONE_STEP, NORMAL_TSTOP, or ONE_STEP_TSTOP.    *
 *          These four modes are described above.                 *
 *                                                                *
 * Here is a brief description of each return value:              *
 *                                                                *
 * SUCCESS       : CVode succeeded.                               *
 *                                                                *
 * TSTOP_RETURN  : CVode succeded and returned at tstop.          *
 *                                                                *
 * CVODE_NO_MEM  : The cvode_mem argument was NULL.               *
 *                                                                *
 * CVODE_NO_MALLOC: cvode_mem was not allocated.                  *
 *                                                                *
 * ILL_INPUT     : One of the inputs to CVode is illegal. This    *
 *                 includes the situation when a component of the *
 *                 error weight vectors becomes < 0 during        *
 *                 internal time-stepping. The ILL_INPUT flag     *
 *                 will also be returned if the linear solver     *
 *                 routine CV--- (called by the user after        *
 *                 calling CVodeMalloc) failed to set one of the  *
 *                 linear solver-related fields in cvode_mem or   *
 *                 if the linear solver's init routine failed. In *
 *                 any case, the user should see the printed      *
 *                 error message for more details.                *
 *                                                                *
 * TOO_MUCH_WORK : The solver took mxstep internal steps but      *
 *                 could not reach tout. The default value for    *
 *                 mxstep is MXSTEP_DEFAULT = 500.                *
 *                                                                *
 * TOO_MUCH_ACC  : The solver could not satisfy the accuracy      *
 *                 demanded by the user for some internal step.   *
 *                                                                *
 * ERR_FAILURE   : Error test failures occurred too many times    *
 *                 (= MXNEF = 7) during one internal time step or *
 *                 occurred with |h| = hmin.                      *
 *                                                                *
 * CONV_FAILURE  : Convergence test failures occurred too many    *
 *                 times (= MXNCF = 10) during one internal time  *
 *                 step or occurred with |h| = hmin.              *
 *                                                                *
 * SETUP_FAILURE : The linear solver's setup routine failed in an *
 *                 unrecoverable manner.                          *
 *                                                                *
 * SOLVE_FAILURE : The linear solver's solve routine failed in an *
 *                 unrecoverable manner.                          *
 *----------------------------------------------------------------*/

int CVode(void *cvode_mem, realtype tout, N_Vector yout, 
          realtype *tret, int itask);

/* CVode return values */
enum { SUCCESS=0, TSTOP_RETURN=1, CVODE_NO_MEM=-1, CVODE_NO_MALLOC=-2,
       ILL_INPUT=-3, TOO_MUCH_WORK=-4, TOO_MUCH_ACC=-5, ERR_FAILURE=-6, 
       CONV_FAILURE=-7, SETUP_FAILURE=-8, SOLVE_FAILURE=-9 };

/*----------------------------------------------------------------*
 * Function : CVodeGetDky                                         *
 *----------------------------------------------------------------*
 * CVodeGetDky computes the kth derivative of the y function at   *
 * time t, where tn-hu <= t <= tn, tn denotes the current         *
 * internal time reached, and hu is the last internal step size   *
 * successfully used by the solver. The user may request          *
 * k=0, 1, ..., qu, where qu is the current order. The            *
 * derivative vector is returned in dky. This vector must be      *
 * allocated by the caller. It is only legal to call this         *
 * function after a successful return from CVode.                 *
 *                                                                *
 * cvode_mem is the pointer to CVODES memory returned by          *
 *              CVodeCreate.                                      *
 *                                                                *
 * t   is the time at which the kth derivative of y is evaluated. *
 *        The legal range for t is [tn-hu,tn] as described above. *
 *                                                                *
 * k   is the order of the derivative of y to be computed. The    *
 *        legal range for k is [0,qu] as described above.         *
 *                                                                *
 * dky is the output derivative vector [(D_k)y](t).               *
 *                                                                *
 * The return values for CVodeGetDky are defined below.           *
 * Here is a brief description of each return value:              *
 *                                                                *
 * OKAY : CVodeGetDky succeeded.                                  *
 *                                                                *
 * BAD_K : k is not in the range 0, 1, ..., qu.                   *
 *                                                                *
 * BAD_T : t is not in the interval [tn-hu,tn].                   *
 *                                                                *
 * BAD_DKY : The dky argument was NULL.                           *
 *                                                                *
 * DKY_NO_MEM : The cvode_mem argument was NULL.                  *
 *----------------------------------------------------------------*/

int CVodeGetDky(void *cvode_mem, realtype t, int k, N_Vector dky);

/*----------------------------------------------------------------*
 * Integrator optional output extraction functions                *
 *----------------------------------------------------------------*
 * The following functions can be called to get optional outputs  *
 * and statistics related to the main integrator.                 *
 * -------------------------------------------------------------- *
 *                                                                *
 * CVodeGetIntWorkSpace returns the CVODES integer workspace size *
 * CVodeGetRealWorkSpace returns the CVODES real workspace size   *
 * CVodeGetNumSteps returns the cumulative number of internal     *
 *       steps taken by the solver                                *
 * CVodeGetNumRhsEvals returns the number of calls to the user's  *
 *       f function                                               *
 * CVodeGetNumLinSolvSetups returns the number of calls made to   *
 *       the linear solver's setup routine                        *
 * CVodeGetNumErrTestFails returns the number of local error test *
 *       failures that have occured                               *
 * CVodeGetLastOrder returns the order used during the last       *
 *       internal step                                            *
 * CVodeGetCurrentOrder returns the order to be used on the next  *
 *       internal step                                            *
 * CVodeGetNumStabLimOrderReds returns the number of order        *
 *       reductions due to stability limit detection              *
 * CVodeGetActualInitStep returns the actual initial step size    *
 *       used by CVODES                                           *
 * CVodeGetLastStep returns the step size for the last internal   *
 *       step                                                     *
 * CVodeGetCurrentStep returns the step size to be attempted on   *
 *       the next internal step                                   *
 * CVodeGetCurrentTime returns the current internal time reached  *
 *       by the solver                                            *
 * CVodeGetTolScaleFactor returns a suggested factor by which the *
 *       user's tolerances should be scaled when too much         *
 *       accuracy has been requested for some internal step       *
 * CVodeGetErrWeights returns the state error weight vector.      *
 *       The user need not allocate space for ewt.                *
 * CVodeGetEstLocalErrors returns the vector of estimated local   *
 *       errors. The user need not allocate space for ele.        *
 *----------------------------------------------------------------*/

int CVodeGetIntWorkSpace(void *cvode_mem, long int *leniw);
int CVodeGetRealWorkSpace(void *cvode_mem, long int *lenrw);
int CVodeGetNumSteps(void *cvode_mem, long int *nsteps);
int CVodeGetNumRhsEvals(void *cvode_mem, long int *nfevals);
int CVodeGetNumLinSolvSetups(void *cvode_mem, long int *nlinsetups);
int CVodeGetNumErrTestFails(void *cvode_mem, long int *netfails);
int CVodeGetLastOrder(void *cvode_mem, int *qlast);
int CVodeGetCurrentOrder(void *cvode_mem, int *qcur);
int CVodeGetNumStabLimOrderReds(void *cvode_mem, long int *nslred);
int CVodeGetActualInitStep(void *cvode_mem, realtype *hinused);
int CVodeGetLastStep(void *cvode_mem, realtype *hlast);
int CVodeGetCurrentStep(void *cvode_mem, realtype *hcur);
int CVodeGetCurrentTime(void *cvode_mem, realtype *tcur);
int CVodeGetTolScaleFactor(void *cvode_mem, realtype *tolsfac);
int CVodeGetErrWeights(void *cvode_mem, N_Vector *eweight);
int CVodeGetEstLocalErrors(void *cvode_mem, N_Vector *ele);
 
/*----------------------------------------------------------------*
 * As a convenience, the following two functions provide the      *
 * optional outputs in groups.                                    *
 *----------------------------------------------------------------*/

int CVodeGetWorkSpace(void *cvode_mem, long int *leniw, long int *lenrw);
int CVodeGetIntegratorStats(void *cvode_mem, long int *nsteps, long int *nfevals, 
                            long int *nlinsetups, long int *netfails, int *qlast, 
                            int *qcur, realtype *hinused, realtype *hlast, 
                            realtype *hcur, realtype *tcur);

/*----------------------------------------------------------------*
 * Nonlinear solver optional output extraction functions          *
 *----------------------------------------------------------------*
 * The following functions can be called to get optional outputs  *
 * and statistics related to the nonlinear solver.                *
 * -------------------------------------------------------------- *
 *                                                                *
 * CVodeGetNumNonlinSolvIters returns the number of nonlinear     *
 *       solver iterations performed.                             *
 * CVodeGetNumNonlinSolvConvFails returns the number of nonlinear *
 *       convergence failures.                                    *
 *----------------------------------------------------------------*/

int CVodeGetNumNonlinSolvIters(void *cvode_mem, long int *nniters);
int CVodeGetNumNonlinSolvConvFails(void *cvode_mem, long int *nncfails);

/*----------------------------------------------------------------*
 * As a convenience, the following function provides the          *
 * optional outputs in a group.                                   *
 *----------------------------------------------------------------*/

int CVodeGetNonlinSolvStats(void *cvode_mem, long int *nniters, long int *nncfails);

/*----------------------------------------------------------------*
 * Quadrature integration solution extraction routines            *
 *----------------------------------------------------------------*
 * The following functions can be called to obtain the quadrature *
 * variables after a successful integration step.                 *
 * If quadratures were not computed, they return DKY_NO_QUAD.     *
 *----------------------------------------------------------------*/

int CVodeGetQuad(void *cvode_mem, realtype t, N_Vector yQout);
int CVodeGetQuadDky(void *cvode_mem, realtype t, int k, N_Vector dky);

/*----------------------------------------------------------------*
 * Quadrature integration optional output extraction routines     *
 *----------------------------------------------------------------*
 * The following functions can be called to get optional outputs  *
 * and statistics related to the integration of quadratures.      *
 * -------------------------------------------------------------- *
 *                                                                * 
 * CVodeGetQuadNumRhsEvals returns the number of calls to the     *
 *      user function fQ defining the right hand side of the      *
 *      quadrature variables.                                     *
 * CVodeGetQuadNumErrTestFails returns the number of local error  *
 *      test failures for quadrature variables.                   *
 * CVodeGetQuadErrWeights returns the vector of error weights for *
 *      the quadrature variables. The user need not allocate      *
 *      space for ewtQ.                                           *
 *----------------------------------------------------------------*/

int CVodeGetQuadNumRhsEvals(void *cvode_mem, long int *nfQevals);
int CVodeGetQuadNumErrTestFails(void *cvode_mem, long int *nQetfails);
int CVodeGetQuadErrWeights(void *cvode_mem, N_Vector *eQweight);

/*----------------------------------------------------------------*
 * As a convenience, the following function provides the          *
 * optional outputs in a group.                                   *
 *----------------------------------------------------------------*/

int CVodeGetQuadStats(void *cvode_mem, long int *nfQevals, long int *nQetfails);

/*----------------------------------------------------------------*
 * Forward sensitivity solution extraction routines               *
 *----------------------------------------------------------------*
 * CVodeGetSensDky computes the kth derivative of the is-th       *
 * sensitivity (is=1, 2, ..., Ns) of the y function at time t,    *
 * where tn-hu <= t <= tn, tn denotes the current internal time   *
 * reached, and hu is the last internal step size successfully    *
 * used by the solver. The user may request k=0, 1, ..., qu,      *
 * where qu is the current order.                                 *
 * The is-th sensitivity derivative vector is returned in dky.    *
 * This vector must be allocated by the caller. It is only legal  *
 * to call this function after a successful return from CVode     *
 * with sensitivty computations enabled.                          *
 * Arguments have the same meaning as in CVodeDky.                *
 *                                                                * 
 * CVodeGetSensDkyAll computes the k-th derivative of all         *
 * sensitivities of the y function at time t. It repeatedly calls *
 * CVodeGetSensDky. The argument dkyA must be a pointer to        *
 * N_Vector and must be allocated by the user to hold at least Ns *
 * vectors.                                                       *
 *                                                                * 
 * CVodeGetSens returns sensitivities of the y function at        *
 * the time t. The argument yS must be a pointer to N_Vector      *
 * and must be allocated by the user to hold at least Ns vectors. *
 *                                                                *
 * Return values are similar to those of CVodeDky. Additionally,  *
 * CVodeSensDky can return DKY_NO_SENSI if sensitivities were     *
 * not computed and BAD_IS if is < 0 or is >= Ns.                 *
 *----------------------------------------------------------------*/

int CVodeGetSens(void *cvode_mem, realtype t, N_Vector *ySout);
int CVodeGetSensDky(void *cvode_mem, realtype t, int k, 
                    int is, N_Vector dky);
int CVodeGetSensDkyAll(void *cvode_mem, realtype t, int k, 
                       N_Vector *dkyA);

/*----------------------------------------------------------------*
 * Forward sensitivity optional output extraction routines        *
 *----------------------------------------------------------------*
 * The following functions can be called to get optional outputs  *
 * and statistics related to the integration of sensitivities.    *
 * -------------------------------------------------------------- *
 *                                                                * 
 * CVodeGetNumSensRhsEvals returns the number of calls to the     *
 *     sensitivity right hand side routine.                       *
 * CVodeGetNumRhsEvalsSens returns the number of calls to the     *
 *     user f routine due to finite difference evaluations of the *
 *     sensitivity equations.                                     *
 * CVodeGetNumSensErrTestFails returns the number of local error  *
 *     test failures for sensitivity variables.                   *
 * CVodeGetNumSensLinSolvSetups returns the number of calls made  *
 *     to the linear solver's setup routine due to sensitivity    *
 *     computations.                                              *
 * CVodeGetSensErrWeights returns the sensitivity error weight    *
 *     vectors. The user need not allocate space for ewtS.        *
 *----------------------------------------------------------------*/

int CVodeGetNumSensRhsEvals(void *cvode_mem, long int *nfSevals);
int CVodeGetNumRhsEvalsSens(void *cvode_mem, long int *nfevalsS);
int CVodeGetNumSensErrTestFails(void *cvode_mem, long int *nSetfails);
int CVodeGetNumSensLinSolvSetups(void *cvode_mem, long int *nlinsetupsS);
int CVodeGetSensErrWeights(void *cvode_mem, N_Vector_S *eSweight);

/*----------------------------------------------------------------*
 * As a convenience, the following function provides the          *
 * optional outputs in a group.                                   *
 *----------------------------------------------------------------*/

int CVodeGetSensStats(void *cvode_mem, long int *nfSevals, long int *nfevalsS, 
                      long int *nSetfails, long int *nlinsetupsS);

/*----------------------------------------------------------------*
 * Sensitivity nonlinear solver optional output extraction        *
 *----------------------------------------------------------------*
 * The following functions can be called to get optional outputs  *
 * and statistics related to the sensitivity nonlinear solver.    *
 * -------------------------------------------------------------- *
 *                                                                * 
 * CVodeGetNumSensNonlinSolvIters returns the total number of     *
 *         nonlinear iterations for sensitivity variables.        *
 * CVodeGetNumSensNonlinSolvConvFails returns the total number of *
 *         nonlinear convergence failures for sensitivity         *
 *         variables                                              *
 * CVodeGetNumStgrSensNonlinSolvIters returns a vector of Ns      *
 *         nonlinear iteration counters for sensitivity variables *
 *         in the STAGGERED1 method.                              *
 * CVodeGetNumStgrSensNonlinSolvConvFails returns a vector of Ns  *
 *         nonlinear solver convergence failure counters for      *
 *         sensitivity variables in the STAGGERED1 method.        *
 *----------------------------------------------------------------*/

int CVodeGetNumSensNonlinSolvIters(void *cvode_mem, long int *nSniters);
int CVodeGetNumSensNonlinSolvConvFails(void *cvode_mem, long int *nSncfails);
int CVodeGetNumStgrSensNonlinSolvIters(void *cvode_mem, long int *nSTGR1niters);
int CVodeGetNumStgrSensNonlinSolvConvFails(void *cvode_mem, long int *nSTGR1ncfails);

/*----------------------------------------------------------------*
 * As a convenience, the following two functions provide the      *
 * optional outputs in groups.                                    *
 *----------------------------------------------------------------*/

int CVodeGetSensNonlinSolvStats(void *cvode_mem, long int *nSniters, 
                                long int *nSncfails);
int CVodeGetStgrSensNonlinSolvStats(void *cvode_mem, long int *nSTGR1niters, 
                                    long int *nSTGR1ncfails);

/* CVodeGet* return values */
enum { OKAY=0, CVG_NO_MEM=-1, CVG_NO_SLDET=-2, 
       BAD_K=-3, BAD_T=-4, BAD_DKY=-5, BAD_IS=-6,
       CVG_NO_QUAD=-7, CVG_NO_SENS=-8 };

/*----------------------------------------------------------------*
 * Function : CVodeFree                                           *
 *----------------------------------------------------------------*
 * CVodeFree frees the problem memory cvode_mem allocated by      *
 * CVodeMalloc.  Its only argument is the pointer cvode_mem       *
 * returned by CVodeCreate.                                       *
 *----------------------------------------------------------------*/

void CVodeFree(void *cvode_mem);

/*----------------------------------------------------------------*
 * Function : CVodeQuadFree                                       *
 *----------------------------------------------------------------*
 * CVodeQuadFree frees the problem memory in cvode_mem allocated  *
 * for quadrature integration. Its only argument is the pointer   *
 * cvode_mem returned by CVodeCreate.                             *
 *                                                                *
 *----------------------------------------------------------------*/

void CVodeQuadFree(void *cvode_mem);

/*----------------------------------------------------------------*
 * Function : CVodeSensFree                                       *
 *----------------------------------------------------------------*
 * CVodeSensFree frees the problem memory in cvode_mem allocated  *
 * for sensitivity analysis. Its only argument is the pointer     *
 * cvode_mem returned by CVodeCreate.                             *
 *----------------------------------------------------------------*/

void CVodeSensFree(void *cvode_mem);

/*================================================================*
 *                                                                *
 *   M A I N    I N T E G R A T O R    M E M O R Y    B L O C K   *
 *                                                                *
 *================================================================*/

/* Basic CVODES constants */

#define ADAMS_Q_MAX 12      /* max value of q for lmm == ADAMS    */
#define BDF_Q_MAX    5      /* max value of q for lmm == BDF      */
#define Q_MAX  ADAMS_Q_MAX  /* max value of q for either lmm      */
#define L_MAX  (Q_MAX+1)    /* max value of L for either lmm      */
#define NUM_TESTS    5      /* number of error test quantities    */

/*----------------------------------------------------------------*
 * Types : struct CVodeMemRec, CVodeMem                           *
 *----------------------------------------------------------------*
 * The type CVodeMem is type pointer to struct CVodeMemRec. This  *
 * structure contains fields to keep track of problem state.      *
 *----------------------------------------------------------------*/
  
typedef struct CVodeMemRec {
    
  realtype cv_uround;      /* machine unit roundoff                        */

  /*---------------------------- 
    Problem Specification Data 
  ----------------------------*/

  RhsFn cv_f;              /* y' = f(t,y(t))                               */
  void *cv_f_data;         /* user pointer passed to f                     */
  int cv_lmm;              /* lmm = ADAMS or BDF                           */
  int cv_iter;             /* iter = FUNCTIONAL or NEWTON                  */
  int cv_itol;             /* itol = SS or SV                              */
  realtype *cv_reltol;     /* ptr to relative tolerance                    */
  void *cv_abstol;         /* ptr to absolute tolerance                    */

  /*--------------------------
    Quadrature Related Data 
  --------------------------*/

  booleantype cv_quad;     /* TRUE if integrating quadratures              */
  QuadRhsFn cv_fQ;
  int cv_itolQ;
  realtype *cv_reltolQ;    /* ptr to relative tolerance for quad           */
  void *cv_abstolQ;        /* ptr to absolute tolerance for quad           */
  booleantype cv_errconQ;
  void *cv_fQ_data;        /* user pointer passed to fQ                    */

  /*--------------------------
    Sensitivity Related Data 
  --------------------------*/

  booleantype cv_sensi;    /* TRUE if computing sensitivities              */
  int cv_Ns;               /* Number of sensitivities                      */
  SensRhsFn cv_fS;         /* fS = (df/dy)*yS + (df/dp)                    */
  SensRhs1Fn cv_fS1;       /* fS1 = (df/dy)*yS_i + (df/dp)                 */
  booleantype cv_fSDQ;
  int cv_ifS;              /* ifS = ALLSENS or ONESENS                     */
  int cv_ism;              /* ism = SIMULTANEOUS or STAGGERED              */
  realtype *cv_p;          /* parameters in f(t,y,p)                       */
  realtype *cv_pbar;       /* scale factors for parameters                 */
  int *cv_plist;           /* list of sensitivities                        */
  realtype *cv_reltolS;    /* ptr to relative tolerance for sensi          */
  void *cv_abstolS;        /* ptr to absolute tolerance for sensi          */
  realtype cv_rhomax;      /* cut-off value for centered/forward finite
                              differences                                  */
  booleantype cv_errconS;  /* TRUE if sensitivities are in err. control    */
  void *cv_fS_data;        /* user pointer passed to fS                    */

  /*-------------------------
    Nordsieck History Array 
  -------------------------*/

  N_Vector cv_zn[L_MAX];   /* Nordsieck array, of size N x (q+1).
                              zn[j] is a vector of length N (j=0,...,q)
                              zn[j] = [1/factorial(j)] * h^j * 
                              (jth derivative of the interpolating 
                              polynomial                                   */

  /*---------------------
    Vectors of length N 
  ---------------------*/

  N_Vector cv_ewt;         /* error weight vector                          */
  N_Vector cv_y;           /* y is used as temporary storage by the solver.
                              The memory is provided by the user to CVode 
                              where the vector is named yout.              */
  N_Vector cv_acor;        /* In the context of the solution of the
                              nonlinear equation, acor = y_n(m) - y_n(0).
                              On return, this vector is scaled to give
                              the estimated local error in y.              */
  N_Vector cv_tempv;       /* temporary storage vector                     */
  N_Vector cv_ftemp;       /* temporary storage vector                     */

  /*-----------------------------
    Quadrature Related Vectors 
  -----------------------------*/

  N_Vector cv_znQ[L_MAX];  /* Nordsieck arrays for sensitivities           */
  N_Vector cv_ewtQ;        /* error weight vector for quadratures          */
  N_Vector cv_yQ;          /* Unlike y, yQ is not allocated by the user    */
  N_Vector cv_acorQ;       /* acorQ = yQ_n(m) - yQ_n(0)                    */
  N_Vector cv_tempvQ;      /* temporary storage vector (~ tempv)           */

  /*-----------------------------
    Sensitivity Related Vectors 
  -----------------------------*/

  N_Vector *cv_znS[L_MAX]; /* Nordsieck arrays for sensitivities           */
  N_Vector *cv_ewtS;       /* error weight vectors for sensitivities       */
  N_Vector *cv_yS;         /* yS=yS0 (allocated by the user)               */
  N_Vector *cv_acorS;      /* acorS = yS_n(m) - yS_n(0)                    */
  N_Vector *cv_tempvS;     /* temporary storage vector (~ tempv)           */
  N_Vector *cv_ftempS;     /* temporary storage vector (~ ftemp)           */

  /*-------------------------------------------------
    Does CVodeSensMalloc allocate additional space?
  -------------------------------------------------*/  

  booleantype cv_abstolSalloc;   /* Is abstolS allocated by CVODES?        */
  booleantype cv_stgr1alloc;     /* Are ncfS1, ncfnS1, and nniS1 allocated 
                                    by CVODES?                             */

  /*-----------------
    Tstop information
  -------------------*/
  booleantype cv_tstopset;
  realtype cv_tstop;

  /*-----------
    Step Data 
  -----------*/

  int cv_q;                /* current order                                */
  int cv_qprime;           /* order to be used on the next step            */ 
                           /* = q-1, q, or q+1                             */
  int cv_next_q;           /* order to be used on the next step            */
  int cv_qwait;            /* number of internal steps to wait before      */
                           /* considering a change in q                    */
  int cv_L;                /* L = q + 1                                    */

  realtype cv_hin;
  realtype cv_h;           /* current step size                            */
  realtype cv_hprime;      /* step size to be used on the next step        */ 
  realtype cv_next_h;      /* step size to be used on the next step        */ 
  realtype cv_eta;         /* eta = hprime / h                             */
  realtype cv_hscale;      /* value of h used in zn                        */
  realtype cv_tn;          /* current internal value of t                  */

  realtype cv_tau[L_MAX+1];    /* array of previous q+1 successful step
                                  sizes indexed from 1 to q+1              */
  realtype cv_tq[NUM_TESTS+1]; /* array of test quantities indexed from
                                  1 to NUM_TESTS(=5)                       */
  realtype cv_l[L_MAX];        /* coefficients of l(x) (degree q poly)     */

  realtype cv_rl1;             /* 1 / l[1]                                 */
  realtype cv_gamma;           /* gamma = h * rl1                          */
  realtype cv_gammap;          /* gamma at the last setup call             */
  realtype cv_gamrat;          /* gamma / gammap                           */

  realtype cv_crate;           /* est. corrector conv. rate in Nls         */
  realtype cv_crateS;          /* est. corrector conv. rate in NlsStgr     */
  realtype cv_acnrm;           /* | acor |                                 */
  realtype cv_acnrmS;          /* | acorS |                                */
  realtype cv_acnrmQ;          /* | acorQ |                                */
  realtype cv_nlscoef;         /* coeficient in nonlinear convergence test */
  int  cv_mnewt;               /* Newton iteration counter                 */
  int  *cv_ncfS1;              /* Array of Ns local counters for conv.  
                                  failures (used in CVStep for STAGGERED1) */

  /*--------
    Limits 
  --------*/

  int cv_qmax;             /* q <= qmax                                    */
  long int cv_mxstep;      /* maximum number of internal steps for one 
                              user call                                    */
  int cv_maxcor;           /* maximum number of corrector iterations for 
                              the solution of the nonlinear equation       */
  int cv_maxcorS;
  int cv_mxhnil;           /* maximum number of warning messages issued to 
                              the user that t + h == t for the next 
                              internal step                                */
  int cv_maxnef;           /* maximum number of error test failures        */
  int cv_maxncf;           /* maximum number of nonlinear conv. failures   */

  realtype cv_hmin;        /* |h| >= hmin                                  */
  realtype cv_hmax_inv;    /* |h| <= 1/hmax_inv                            */
  realtype cv_etamax;      /* eta <= etamax                                */

  /*----------
    Counters 
  ----------*/

  long int cv_nst;              /* number of internal steps taken               */
  long int cv_nfe;              /* number of f calls                            */
  long int cv_nfSe;             /* number of fS calls                           */
  long int cv_nfQe;             /* number of fQ calls                           */
  long int cv_nfeS;             /* number of f calls from sensi DQ              */

  long int cv_ncfn;             /* number of corrector convergence failures     */
  long int cv_ncfnS;            /* number of total sensi. corr. conv. failures  */
  long int *cv_ncfnS1;          /* number of sensi. corrector conv. failures    */

  long int cv_nni;              /* number of nonlinear iterations performed     */
  long int cv_nniS;             /* number of total sensi. nonlinear iterations  */
  long int *cv_nniS1;           /* number of sensi. nonlinear iterations        */

  long int cv_netf;             /* number of error test failures                */
  long int cv_netfS;            /* number of sensi. error test failures         */
  long int cv_netfQ;            /* number of quadr. error test failures         */

  long int cv_nsetups;          /* number of setup calls                        */
  long int cv_nsetupsS;         /* number of setup calls due to sensitivities   */

  int cv_nhnil;                 /* number of messages issued to the user that
                                   t + h == t for the next iternal step         */

  /*------------------------------- 
    Space requirements for CVODES 
  -------------------------------*/

  long int cv_lrw1;        /* no. of realtype words in 1 N_Vector y        */ 
  long int cv_liw1;        /* no. of integer words in 1 N_Vector y         */ 
  long int cv_lrw1Q;       /* no. of realtype words in 1 N_Vector yQ       */ 
  long int cv_liw1Q;       /* no. of integer words in 1 N_Vector yQ        */ 
  long int cv_lrw;         /* no. of realtype words in CVODES work vectors */
  long int cv_liw;         /* no. of integer words in CVODES work vectors  */

  /*------------------
    Step size ratios
  ------------------*/

  realtype cv_etaqm1;      /* ratio of new to old h for order q-1          */
  realtype cv_etaq;        /* ratio of new to old h for order q            */
  realtype cv_etaqp1;      /* ratio of new to old h for order q+1          */

  /*--------------------
    Linear Solver Data 
  --------------------*/

  /* Linear Solver functions to be called */

  int (*cv_linit)(struct CVodeMemRec *cv_mem);

  int (*cv_lsetup)(struct CVodeMemRec *cv_mem, int convfail, 
                   N_Vector ypred, N_Vector fpred, booleantype *jcurPtr, 
                   N_Vector vtemp1, N_Vector vtemp2, N_Vector vtemp3); 

  int (*cv_lsolve)(struct CVodeMemRec *cv_mem, N_Vector b, N_Vector weight,
                   N_Vector ycur, N_Vector fcur);

  void (*cv_lfree)(struct CVodeMemRec *cv_mem);

  /* Linear Solver specific memory */

  void *cv_lmem;           

  /* Flag to request a call to the setup routine */

  booleantype cv_forceSetup;

  /*--------------
    Saved Values
  --------------*/

  int cv_qu;               /* last successful q value used                 */
  long int cv_nstlp;       /* step number of last setup call               */
  realtype cv_h0u;         /* actual initial stepsize                      */
  realtype cv_hu;          /* last successful h value used                 */
  realtype cv_saved_tq5;   /* saved value of tq[5]                         */
  booleantype cv_jcur;     /* Is the Jacobian info used by
                              linear solver current?                       */
  realtype cv_tolsf;       /* tolerance scale factor                       */
  booleantype cv_setupNonNull;  /* Does setup do something?                */

  /*-------------------------------------------------------------------
    Flags turned ON by CVodeMalloc, CVodeSensMalloc, and CVodeQuadMalloc 
    and read by CVodeMalloc, CVodeSensReInit, and CVodeQuadReInit
    ------------------------------------------------------------------*/

  booleantype cv_MallocDone;
  booleantype cv_sensMallocDone;
  booleantype cv_quadMallocDone;

  /*------------
    Error File 
  ------------*/

  FILE *cv_errfp;             /* CVODE error messages are sent to errfp    */

  /*-----------------------------------------------------
    Pointer to the vector specification structure for
    state N_Vectors
  -----------------------------------------------------*/

  NV_Spec cv_nvspec;

  /*-----------------------------------------------------
    Pointer to the vector specification structure for 
    quadrature N_Vectors
  -----------------------------------------------------*/

  NV_Spec cv_nvspecQ;

  /*-------------------------
    Stability Limit Detection
  ---------------------------*/

  booleantype cv_sldeton;     /* Is Stability Limit Detection on?          */
  realtype cv_ssdat[6][4];    /* scaled data array for STALD               */
  int cv_nscon;               /* counter for STALD method                  */
  long int cv_nor;            /* counter for number of order reductions    */

  /*-------------------------
    Complex step memory block
    -------------------------*/

  void *cv_csmem;

} *CVodeMem;


/*================================================================*
 *                                                                *
 *     I N T E R F A C E   T O    L I N E A R   S O L V E R S     *
 *                                                                *
 *================================================================*/

/*----------------------------------------------------------------*
 * Communication between user and a CVODES Linear Solver          *
 *----------------------------------------------------------------*
 * Return values of the linear solver specification routine       *
 * and of the linear solver set / get routines:                   *
 *                                                                *
 *    SUCCESS      : The routine was successful.                  *
 *                                                                *
 *    LIN_NO_MEM   : CVODES memory = NULL.                        *
 *                                                                *
 *    LMEM_FAIL    : A memory allocation failed.                  *
 *                                                                *
 *    LIN_ILL_INPUT: Some input was illegal (see message).        *
 *                                                                *
 *    LIN_NO_LMEM  : The linear solver's memory = NULL.           *
 *----------------------------------------------------------------*/

/* SUCCESS = 0  */
enum {LMEM_FAIL=-1, LIN_ILL_INPUT=-2, LIN_NO_MEM=-3, LIN_NO_LMEM=-4};

/*----------------------------------------------------------------*
 * Communication between CVODES and a CVODE Linear Solver         *
 *----------------------------------------------------------------*
 * (1) cv_linit return values                                     *
 *                                                                *
 * LINIT_OK    : The cv_linit routine succeeded.                  *
 *                                                                *
 * LINIT_ERR   : The cv_linit routine failed. Each linear solver  *
 *               init routine should print an appropriate error   *
 *               message to (cv_mem->errfp).                      *
 *                                                                *
 * (2) convfail (input to cv_lsetup)                              *
 *                                                                *
 * NO_FAILURES : Either this is the first cv_setup call for this  *
 *               step, or the local error test failed on the      *
 *               previous attempt at this step (but the Newton    *
 *               iteration converged).                            * 
 *                                                                *
 * FAIL_BAD_J  : This value is passed to cv_lsetup if             *
 *                                                                *
 *               (1) The previous Newton corrector iteration      *
 *                   did not converge and the linear solver's     *
 *                   setup routine indicated that its Jacobian-   *
 *                   related data is not current                  *
 *                                   or                           *
 *               (2) During the previous Newton corrector         *
 *                   iteration, the linear solver's solve routine *
 *                   failed in a recoverable manner and the       *
 *                   linear solver's setup routine indicated that *
 *                   its Jacobian-related data is not current.    *
 *                                                                *
 * FAIL_OTHER  : During the current internal step try, the        *
 *               previous Newton iteration failed to converge     *
 *               even though the linear solver was using current  *
 *               Jacobian-related data.                           *
 *                                                                *
 * (3) Parameter documentation, as well as a brief description    *
 *     of purpose, for each CVODES linear solver routine to be    *
 *     called in cvodes.c is given below the constant declarations*
 *     that follow.                                               *
 *----------------------------------------------------------------*/

/* cv_linit return values */

#define LINIT_OK        0
#define LINIT_ERR      -1

/* Constants for convfail (input to cv_lsetup) */

#define NO_FAILURES 0   
#define FAIL_BAD_J  1  
#define FAIL_OTHER  2  

/*----------------------------------------------------------------*
 * int (*cv_linit)(CVodeMem cv_mem);                              *
 *----------------------------------------------------------------*
 * The purpose of cv_linit is to complete initializations for a   *
 * specific linear solver, such as counters and statistics.       *
 * An LInitFn should return LINIT_OK (= 0) if it has successfully *
 * initialized the CVODES linear solver and LINIT_ERR (= -1)      *
 * otherwise. These constants are defined above.  If an error does*
 * occur, an appropriate message should be sent to (cv_mem->errfp)*
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 * int (*cv_lsetup)(CVodeMem cv_mem, int convfail, N_Vector ypred,*
 *                 N_Vector fpred, booleantype *jcurPtr,          *
 *                 N_Vector vtemp1, N_Vector vtemp2,              *
 *                 N_Vector vtemp3);                              *
 *----------------------------------------------------------------*
 * The job of cv_lsetup is to prepare the linear solver for       *
 * subsequent calls to cv_lsolve. It may recompute Jacobian-      *
 * related data is it deems necessary. Its parameters are as      *
 * follows:                                                       *
 *                                                                *
 * cv_mem - problem memory pointer of type CVodeMem. See the big  *
 *          typedef earlier in this file.                         *
 *                                                                *
 * convfail - a flag to indicate any problem that occurred during *
 *            the solution of the nonlinear equation on the       *
 *            current time step for which the linear solver is    *
 *            being used. This flag can be used to help decide    *
 *            whether the Jacobian data kept by a CVODES linear   *
 *            solver needs to be updated or not.                  *
 *            Its possible values have been documented above.     *
 *                                                                *
 * ypred - the predicted y vector for the current CVODES internal *
 *         step.                                                  *
 *                                                                *
 * fpred - f(tn, ypred).                                          *
 *                                                                *
 * jcurPtr - a pointer to a boolean to be filled in by cv_lsetup. *
 *           The function should set *jcurPtr=TRUE if its Jacobian*
 *           data is current after the call and should set        *
 *           *jcurPtr=FALSE if its Jacobian data is not current.  *
 *           Note: If cv_lsetup calls for re-evaluation of        *
 *           Jacobian data (based on convfail and CVODES state    *
 *           data), it should return *jcurPtr=TRUE always;        *
 *           otherwise an infinite loop can result.               *
 *                                                                *
 * vtemp1 - temporary N_Vector provided for use by cv_lsetup.     *
 *                                                                *
 * vtemp3 - temporary N_Vector provided for use by cv_lsetup.     *
 *                                                                *
 * vtemp3 - temporary N_Vector provided for use by cv_lsetup.     *
 *                                                                *
 * The cv_lsetup routine should return 0 if successful,           *
 * a positive value for a recoverable error, and a negative value *
 * for an unrecoverable error.                                    *
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 * int (*cv_lsolve)(CVodeMem cv_mem, N_Vector b, N_Vector weight, *
 *                  N_Vector ycur, N_Vector fcur);                *
 *----------------------------------------------------------------*
 * cv_lsolve must solve the linear equation P x = b, where        *
 * P is some approximation to (I - gamma J), J = (df/dy)(tn,ycur) *
 * and the RHS vector b is input. The N-vector ycur contains      *
 * the solver's current approximation to y(tn) and the vector     *
 * fcur contains the N_Vector f(tn,ycur). The solution is to be   *
 * returned in the vector b. cv_lsolve returns a positive value   *
 * for a recoverable error and a negative value for an            *
 * unrecoverable error. Success is indicated by a 0 return value. *
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 * void (*cv_lfree)(CVodeMem cv_mem);                             *
 *----------------------------------------------------------------*
 * cv_lfree should free up any memory allocated by the linear     *
 * solver. This routine is called once a problem has been         *
 * completed and the linear solver is no longer needed.           *
 *----------------------------------------------------------------*/


#endif

#ifdef __cplusplus
}
#endif
