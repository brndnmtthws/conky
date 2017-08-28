#include "darwin.h"

#define STUB()  printf("%s: STUB!", __func__);

int get_entropy_avail(unsigned int *val)
{
    STUB();
    
    /* Not applicable for FreeBSD as it uses the yarrow prng. */
    (void)val;
    return 1;
}

int get_entropy_poolsize(unsigned int *val)
{
    STUB();
    
    /* Not applicable for FreeBSD as it uses the yarrow prng. */
    (void)val;
    return 1;
}
