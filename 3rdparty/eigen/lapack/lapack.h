#ifndef LAPACK_H
#define LAPACK_H

#include "../blas/blas.h"

#ifdef __cplusplus
extern "C" {
#endif

void BLASFUNC(csymv)(const char *, const int *, const float *, const float *, const int *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(zsymv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);
void BLASFUNC(xsymv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);

void BLASFUNC(cspmv)(char *, int *, float *, float *, float *, int *, float *, float *, int *);
void BLASFUNC(zspmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);
void BLASFUNC(xspmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);

void BLASFUNC(csyr)(char *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(zsyr)(char *, int *, double *, double *, int *, double *, int *);
void BLASFUNC(xsyr)(char *, int *, double *, double *, int *, double *, int *);

void BLASFUNC(cspr)(char *, int *, float *, float *, int *, float *);
void BLASFUNC(zspr)(char *, int *, double *, double *, int *, double *);
void BLASFUNC(xspr)(char *, int *, double *, double *, int *, double *);

void BLASFUNC(sgemt)(char *, int *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(dgemt)(char *, int *, int *, double *, double *, int *, double *, int *);
void BLASFUNC(cgemt)(char *, int *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(zgemt)(char *, int *, int *, double *, double *, int *, double *, int *);

void BLASFUNC(sgema)(char *, char *, int *, int *, float *, float *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(dgema)(char *, char *, int *, int *, double *, double *, int *, double *, double *, int *, double *,
                     int *);
void BLASFUNC(cgema)(char *, char *, int *, int *, float *, float *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(zgema)(char *, char *, int *, int *, double *, double *, int *, double *, double *, int *, double *,
                     int *);

void BLASFUNC(sgems)(char *, char *, int *, int *, float *, float *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(dgems)(char *, char *, int *, int *, double *, double *, int *, double *, double *, int *, double *,
                     int *);
void BLASFUNC(cgems)(char *, char *, int *, int *, float *, float *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(zgems)(char *, char *, int *, int *, double *, double *, int *, double *, double *, int *, double *,
                     int *);

void BLASFUNC(sgetf2)(int *, int *, float *, int *, int *, int *);
void BLASFUNC(dgetf2)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(qgetf2)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(cgetf2)(int *, int *, float *, int *, int *, int *);
void BLASFUNC(zgetf2)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(xgetf2)(int *, int *, double *, int *, int *, int *);

void BLASFUNC(sgetrf)(int *, int *, float *, int *, int *, int *);
void BLASFUNC(dgetrf)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(qgetrf)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(cgetrf)(int *, int *, float *, int *, int *, int *);
void BLASFUNC(zgetrf)(int *, int *, double *, int *, int *, int *);
void BLASFUNC(xgetrf)(int *, int *, double *, int *, int *, int *);

void BLASFUNC(slaswp)(int *, float *, int *, int *, int *, int *, int *);
void BLASFUNC(dlaswp)(int *, double *, int *, int *, int *, int *, int *);
void BLASFUNC(qlaswp)(int *, double *, int *, int *, int *, int *, int *);
void BLASFUNC(claswp)(int *, float *, int *, int *, int *, int *, int *);
void BLASFUNC(zlaswp)(int *, double *, int *, int *, int *, int *, int *);
void BLASFUNC(xlaswp)(int *, double *, int *, int *, int *, int *, int *);

void BLASFUNC(sgetrs)(char *, int *, int *, float *, int *, int *, float *, int *, int *);
void BLASFUNC(dgetrs)(char *, int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(qgetrs)(char *, int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(cgetrs)(char *, int *, int *, float *, int *, int *, float *, int *, int *);
void BLASFUNC(zgetrs)(char *, int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(xgetrs)(char *, int *, int *, double *, int *, int *, double *, int *, int *);

void BLASFUNC(sgesv)(int *, int *, float *, int *, int *, float *, int *, int *);
void BLASFUNC(dgesv)(int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(qgesv)(int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(cgesv)(int *, int *, float *, int *, int *, float *, int *, int *);
void BLASFUNC(zgesv)(int *, int *, double *, int *, int *, double *, int *, int *);
void BLASFUNC(xgesv)(int *, int *, double *, int *, int *, double *, int *, int *);

void BLASFUNC(spotf2)(char *, int *, float *, int *, int *);
void BLASFUNC(dpotf2)(char *, int *, double *, int *, int *);
void BLASFUNC(qpotf2)(char *, int *, double *, int *, int *);
void BLASFUNC(cpotf2)(char *, int *, float *, int *, int *);
void BLASFUNC(zpotf2)(char *, int *, double *, int *, int *);
void BLASFUNC(xpotf2)(char *, int *, double *, int *, int *);

void BLASFUNC(spotrf)(char *, int *, float *, int *, int *);
void BLASFUNC(dpotrf)(char *, int *, double *, int *, int *);
void BLASFUNC(qpotrf)(char *, int *, double *, int *, int *);
void BLASFUNC(cpotrf)(char *, int *, float *, int *, int *);
void BLASFUNC(zpotrf)(char *, int *, double *, int *, int *);
void BLASFUNC(xpotrf)(char *, int *, double *, int *, int *);

void BLASFUNC(slauu2)(char *, int *, float *, int *, int *);
void BLASFUNC(dlauu2)(char *, int *, double *, int *, int *);
void BLASFUNC(qlauu2)(char *, int *, double *, int *, int *);
void BLASFUNC(clauu2)(char *, int *, float *, int *, int *);
void BLASFUNC(zlauu2)(char *, int *, double *, int *, int *);
void BLASFUNC(xlauu2)(char *, int *, double *, int *, int *);

void BLASFUNC(slauum)(char *, int *, float *, int *, int *);
void BLASFUNC(dlauum)(char *, int *, double *, int *, int *);
void BLASFUNC(qlauum)(char *, int *, double *, int *, int *);
void BLASFUNC(clauum)(char *, int *, float *, int *, int *);
void BLASFUNC(zlauum)(char *, int *, double *, int *, int *);
void BLASFUNC(xlauum)(char *, int *, double *, int *, int *);

void BLASFUNC(strti2)(char *, char *, int *, float *, int *, int *);
void BLASFUNC(dtrti2)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(qtrti2)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(ctrti2)(char *, char *, int *, float *, int *, int *);
void BLASFUNC(ztrti2)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(xtrti2)(char *, char *, int *, double *, int *, int *);

void BLASFUNC(strtri)(char *, char *, int *, float *, int *, int *);
void BLASFUNC(dtrtri)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(qtrtri)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(ctrtri)(char *, char *, int *, float *, int *, int *);
void BLASFUNC(ztrtri)(char *, char *, int *, double *, int *, int *);
void BLASFUNC(xtrtri)(char *, char *, int *, double *, int *, int *);

void BLASFUNC(spotri)(char *, int *, float *, int *, int *);
void BLASFUNC(dpotri)(char *, int *, double *, int *, int *);
void BLASFUNC(qpotri)(char *, int *, double *, int *, int *);
void BLASFUNC(cpotri)(char *, int *, float *, int *, int *);
void BLASFUNC(zpotri)(char *, int *, double *, int *, int *);
void BLASFUNC(xpotri)(char *, int *, double *, int *, int *);

#ifdef __cplusplus
}
#endif

#endif
