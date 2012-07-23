/*!---------------------------------------------------------------------
\file compiler_definitions.h
\brief

<pre>
Maintainer: Malte Neumann
            neumann@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/neumann/
            0711 - 685-6121
</pre>

---------------------------------------------------------------------*/

#ifndef COMPILER_DEFINITIONS_H
#define COMPILER_DEFINITIONS_H


/*----------------------------------------------------------------------*
 | special definitions for special compilers.....                       |
 *----------------------------------------------------------------------*/
/* append underslashs, if necessary. Important for linking to fortran routines! */

 #undef CCA_APPEND_U

/* append underslash for gnu's linux compiler gcc and g77 */
/* refer to src/fortran for the respective routines */

/* LINUX_MUENCH is still in use!  */
#ifdef LINUX_MUENCH
#define CCA_APPEND_U (1)
#endif

#ifdef CCA_APPEND_U
#define c1ab                c1ab_
#define c1inv3              c1inv3_
#define c1inv6              c1inv6_
#define c1invf              c1invf_
#define c1jacb              c1jacb_
#define colsol              colsol_
/* required for lapack access. do not remove! */
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

#define s8jacb              s8jacb_

#endif

/* fortran routines from the lapack package, used in src/linalg/linalg_utils.cpp */
#ifdef __cplusplus
extern "C"
{
#endif

void dsytrf(char *uplo, int *n, double *a, int *lda, int *ipiv, double *work, int *lwork, int *info);
void dsytri(char *uplo, int *n, double *a, int *lda, int *ipiv, double *work, int *info);
void dgetrf(int *m,int *n, double *a, int *lda, int *ipiv, int* info);
void dgetri(int *n, double *a, int *lda, int *ipiv, double *work, int *lwork, int *info);

#ifdef __cplusplus
}

#endif

#endif /* COMPILER_DEFINITIONS_H */
