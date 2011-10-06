#ifndef _SPLITTERJOINTER_HPP
#define _SPLITTERJOINTER_HPP

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

#include "FENode.hpp"

namespace streamit {

/**
 * Base class for all splitters and joiners.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: SplitterJoiner.java,v 1.2 2003/10/09 19:50:59 dmaze Exp $
 */
class SplitterJoiner : public FENode
{
public:
    SplitterJoiner(FEContext *context) : FENode(context)
    {
    }
    
    virtual void *accept(FEVisitor *v) = 0;
};

}

#endif
