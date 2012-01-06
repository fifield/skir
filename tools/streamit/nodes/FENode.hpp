/*
 * Copyright 2003 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
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
