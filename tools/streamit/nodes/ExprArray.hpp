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

#ifndef _EXPRARRAY_HPP_
#define _EXPRARRAY_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * An array-element reference.  This is an expression like
 * <code>a[n]</code>.  There is a base expression (the "a") and an
 * offset expresion (the "n").
 * 
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprArray.java,v 1.5 2006/03/16 21:16:51 madrake Exp $
 */
class ExprArray : public Expression
{
private:
    Expression *base, *offset;

public:
    /** Creates a new ExprArray with the specified base and offset. */
    ExprArray(FEContext *context, Expression *base, Expression *offset) : Expression(context)
    {
        this->base = base;
        this->offset = offset;
    }
    
    /** Returns the base expression of this. */
    Expression *getBase() { return base; }

    /** Returns the component expression of this. */
    ExprVar *getComponent() {
        Expression *comp = this;
        while (comp->isArray())
            comp = ((ExprArray*)comp)->getBase();
        return (ExprVar*)comp;
    }

    /** Returns the offset expression of this. */
    Expression *getOffset() { return offset; }

    /**
     * Determine if this expression can be assigned to.  Array
     * elements can always be assigned to.
     *
     * @return always true
     */
    bool isLValue()
    {
        return true;
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprArray(this);
    }

    string toString()
    {
	string s = base->toString() + "[" + offset->toString() + "]";
        return s;
    }
    
    bool isArray(void) { return true; }

//     public int hashCode()
//     {
//         return base.hashCode() ^ offset.hashCode();
//     }
    
//     public boolean equals(Object o)
//     {
//         if (!(o instanceof ExprArray))
//             return false;
//         ExprArray ao = (ExprArray)o;
//         return (ao.base.equals(base) && ao.offset.equals(offset));
//     }
};

} //namespace

#endif
