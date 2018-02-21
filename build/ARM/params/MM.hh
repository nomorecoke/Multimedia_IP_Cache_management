#ifndef __PARAMS__MM__
#define __PARAMS__MM__

class MM;


#include "params/BaseTags.hh"

struct MMParams
    : public BaseTagsParams
{
    MM * create();
};

#endif // __PARAMS__MM__
