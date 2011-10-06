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
#ifndef _SJDUPLICATE_HPP_
#define _SJDUPLICATE_HPP_

#include "SplitterJoiner.hpp"

namespace streamit {

/**
 * A duplicating splitter.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: SJDuplicate.java,v 1.3 2003/10/09 19:50:59 dmaze Exp $
 */
class SJDuplicate : public SplitterJoiner
{
public:
    /** Creates a new duplicating splitter. */
    SJDuplicate(FEContext *context) : SplitterJoiner(context)
    {
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitSJDuplicate(this);
    }
};

}

#endif
