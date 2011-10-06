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
#ifndef _EXPRVAR_HPP_
#define _EXPRVAR_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A name-indexed variable reference.  In <code>i++</code>, it's the
 * <code>i</code>.  The exact meaning of this depends on the scope in
 * which it exists; some external analysis is needed to disambiguate
 * variables and determine the types of variables.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprVar.java,v 1.6 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprVar : public Expression
{
private:
    string name;
    
public:
    /** Create a new ExprVar for a particular named variable. */
    ExprVar(FEContext *context, string name) : Expression(context)
    {
        this->name = name;
    }
    
    /** Return the name of the variable referenced. */
    string getName() { return name; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprVar(this);
    }

    /**
     * Determine if this expression can be assigned to.  Variables can
     * generally be assigned to, particularly if they are local
     * variables.  Determining whether a variable is a (constant)
     * stream parameter is beyond the intended use of this function.
     *
     * @return always true
     */
    bool isLValue()
    {
        return true;
    }

    string toString()
    {
        return name;
    }

//     int hashCode()
//     {
//         return name.hashCode();
//     }
    
//     bool equals(Object o)
//     {
//         if (!(o instanceof ExprVar))
//             return false;
//         return name.equals(((ExprVar)o).name);
//     }
};

}

#endif
