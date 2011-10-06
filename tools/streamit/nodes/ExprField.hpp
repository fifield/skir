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
#ifndef _EXPRFIELD_HPP_
#define _EXPRFIELD_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A reference to a named field of a StreamIt structure.  This is
 * the expression "foo.bar".  It contains a "left" expression
 * and the name of the field being referenced.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprField.java,v 1.5 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprField : public Expression
{
private:
    Expression *left;
    string name;
    
    /** Creates a new field-reference expression, referencing the
     * named field of the specified expression. */
public:
    ExprField(FEContext *context, Expression *left, string name) : Expression(context)
    {
        this->left = left;
        this->name = name;
    }
    
    /** Returns the expression we're taking a field from. */
    Expression *getLeft() { return left; }  

    /** Returns the name of the field. */
    string getName() { return name; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprField(this);
    }

    /**
     * Determine if this expression can be assigned to.  Fields can
     * always be assigned to.
     *
     * @return always true
     */
    bool isLValue()
    {
        return true;
    }

    string toString()
    {
	string s = left->toString() + "." + name;
        return s;
    }
    
//     bool equals(Object other)
//     {
//         if (!(other instanceof ExprField))
//             return false;
//         ExprField that = (ExprField)other;
//         if (!(this.left.equals(that.left)))
//             return false;
//         if (!(this.name.equals(that.name)))
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         return left.hashCode() ^ name.hashCode();
//     }
};

}

#endif
