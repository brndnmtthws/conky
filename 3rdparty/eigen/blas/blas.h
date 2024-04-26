#ifndef BLAS_H
#define BLAS_H

#ifdef __cplusplus
extern "C" {
#endif

#define BLASFUNC(FUNC) FUNC##_

#ifdef __WIN64__
typedef long long BLASLONG;
typedef unsigned long long BLASULONG;
#else
typedef long BLASLONG;
typedef unsigned long BLASULONG;
#endif

void BLASFUNC(xerbla)(const char *, int *info);

float BLASFUNC(sdot)(int *, float *, int *, float *, int *);
float BLASFUNC(sdsdot)(int *, float *, float *, int *, float *, int *);

double BLASFUNC(dsdot)(int *, float *, int *, float *, int *);
double BLASFUNC(ddot)(int *, double *, int *, double *, int *);
double BLASFUNC(qdot)(int *, double *, int *, double *, int *);

void BLASFUNC(cdotuw)(int *, float *, int *, float *, int *, float *);
void BLASFUNC(cdotcw)(int *, float *, int *, float *, int *, float *);
void BLASFUNC(zdotuw)(int *, double *, int *, double *, int *, double *);
void BLASFUNC(zdotcw)(int *, double *, int *, double *, int *, double *);

void BLASFUNC(saxpy)(const int *, const float *, const float *, const int *, float *, const int *);
void BLASFUNC(daxpy)(const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(qaxpy)(const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(caxpy)(const int *, const float *, const float *, const int *, float *, const int *);
void BLASFUNC(zaxpy)(const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(xaxpy)(const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(caxpyc)(const int *, const float *, const float *, const int *, float *, const int *);
void BLASFUNC(zaxpyc)(const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(xaxpyc)(const int *, const double *, const double *, const int *, double *, const int *);

void BLASFUNC(scopy)(int *, float *, int *, float *, int *);
void BLASFUNC(dcopy)(int *, double *, int *, double *, int *);
void BLASFUNC(qcopy)(int *, double *, int *, double *, int *);
void BLASFUNC(ccopy)(int *, float *, int *, float *, int *);
void BLASFUNC(zcopy)(int *, double *, int *, double *, int *);
void BLASFUNC(xcopy)(int *, double *, int *, double *, int *);

void BLASFUNC(sswap)(int *, float *, int *, float *, int *);
void BLASFUNC(dswap)(int *, double *, int *, double *, int *);
void BLASFUNC(qswap)(int *, double *, int *, double *, int *);
void BLASFUNC(cswap)(int *, float *, int *, float *, int *);
void BLASFUNC(zswap)(int *, double *, int *, double *, int *);
void BLASFUNC(xswap)(int *, double *, int *, double *, int *);

float BLASFUNC(sasum)(int *, float *, int *);
float BLASFUNC(scasum)(int *, float *, int *);
double BLASFUNC(dasum)(int *, double *, int *);
double BLASFUNC(qasum)(int *, double *, int *);
double BLASFUNC(dzasum)(int *, double *, int *);
double BLASFUNC(qxasum)(int *, double *, int *);

int BLASFUNC(isamax)(int *, float *, int *);
int BLASFUNC(idamax)(int *, double *, int *);
int BLASFUNC(iqamax)(int *, double *, int *);
int BLASFUNC(icamax)(int *, float *, int *);
int BLASFUNC(izamax)(int *, double *, int *);
int BLASFUNC(ixamax)(int *, double *, int *);

int BLASFUNC(ismax)(int *, float *, int *);
int BLASFUNC(idmax)(int *, double *, int *);
int BLASFUNC(iqmax)(int *, double *, int *);
int BLASFUNC(icmax)(int *, float *, int *);
int BLASFUNC(izmax)(int *, double *, int *);
int BLASFUNC(ixmax)(int *, double *, int *);

int BLASFUNC(isamin)(int *, float *, int *);
int BLASFUNC(idamin)(int *, double *, int *);
int BLASFUNC(iqamin)(int *, double *, int *);
int BLASFUNC(icamin)(int *, float *, int *);
int BLASFUNC(izamin)(int *, double *, int *);
int BLASFUNC(ixamin)(int *, double *, int *);

int BLASFUNC(ismin)(int *, float *, int *);
int BLASFUNC(idmin)(int *, double *, int *);
int BLASFUNC(iqmin)(int *, double *, int *);
int BLASFUNC(icmin)(int *, float *, int *);
int BLASFUNC(izmin)(int *, double *, int *);
int BLASFUNC(ixmin)(int *, double *, int *);

float BLASFUNC(samax)(int *, float *, int *);
double BLASFUNC(damax)(int *, double *, int *);
double BLASFUNC(qamax)(int *, double *, int *);
float BLASFUNC(scamax)(int *, float *, int *);
double BLASFUNC(dzamax)(int *, double *, int *);
double BLASFUNC(qxamax)(int *, double *, int *);

float BLASFUNC(samin)(int *, float *, int *);
double BLASFUNC(damin)(int *, double *, int *);
double BLASFUNC(qamin)(int *, double *, int *);
float BLASFUNC(scamin)(int *, float *, int *);
double BLASFUNC(dzamin)(int *, double *, int *);
double BLASFUNC(qxamin)(int *, double *, int *);

float BLASFUNC(smax)(int *, float *, int *);
double BLASFUNC(dmax)(int *, double *, int *);
double BLASFUNC(qmax)(int *, double *, int *);
float BLASFUNC(scmax)(int *, float *, int *);
double BLASFUNC(dzmax)(int *, double *, int *);
double BLASFUNC(qxmax)(int *, double *, int *);

float BLASFUNC(smin)(int *, float *, int *);
double BLASFUNC(dmin)(int *, double *, int *);
double BLASFUNC(qmin)(int *, double *, int *);
float BLASFUNC(scmin)(int *, float *, int *);
double BLASFUNC(dzmin)(int *, double *, int *);
double BLASFUNC(qxmin)(int *, double *, int *);

void BLASFUNC(sscal)(int *, float *, float *, int *);
void BLASFUNC(dscal)(int *, double *, double *, int *);
void BLASFUNC(qscal)(int *, double *, double *, int *);
void BLASFUNC(cscal)(int *, float *, float *, int *);
void BLASFUNC(zscal)(int *, double *, double *, int *);
void BLASFUNC(xscal)(int *, double *, double *, int *);
void BLASFUNC(csscal)(int *, float *, float *, int *);
void BLASFUNC(zdscal)(int *, double *, double *, int *);
void BLASFUNC(xqscal)(int *, double *, double *, int *);

float BLASFUNC(snrm2)(int *, float *, int *);
float BLASFUNC(scnrm2)(int *, float *, int *);

double BLASFUNC(dnrm2)(int *, double *, int *);
double BLASFUNC(qnrm2)(int *, double *, int *);
double BLASFUNC(dznrm2)(int *, double *, int *);
double BLASFUNC(qxnrm2)(int *, double *, int *);

void BLASFUNC(srot)(int *, float *, int *, float *, int *, float *, float *);
void BLASFUNC(drot)(int *, double *, int *, double *, int *, double *, double *);
void BLASFUNC(qrot)(int *, double *, int *, double *, int *, double *, double *);
void BLASFUNC(csrot)(int *, float *, int *, float *, int *, float *, float *);
void BLASFUNC(zdrot)(int *, double *, int *, double *, int *, double *, double *);
void BLASFUNC(xqrot)(int *, double *, int *, double *, int *, double *, double *);

void BLASFUNC(srotg)(float *, float *, float *, float *);
void BLASFUNC(drotg)(double *, double *, double *, double *);
void BLASFUNC(qrotg)(double *, double *, double *, double *);
void BLASFUNC(crotg)(float *, float *, float *, float *);
void BLASFUNC(zrotg)(double *, double *, double *, double *);
void BLASFUNC(xrotg)(double *, double *, double *, double *);

void BLASFUNC(srotmg)(float *, float *, float *, float *, float *);
void BLASFUNC(drotmg)(double *, double *, double *, double *, double *);

void BLASFUNC(srotm)(int *, float *, int *, float *, int *, float *);
void BLASFUNC(drotm)(int *, double *, int *, double *, int *, double *);
void BLASFUNC(qrotm)(int *, double *, int *, double *, int *, double *);

/* Level 2 routines */

void BLASFUNC(sger)(int *, int *, float *, float *, int *, float *, int *, float *, int *);
void BLASFUNC(dger)(int *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(qger)(int *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(cgeru)(int *, int *, float *, float *, int *, float *, int *, float *, int *);
void BLASFUNC(cgerc)(int *, int *, float *, float *, int *, float *, int *, float *, int *);
void BLASFUNC(zgeru)(int *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(zgerc)(int *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(xgeru)(int *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(xgerc)(int *, int *, double *, double *, int *, double *, int *, double *, int *);

void BLASFUNC(sgemv)(const char *, const int *, const int *, const float *, const float *, const int *, const float *,
                     const int *, const float *, float *, const int *);
void BLASFUNC(dgemv)(const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(qgemv)(const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(cgemv)(const char *, const int *, const int *, const float *, const float *, const int *, const float *,
                     const int *, const float *, float *, const int *);
void BLASFUNC(zgemv)(const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xgemv)(const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);

void BLASFUNC(strsv)(const char *, const char *, const char *, const int *, const float *, const int *, float *,
                     const int *);
void BLASFUNC(dtrsv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(qtrsv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(ctrsv)(const char *, const char *, const char *, const int *, const float *, const int *, float *,
                     const int *);
void BLASFUNC(ztrsv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(xtrsv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);

void BLASFUNC(stpsv)(char *, char *, char *, int *, float *, float *, int *);
void BLASFUNC(dtpsv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(qtpsv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(ctpsv)(char *, char *, char *, int *, float *, float *, int *);
void BLASFUNC(ztpsv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(xtpsv)(char *, char *, char *, int *, double *, double *, int *);

void BLASFUNC(strmv)(const char *, const char *, const char *, const int *, const float *, const int *, float *,
                     const int *);
void BLASFUNC(dtrmv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(qtrmv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(ctrmv)(const char *, const char *, const char *, const int *, const float *, const int *, float *,
                     const int *);
void BLASFUNC(ztrmv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);
void BLASFUNC(xtrmv)(const char *, const char *, const char *, const int *, const double *, const int *, double *,
                     const int *);

void BLASFUNC(stpmv)(char *, char *, char *, int *, float *, float *, int *);
void BLASFUNC(dtpmv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(qtpmv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(ctpmv)(char *, char *, char *, int *, float *, float *, int *);
void BLASFUNC(ztpmv)(char *, char *, char *, int *, double *, double *, int *);
void BLASFUNC(xtpmv)(char *, char *, char *, int *, double *, double *, int *);

void BLASFUNC(stbmv)(char *, char *, char *, int *, int *, float *, int *, float *, int *);
void BLASFUNC(dtbmv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(qtbmv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(ctbmv)(char *, char *, char *, int *, int *, float *, int *, float *, int *);
void BLASFUNC(ztbmv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(xtbmv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);

void BLASFUNC(stbsv)(char *, char *, char *, int *, int *, float *, int *, float *, int *);
void BLASFUNC(dtbsv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(qtbsv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(ctbsv)(char *, char *, char *, int *, int *, float *, int *, float *, int *);
void BLASFUNC(ztbsv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);
void BLASFUNC(xtbsv)(char *, char *, char *, int *, int *, double *, int *, double *, int *);

void BLASFUNC(ssymv)(const char *, const int *, const float *, const float *, const int *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(dsymv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);
void BLASFUNC(qsymv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);

void BLASFUNC(sspmv)(char *, int *, float *, float *, float *, int *, float *, float *, int *);
void BLASFUNC(dspmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);
void BLASFUNC(qspmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);

void BLASFUNC(ssyr)(const char *, const int *, const float *, const float *, const int *, float *, const int *);
void BLASFUNC(dsyr)(const char *, const int *, const double *, const double *, const int *, double *, const int *);
void BLASFUNC(qsyr)(const char *, const int *, const double *, const double *, const int *, double *, const int *);

void BLASFUNC(ssyr2)(const char *, const int *, const float *, const float *, const int *, const float *, const int *,
                     float *, const int *);
void BLASFUNC(dsyr2)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, double *, const int *);
void BLASFUNC(qsyr2)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, double *, const int *);
void BLASFUNC(csyr2)(const char *, const int *, const float *, const float *, const int *, const float *, const int *,
                     float *, const int *);
void BLASFUNC(zsyr2)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, double *, const int *);
void BLASFUNC(xsyr2)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, double *, const int *);

void BLASFUNC(sspr)(char *, int *, float *, float *, int *, float *);
void BLASFUNC(dspr)(char *, int *, double *, double *, int *, double *);
void BLASFUNC(qspr)(char *, int *, double *, double *, int *, double *);

void BLASFUNC(sspr2)(char *, int *, float *, float *, int *, float *, int *, float *);
void BLASFUNC(dspr2)(char *, int *, double *, double *, int *, double *, int *, double *);
void BLASFUNC(qspr2)(char *, int *, double *, double *, int *, double *, int *, double *);
void BLASFUNC(cspr2)(char *, int *, float *, float *, int *, float *, int *, float *);
void BLASFUNC(zspr2)(char *, int *, double *, double *, int *, double *, int *, double *);
void BLASFUNC(xspr2)(char *, int *, double *, double *, int *, double *, int *, double *);

void BLASFUNC(cher)(char *, int *, float *, float *, int *, float *, int *);
void BLASFUNC(zher)(char *, int *, double *, double *, int *, double *, int *);
void BLASFUNC(xher)(char *, int *, double *, double *, int *, double *, int *);

void BLASFUNC(chpr)(char *, int *, float *, float *, int *, float *);
void BLASFUNC(zhpr)(char *, int *, double *, double *, int *, double *);
void BLASFUNC(xhpr)(char *, int *, double *, double *, int *, double *);

void BLASFUNC(cher2)(char *, int *, float *, float *, int *, float *, int *, float *, int *);
void BLASFUNC(zher2)(char *, int *, double *, double *, int *, double *, int *, double *, int *);
void BLASFUNC(xher2)(char *, int *, double *, double *, int *, double *, int *, double *, int *);

void BLASFUNC(chpr2)(char *, int *, float *, float *, int *, float *, int *, float *);
void BLASFUNC(zhpr2)(char *, int *, double *, double *, int *, double *, int *, double *);
void BLASFUNC(xhpr2)(char *, int *, double *, double *, int *, double *, int *, double *);

void BLASFUNC(chemv)(const char *, const int *, const float *, const float *, const int *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(zhemv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);
void BLASFUNC(xhemv)(const char *, const int *, const double *, const double *, const int *, const double *,
                     const int *, const double *, double *, const int *);

void BLASFUNC(chpmv)(char *, int *, float *, float *, float *, int *, float *, float *, int *);
void BLASFUNC(zhpmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);
void BLASFUNC(xhpmv)(char *, int *, double *, double *, double *, int *, double *, double *, int *);

void BLASFUNC(snorm)(char *, int *, int *, float *, int *);
void BLASFUNC(dnorm)(char *, int *, int *, double *, int *);
void BLASFUNC(cnorm)(char *, int *, int *, float *, int *);
void BLASFUNC(znorm)(char *, int *, int *, double *, int *);

void BLASFUNC(sgbmv)(char *, int *, int *, int *, int *, float *, float *, int *, float *, int *, float *, float *,
                     int *);
void BLASFUNC(dgbmv)(char *, int *, int *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                     int *);
void BLASFUNC(qgbmv)(char *, int *, int *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                     int *);
void BLASFUNC(cgbmv)(char *, int *, int *, int *, int *, float *, float *, int *, float *, int *, float *, float *,
                     int *);
void BLASFUNC(zgbmv)(char *, int *, int *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                     int *);
void BLASFUNC(xgbmv)(char *, int *, int *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                     int *);

void BLASFUNC(ssbmv)(char *, int *, int *, float *, float *, int *, float *, int *, float *, float *, int *);
void BLASFUNC(dsbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);
void BLASFUNC(qsbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);
void BLASFUNC(csbmv)(char *, int *, int *, float *, float *, int *, float *, int *, float *, float *, int *);
void BLASFUNC(zsbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);
void BLASFUNC(xsbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);

void BLASFUNC(chbmv)(char *, int *, int *, float *, float *, int *, float *, int *, float *, float *, int *);
void BLASFUNC(zhbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);
void BLASFUNC(xhbmv)(char *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);

/* Level 3 routines */

void BLASFUNC(sgemm)(const char *, const char *, const int *, const int *, const int *, const float *, const float *,
                     const int *, const float *, const int *, const float *, float *, const int *);
void BLASFUNC(dgemm)(const char *, const char *, const int *, const int *, const int *, const double *, const double *,
                     const int *, const double *, const int *, const double *, double *, const int *);
void BLASFUNC(qgemm)(const char *, const char *, const int *, const int *, const int *, const double *, const double *,
                     const int *, const double *, const int *, const double *, double *, const int *);
void BLASFUNC(cgemm)(const char *, const char *, const int *, const int *, const int *, const float *, const float *,
                     const int *, const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zgemm)(const char *, const char *, const int *, const int *, const int *, const double *, const double *,
                     const int *, const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xgemm)(const char *, const char *, const int *, const int *, const int *, const double *, const double *,
                     const int *, const double *, const int *, const double *, double *, const int *);

void BLASFUNC(cgemm3m)(char *, char *, int *, int *, int *, float *, float *, int *, float *, int *, float *, float *,
                       int *);
void BLASFUNC(zgemm3m)(char *, char *, int *, int *, int *, double *, double *, int *, double *, int *, double *,
                       double *, int *);
void BLASFUNC(xgemm3m)(char *, char *, int *, int *, int *, double *, double *, int *, double *, int *, double *,
                       double *, int *);

void BLASFUNC(sge2mm)(char *, char *, char *, int *, int *, float *, float *, int *, float *, int *, float *, float *,
                      int *);
void BLASFUNC(dge2mm)(char *, char *, char *, int *, int *, double *, double *, int *, double *, int *, double *,
                      double *, int *);
void BLASFUNC(cge2mm)(char *, char *, char *, int *, int *, float *, float *, int *, float *, int *, float *, float *,
                      int *);
void BLASFUNC(zge2mm)(char *, char *, char *, int *, int *, double *, double *, int *, double *, int *, double *,
                      double *, int *);

void BLASFUNC(strsm)(const char *, const char *, const char *, const char *, const int *, const int *, const float *,
                     const float *, const int *, float *, const int *);
void BLASFUNC(dtrsm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(qtrsm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(ctrsm)(const char *, const char *, const char *, const char *, const int *, const int *, const float *,
                     const float *, const int *, float *, const int *);
void BLASFUNC(ztrsm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(xtrsm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);

void BLASFUNC(strmm)(const char *, const char *, const char *, const char *, const int *, const int *, const float *,
                     const float *, const int *, float *, const int *);
void BLASFUNC(dtrmm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(qtrmm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(ctrmm)(const char *, const char *, const char *, const char *, const int *, const int *, const float *,
                     const float *, const int *, float *, const int *);
void BLASFUNC(ztrmm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);
void BLASFUNC(xtrmm)(const char *, const char *, const char *, const char *, const int *, const int *, const double *,
                     const double *, const int *, double *, const int *);

void BLASFUNC(ssymm)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, const int *, const float *, float *, const int *);
void BLASFUNC(dsymm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(qsymm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(csymm)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zsymm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xsymm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);

void BLASFUNC(csymm3m)(char *, char *, int *, int *, float *, float *, int *, float *, int *, float *, float *, int *);
void BLASFUNC(zsymm3m)(char *, char *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                       int *);
void BLASFUNC(xsymm3m)(char *, char *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                       int *);

void BLASFUNC(ssyrk)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(dsyrk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);
void BLASFUNC(qsyrk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);
void BLASFUNC(csyrk)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(zsyrk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);
void BLASFUNC(xsyrk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);

void BLASFUNC(ssyr2k)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                      const float *, const int *, const float *, float *, const int *);
void BLASFUNC(dsyr2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);
void BLASFUNC(qsyr2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);
void BLASFUNC(csyr2k)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                      const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zsyr2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xsyr2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);

void BLASFUNC(chemm)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zhemm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xhemm)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, const int *, const double *, double *, const int *);

void BLASFUNC(chemm3m)(char *, char *, int *, int *, float *, float *, int *, float *, int *, float *, float *, int *);
void BLASFUNC(zhemm3m)(char *, char *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                       int *);
void BLASFUNC(xhemm3m)(char *, char *, int *, int *, double *, double *, int *, double *, int *, double *, double *,
                       int *);

void BLASFUNC(cherk)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                     const float *, float *, const int *);
void BLASFUNC(zherk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);
void BLASFUNC(xherk)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                     const double *, double *, const int *);

void BLASFUNC(cher2k)(const char *, const char *, const int *, const int *, const float *, const float *, const int *,
                      const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zher2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xher2k)(const char *, const char *, const int *, const int *, const double *, const double *, const int *,
                      const double *, const int *, const double *, double *, const int *);
void BLASFUNC(cher2m)(const char *, const char *, const char *, const int *, const int *, const float *, const float *,
                      const int *, const float *, const int *, const float *, float *, const int *);
void BLASFUNC(zher2m)(const char *, const char *, const char *, const int *, const int *, const double *,
                      const double *, const int *, const double *, const int *, const double *, double *, const int *);
void BLASFUNC(xher2m)(const char *, const char *, const char *, const int *, const int *, const double *,
                      const double *, const int *, const double *, const int *, const double *, double *, const int *);

#ifdef __cplusplus
}
#endif

#endif
