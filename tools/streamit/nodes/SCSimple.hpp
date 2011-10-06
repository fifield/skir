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
#ifndef _SCSIMPLE_HPP_
#define _SCSIMPLE_HPP_

#include "StreamCreator.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * Stream creator that instantiates streams by name.  This creates a
 * stream given its name, a possibly empty list of type parameters,
 * and a parameter list.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: SCSimple.java,v 1.6 2003/10/09 19:50:59 dmaze Exp $
 */
class SCSimple : public  StreamCreator
{
private:
    string name;
    TypeList *types;
    ExpressionList *params;

public:    
    /**
     * Create a stream object given its name and a parameter list.
     *
     * @param context  file and line number this object corresponds to
     * @param name     name of the stream class to instantiate
     * @param types    list of <code>Type</code> giving the parameterized
     *                 type list for templated stream types
     * @param params   list of <code>Expression</code> giving the
     *                 parameter list for the stream
     * @param portals  list of <code>Expression</code> giving the
     *                 portals to register the new stream with
     */
    SCSimple(FEContext *context, string name, TypeList *types, ExpressionList *params,
	     ExpressionList *portals) : StreamCreator(context, portals)
    {
        this->name = name;
        this->types = types;
        this->params = params;
    }
    

    /** Return the name of the stream created by this. */
    string getName()
    {
        return name;
    }

    /**
     * Return the type parameter list of the stream.  This parameter list
     * is used by templated stream types, such as the built-in
     * <code>Identity</code> stream.
     *
     * @return  list of <code>Type</code>
     */
    TypeList *getTypes()
    {
        return types;
    }
    
    /**
     * Return the parameter list of the stream.
     *
     * @return  list of <code>Expression</code>
     */
    ExpressionList *getParams()
    {
        return params;
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitSCSimple(this);
    }
};

}

#endif
