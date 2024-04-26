
#include <stdio.h>

#if (defined __GNUC__) && (!defined __MINGW32__) && (!defined __CYGWIN__)
#define EIGEN_WEAK_LINKING __attribute__((weak))
#else
#define EIGEN_WEAK_LINKING
#endif

#ifdef __cplusplus
extern "C" {
#endif

EIGEN_WEAK_LINKING void xerbla_(const char *msg, int *info) { printf("Eigen BLAS ERROR #%i: %s\n", *info, msg); }

#ifdef __cplusplus
}
#endif
