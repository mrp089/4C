/*!---------------------------------------------------------------------
\file
\brief 

<pre>
Maintainer: Malte Neumann
            neumann@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/neumann/
            0711 - 685-6121
</pre>

---------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 | NOTE:                                                                |
 | - if changes or additions are made to this file, a complete recompile|
 |   of the whole code is recommended                                   |
 | - if segmentation violation errors occure in runtime, check the      |
 |   values below, as some of them are the default sizes of arrays      | 
 | - please do not define thousands of all kinds of variables, because  |
 |   they are global, do only define globally important ones            |
 | - do NOT use common words (e.g. JACOBI, NODE, ELEMENT ...)           |
 | - always use strict upper case letters                               |
 |                                                                      |
 *----------------------------------------------------------------------*/
#include "define_sizes.h"

/*----------------------------------------------------------------------*
 | some definitions that are important for dynamic memory management    |
 *----------------------------------------------------------------------*/
#ifdef SIXTYFOUR /*--------------- a 64 bit pointer is of size long int */
typedef long int PTRSIZE;
#else/*-------------------------------- a 32 bit pointer is of size INT */
typedef int PTRSIZE;
#endif

/*----------------------------------------------------------------------*
 | basic data types, do not use INT, DOUBLE or char !                   |
 *----------------------------------------------------------------------*/
#ifdef INT
#undef INT
#endif
typedef int       INT;
#ifdef DOUBLE
#undef DOUBLE
#endif
typedef double    DOUBLE;
#ifdef CHAR
#undef CHAR
#endif
typedef char      CHAR;

/*----------------------------------------------------------------------*
 | special definitions for special compilers.....                       |
 *----------------------------------------------------------------------*/
/* append underslashs, if necessary */
#undef CCA_APPEND_U

/* append underslash for gnu's linux compiler gcc and g77 */
#ifdef SUSE73 
#define CCA_APPEND_U (1)
#endif

#ifdef LINUX_MUENCH
#define CCA_APPEND_U (1)
#endif

#ifdef HPUX_GNU
#define CCA_APPEND_U (1)
#endif

/* append underslash for CUSS Sunfire */
#ifdef SUN 
#define CCA_APPEND_U (1)
#endif

/* append underslash for SIXTYFOUR flag */
#ifdef SIXTYFOUR 
#define CCA_APPEND_U (1)
#endif

/* append underslash for HPUX11i flag */
#ifdef HPUXITA 
#define CCA_APPEND_U (1)
#endif

#ifdef HPUX_MUENCH
#define CCA_APPEND_U (1)
#endif

#ifdef CCA_APPEND_U
#define c1ab                c1ab_
#define c1inv3              c1inv3_
#define c1inv6              c1inv6_
#define c1invf              c1invf_
#define c1jacb              c1jacb_
#define colsol              colsol_
#define dgesv               dgesv_
#define dgetrf              dgetrf_
#define dgetri              dgetri_
#define dgetrs              dgetrs_
#define dsyev               dsyev_
#define dsyevd              dsyevd_
#define dsygv               dsygv_
#define dsytrf              dsytrf_
#define dsytri              dsytri_
#define dsytrs              dsytrs_
#define dveczero            dveczero_
#define fortranpow          fortranpow_
#define fsdoc               fsdoc_
#define iluk                iluk_
#define iveczero            iveczero_
#define lusol               lusol_
#define mlpcgupdupdvec      mlpcgupdupdvec_
#define mlpcgupdvec         mlpcgupdvec_
#define mlpcgvecvec         mlpcgvecvec_
#define mlpcgveczero        mlpcgveczero_
#define mumps_interface     mumps_interface_
#define mxmab               mxmab_
#define mxmabt              mxmabt_
#define mxmatb              mxmatb_
#define mydsyevx            mydsyevx_
#define qat2v2              qat2v2_
#define s8jacb              s8jacb_
#define solveq              solveq_
#define sspace              sspace_
#define v2_cursor           v2_cursor_
#define v2call              v2call_
#define v2data              v2data_
#define v2grid              v2grid_
#define v2scal              v2scal_
#define v2update            v2update_
#define v2vect              v2vect_
#define v3call              v3call_
#define v3grid              v3grid_
#define v3scal              v3scal_
#define v3surface           v3surface_
#define v3update            v3update_
#define v3vect              v3vect_

#endif

#ifndef AZTEC_PACKAGE
void dsytrf(char *uplo, INT *n, DOUBLE *a, INT *lda, INT *ipiv, DOUBLE *work, INT *lwork, INT *info);
void dsytri(char *uplo, INT *n, DOUBLE *a, INT *lda, INT *ipiv, DOUBLE *work, INT *info);
void dsytrs(char *uplo, INT *n, INT *nrhs, DOUBLE *a, INT *lda, INT *ipiv, DOUBLE *b, INT *ldb, INT *info);
void dgetrf(INT *m, INT *n, DOUBLE *a, INT *lda, INT *ipiv, INT *info);
void dgetri(INT *n, DOUBLE *a, INT *lda, INT *ipiv, DOUBLE *work, INT *lwork, INT *info);
void dgetrs(char *trans, INT *n, INT *nrhs, DOUBLE *a, INT *lda, INT *ipiv, DOUBLE *b, INT *ldb, INT *info);
void dsygv(INT *itype, char *jobz, char *uplo, INT *n, DOUBLE *a, INT *lda, DOUBLE *b, INT *ldb, DOUBLE *w, DOUBLE *work, INT *lwork, INT *info);
void dsyevd(char *jobz, char *uplo, INT *n, DOUBLE *a, INT *lda, DOUBLE *w, DOUBLE *work, INT *lwork, INT *iwork, INT *liwork, INT *info);
void dsyev(char *jobz, char *uplo, INT *n, DOUBLE *a, INT *lda, DOUBLE *w, DOUBLE *work, INT *lwork, INT *info);
#endif
void colsol(DOUBLE *a, DOUBLE *v, INT *maxa, INT *nn, INT *nrr, INT *nrc, INT *nwa, INT *nqm, INT *nr1, INT *nr2, INT *kkk, DOUBLE *det, INT *isc, INT *nsch, INT *ipr, INT *info);
void iluk(INT *n, DOUBLE *a, INT *ja, INT *ia, INT *lfil, DOUBLE *alu, INT *jlu, INT *ju, INT *levs, INT *iwk, DOUBLE *w, INT *jw, INT *ierr);
void lusol(INT *n, DOUBLE *y, DOUBLE *x, DOUBLE *alu, INT *jlu, INT *ju);
void mlpcgveczero(DOUBLE *x, INT *n);
void mlpcgvecvec(DOUBLE *x, DOUBLE *y, DOUBLE *sum, INT *n);
void mlpcgupdupdvec(DOUBLE *a, DOUBLE *y, DOUBLE *facy, DOUBLE *x, DOUBLE *facx, INT *init, INT *n);
void mlpcgupdvec(DOUBLE *y, DOUBLE *x, DOUBLE *fac, INT *init, INT *n);
void dveczero(DOUBLE *x, INT *n);
void iveczero(INT *x, INT *n);
void mydsyevx(char *jobz,char *range,char *uplo,INT *n,DOUBLE *a,INT *lda,DOUBLE *vl,DOUBLE *vu,INT *il,INT *iu,DOUBLE *abstol,INT *m,DOUBLE *w,DOUBLE *z,INT *ldz,DOUBLE *work,INT *lwork,INT *iwork,INT *ifail,INT *info);
void fortranpow(DOUBLE *V,DOUBLE *R,DOUBLE *RE);
/*----------------------------------------------------------------------*
 | sign of an integer                                          |
 *----------------------------------------------------------------------*/
#define SIGN(x)    ((x) <  0  ? (-1) : (1))

/*----------------------------------------------------------------------*
 | sign of a DOUBLE                                            |
 *----------------------------------------------------------------------*/
#define FSIGN(x)   ((x) < 0.0 ? (-(1.0)) : (1.0))

/*----------------------------------------------------------------------*
 | absolut value of an integer                                          |
 *----------------------------------------------------------------------*/
#define ABS(x)    ((x) <  0  ? (-x) : (x))

/*----------------------------------------------------------------------*
 | absolut value of a DOUBLE                                            |
 *----------------------------------------------------------------------*/
#define FABS(x)   ((x) < 0.0 ? (-(x)) : (x))

/*----------------------------------------------------------------------*
 | square of a DOUBLE                                                   |
 *----------------------------------------------------------------------*/
#define DSQR(a) ((a)*(a))

/*----------------------------------------------------------------------*
 | plain old min and max (working version)                              |
 *----------------------------------------------------------------------*/
#if !defined(MAX)
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif
#if !defined(MIN)
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

/*----------------------------------------------------------------------*
 | the larger of two doubles (fast version)                             |
 *----------------------------------------------------------------------*/
/*
static DOUBLE dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b), (dmaxarg1) > (dmaxarg2) ? (dmaxarg1) : (dmaxarg2))
*/
#define DMAX MAX

/*----------------------------------------------------------------------*
 | the smaller of two doubles (fast version)                            |
 *----------------------------------------------------------------------*/
/*
static DOUBLE dminarg1,dminarg2;
#define DMIN(a,b) (dminarg1=(a),dminarg2=(b), (dminarg1) < (dminarg2) ? (dminarg1) : (dminarg2))
*/
#define DMIN MIN

/*----------------------------------------------------------------------*
 | the larger of two integer (fast version)                             |
 *----------------------------------------------------------------------*/
/*
static INT imaxarg1,imaxarg2;
#define IMAX(a,b) (imaxarg1=(a),imaxarg2=(b), (imaxarg1) > (imaxarg2) ? (imaxarg1) : (imaxarg2))
*/
#define IMAX MAX

/*----------------------------------------------------------------------*
 | the smaller of two integer (fast version)                            |
 *----------------------------------------------------------------------*/
/*
static INT iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b), (iminarg1) < (iminarg2) ? (iminarg1) : (iminarg2))
*/
#define IMIN MIN

/*----------------------------------------------------------------------*
 | max number of processors                                             |
 *----------------------------------------------------------------------*/
#define MAXPROC          (16)

/*----------------------------------------------------------------------*
 | size of buffer to attach to intra-communicator in byte               |
 *----------------------------------------------------------------------*/
#define MPIBUFFSIZE      (52428800) /* this is 50 MB */

/*----------------------------------------------------------------------*
 | exact one RAD                                                        |
 *----------------------------------------------------------------------*/
#define RAD              (atan(1.0)/45.0)

/*----------------------------------------------------------------------*
 | exact PI                                                             |
 *----------------------------------------------------------------------*/
#define PI               (asin(1.0)*2.0)

/*----------------------------------------------------------------------*
 | maximum number columns in input file                                 |
 *----------------------------------------------------------------------*/
#define MAXNUMCOL        (500)

/*----------------------------------------------------------------------*
 | maximum size of a file system is able to handle (KB)                 |
 *----------------------------------------------------------------------*/
#define MAXFILESIZE     (2000000)

/*----------------------------------------------------------------------*
 | maximum number of dofs to an element                                 |
 *----------------------------------------------------------------------*/
#define MAXDOFPERELE     (MAXNOD*MAXDOFPERNODE)

/*----------------------------------------------------------------------*
 | maximum number nonzero entries in a row of a sparse system matrix    |
 | is number of nodes to an element *                                   |
 | number of dofs to a node *                                           |
 | number of elements to a node (8) *                                   |
 | 2 (unsymmetric case)                                                 |
 *----------------------------------------------------------------------*/
#define MAX_NNZPERROW     (MAXNOD*MAXDOFPERNODE*8*2)

/*----------------------------------------------------------------------*
 | numbers                                                              |
 *----------------------------------------------------------------------*/
#define ZERO              (0.0)
#define ONE               (1.0)
#define TWO               (2.0)
#define THREE             (3.0)
#define FOUR              (4.0)
#define FIVE              (5.0)
#define SIX               (6.0)
#define SEVEN             (7.0)
#define EIGHT             (8.0)
#define NINE              (9.0)
#define TEN              (10.0)
#define ELEVEN           (11.0)
#define TWELVE           (12.0)
/*----------------------------------------------------------------------*
 | a set of different tolerances                                        |
 *----------------------------------------------------------------------*/
#define EPS5             (1.0E-05)
#define EPS6             (1.0E-06)
#define EPS7             (1.0E-07)
#define EPS8             (1.0E-08)
#define EPS9             (1.0E-09)
#define EPS10            (1.0E-10)
#define EPS11            (1.0E-11)
#define EPS12            (1.0E-12)
#define EPS13            (1.0E-13)
#define EPS14            (1.0E-14)
#define EPS15            (1.0E-15)
/*----------------------------------------------------------------------*
 | a set of numbers                                                     |
 *----------------------------------------------------------------------*/
#define VERYLARGEINT     (1000000000)
#define VERYLARGEREAL    (1000000000.0)
