#ifndef _FENODE_HPP_
#define _FENODE_HPP_

#include "FEVisitor.hpp"
#include "FEContext.hpp"

namespace streamit {

/**
 * Any node in the tree created by the front-end's parser.  This is
 * the root of the front-end class tree.  Derived classes include
 * statement, expression, and stream object nodes.
 *
 */
class FENode
{
private:
    FEContext *context;
    
public:
    /**
     * Create a new node with the specified context.
     *
     * @param context  file and line number for the node
     */
    FENode(FEContext *ctx)
    {
        context = ctx;
    }
    
    /**
     * Returns the context associated with this node.
     *
     * @return context object with file and line number
     */
    FEContext *getContext()
    {
        return context;
    }

    /**
     * Calls an appropriate method in a visitor object with this as
     * a parameter.
     *
     * @param v  visitor object
     * @return   the value returned by the method corresponding to
     *           this type in the visitor object
     */
    virtual void *accept(FEVisitor *v) = 0;

    /* imitate java's toString */
    virtual string toString()
    {
	std::stringstream ss;
	ss << "FENode (" << std::hex << this << ")\n";
	return ss.str();
    }

    virtual bool isFunction(void) { return false; }
    virtual bool isArray(void) { return false; }
    virtual bool isArrayInit(void) { return false; }
};

}

#endif
