#ifndef _EXPREVALVISITOR_HPP_
#define _EXPREVALVISITOR_HPP_

#include "nodes/FEPrintVisitor.hpp"

// this class evaluates simple integer expressions

namespace streamit {

class ExprEvalVisitor : public FEPrintVisitor
{
public:
    void print(std::string s) { 
	std::cout << "ExprEvalVisitor: error: cannot evaluate " << s << std::endl;
	assert(0);
    }
    
    void *visitExprConstInt(ExprConstInt *exp) {
	return (void *)exp->getVal();
    }
};

}
#endif
