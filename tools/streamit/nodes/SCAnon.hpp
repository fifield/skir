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
#ifndef _SCANON_HPP_
#define _SCANON_HPP_

#include "StreamCreator.hpp"

namespace streamit {

/**
 * Stream creator for anonymous streams.  It has a
 * <code>StreamSpec</code> object which completely specifies the new
 * stream being created.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: SCAnon.java,v 1.6 2003/10/09 19:50:59 dmaze Exp $
 */
class SCAnon : public StreamCreator
{
private:
    StreamSpec *spec;
    
public:
    /**
     * Creates a new anonymous stream given its specification.
     *
     * @param context  file and line number this object corresponds to
     * @param spec     contents of the anonymous stream
     * @param portals  list of <code>Expression</code> giving the
     *                 portals to register the new stream with
     */
    SCAnon(FEContext *context, StreamSpec *spec, ExpressionList *portals) 
	: StreamCreator(context, portals)
    {
        this->spec = spec;
    }
    
    /**
     * Creates a new anonymous stream given the type of stream and
     * its init function.
     *
     * @param context  file and line number this object corresponds to
     * @param type     type of stream, as one of the constants in
     *                 <code>StreamSpec</code>
     * @param init     contents of the stream's initialization code
     * @param portals  list of <code>Expression</code> giving the
     *                 portals to register the new stream with
     */
    SCAnon(FEContext *context, int type, Statement *init, ExpressionList *portals)
	: StreamCreator(context, portals)
    {
        this->spec = new StreamSpec(context, type, NULL, string(),
				    new ParameterList, init);
    }
    
    /**
     * Returns the stream specification this creates.
     *
     * @return  specification of the child stream
     */
    StreamSpec *getSpec()
    {
        return spec;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitSCAnon(this);
    }

    bool isAnon(void) {
	return true;
    }
};

}

#endif
