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
#ifndef _PROGRAM_HPP_
#define _PROGRAM_HPP_

#include "FENode.hpp"
#include "StreamSpec.hpp"
#include "TypeStruct.hpp"
#include "TypeHelper.hpp"

namespace streamit {

/**
 * An entire StreamIt program.  This includes all of the program's
 * declared streams and structure types.  It consequently has Lists of
 * streams (as {@link streamit.frontend.nodes.StreamSpec} objects) and
 * of structures (as {@link streamit.frontend.nodes.TypeStruct} objects).
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: Program.java,v 1.4 2005/06/27 21:08:51 janiss Exp $
 */
class Program : public FENode
{

private:
    //private List streams, structs, helpers;
    StreamSpecList *streams;
    TypeStructList *structs;
    TypeHelperList *helpers;

public:
    /** Creates a new StreamIt program, given lists of streams and
     * structures. */
    Program(FEContext *context, StreamSpecList *streams,
	    TypeStructList *structs, TypeHelperList *helpers) 
	: FENode(context),
	  streams(streams),
	  structs(structs),
	  helpers(helpers)
    {
    }
    
    /** Returns the list of streams declared in this. */
    StreamSpecList *getStreams()
    {
        return streams;
    }
    
    /** Returns the list of structures declared in this. */
    TypeStructList *getStructs()
    {
        return structs;
    }

    /** Returns the list of helpers declared in this. */
    TypeHelperList *getHelpers()
    {
        return helpers;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitProgram(this);
    }
};

}

#endif
