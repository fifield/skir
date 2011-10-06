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
#ifndef _EXPRARRAYINIT_HPP_
#define _EXPRARRAYINIT_HPP_

namespace streamit {

#include "Expression.hpp"

/**
 * An array initializer.  This is an expression like the right hand
 * side of <code>a[3] = {1, 2, 3}</code>.  Each array initializer
 * contains only a single dimension; multi-dimensional arrays are
 * supported by nested initializers.
 *
 * NOTE that the current implementation (first checkin) assumes that
 * all of the literals are specified in the array initialization.  It
 * does not currently allow symbolic arrays as members -- for example,
 * 
 * <code>
 * A[1] = { 1 );  B[1][1] = { A };
 * </code>
 * 
 * If this behavior is going to be supported, you'll need to adjust
 * (at least) the constructor of this class, as well as the semantic
 * checker and GenerateCopies.
 *
 * @author  Bill Thies &lt;thies@mit.edu&gt;
 * @version $Id: ExprArrayInit.java,v 1.2 2006/01/25 17:04:25 thies Exp $
 */
class ExprArrayInit : public Expression
{
    /** list of Expressions that are the initial elements of the array */
private:
    ExpressionList *elements;
    
    /** number of dimensions that are initialized in this.  If all the
     * <elements> are plain Expressions, then dims=1.  If the elements
     * are also array initializers, then dims=1+elem.dims (where
     * "elem" is one of the children.
     */
    int dims;

public:
    /** Creates a new ExprArrayInit with the specified elements. */
    ExprArrayInit(FEContext *context, ExpressionList *elements) : Expression(context)
    {
        this->elements = elements;
        // determine dims based on first element.  That the elements
        // are uniform will be checked in semantic checker.
        if (elements->size()==0) {
            dims = 1;
        } else if (elements->front()->isArrayInit()) {
            dims = 1 + ((ExprArrayInit*)(elements->front()))->getDims();
        } else {
            // assumes all literals in array are specified
            dims = 1;
        }
    }
    
    /** Returns the components of this.  The returned list is a list
     * of expressions.  */
    ExpressionList *getElements() { return elements; }

    /** Returns how many dimensions in this array */
    int getDims() { return dims; }

    /** Accept a front-end visitor. */
    void * accept(FEVisitor *v)
    {
        return v->visitExprArrayInit(this);
    }

    /**
     * Determine if this expression can be assigned to.  Array
     * initializers can never be assigned to.
     *
     * @return always false
     */
    bool isLValue()
    {
        return false;
    }

    bool isArrayInit(void) { return true; }

//     public String toString()
//     {
//         StringBuffer sb = new StringBuffer();
//         sb.append("{");
//         for (int i=0; i<elements.size(); i++) {
//             sb.append(elements.get(i));
//             if (i!=elements.size()-1) {
//                 sb.append(",");
//             }
//         }
//         sb.append("}");
//         return sb.toString();
//     }
    

//     public boolean equals(Object o)
//     {
//         if (!(o instanceof ExprArrayInit))
//             return false;
//         ExprArrayInit ao = (ExprArrayInit)o;
//         for (int i=0; i<elements.size(); i++) {
//             if (!(elements.get(i).equals(ao.elements.get(i)))) {
//                 return false;
//             }
//         }
//         return true;
//     }
};

} // namespace

#endif
