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
#ifndef _SJWEIGHTEDRR_HPP_
#define _SJWEIGHTEDRR_HPP_

#include "SplitterJoiner.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * A variable-weight round-robin splitter or joiner.  This has a list
 * of expressions, each of which is a number of items to take or give
 * to a given tape.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: SJWeightedRR.java,v 1.3 2003/10/09 19:50:59 dmaze Exp $
 */
class SJWeightedRR : public SplitterJoiner
{
private:
    ExpressionList *weights;
    
public:
    /** Creates a new round-robin splitter or joiner with the specified
     * weights. */
    SJWeightedRR(FEContext *context, ExpressionList *weights) : SplitterJoiner(context),
								weights(weights)
    {
    }

    /** Returns the list of round-robin weights. */
    ExpressionList *getWeights()
    {
        return weights;
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitSJWeightedRR(this);
    }
};

}

#endif
