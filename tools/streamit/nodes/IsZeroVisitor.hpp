#ifndef _ISZEROVISITOR_HPP_
#define _ISZEROVISITOR_HPP_

#include "FEVisitor.hpp"

namespace streamit {

class IsZeroVisitor : public FEVisitor
{
public:
    virtual void *visitExprConstInt(ExprConstInt *exp) { 
	if (exp->getVal() == 0)
	    return (void *)1;
	return NULL;
    }
};

}

#endif
