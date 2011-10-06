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
#ifndef _STATEMENT_HPP_
#define _STATEMENT_HPP_

#include "FENode.hpp"

namespace streamit {

/**
 * A generic statement, as created in the front-end.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: Statement.java,v 1.2 2003/10/09 19:50:59 dmaze Exp $
 */
class Statement : public FENode
{
public:
    Statement(FEContext *context) : FENode(context)
    {
    }
    
};

typedef vector<Statement *> StatementList;

}

#endif
