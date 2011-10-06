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
#ifndef _TYPEARRAY_HPP_
#define _TYPEARRAY_HPP_

#include "Type.hpp"

namespace streamit {

/**
 * A fixed-length homogenous array type.  This type has a base type and
 * an expression for the length.  The expression must be real and integral,
 * but may contain variables if they can be resolved by constant propagation.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: TypeArray.java,v 1.5 2006/06/03 15:18:23 rabbah Exp $
 */
class TypeArray : public Type
{
private:
    Type *base;
    Expression *length;
    
public:
    /** Creates an array type of the specified base type with the
     * specified length. */
    TypeArray(Type *base, Expression *length) : base(base), length(length)
    {
    }
    
    /** Gets the base type of this. */
    Type *getBase()
    {
        return base;
    }

    // RMR {
    /** Returns the number of dimensions in this array */
    int getDims() { 
        int count = 0;
        Type *dim = base;
        while (dim->isArray()) {
            dim = ((TypeArray*)dim)->getBase();
            count++;
        }
        return count + 1;
    }
    // } RMR

    /** gets the component that this array is made out of */
    Type *getComponent() 
    {
        Type *component = this;
        while (component->isArray())
            component = ((TypeArray*)component)->getBase();
        return component;
    }
    
    /** Gets the length of this. */
    Expression *getLength()
    {
        return length;
    }

//     string toString()
//     {
//         return base->toString() + "[" + length->toString() + "]";
//     }
    
    bool isArray(void) { return true; }
    
    void *accept(TypeVisitor *v) {
	return v->visitTypeArray(this);
    }
//     public boolean equals(Object other)
//     {
//         if (!(other instanceof TypeArray))
//             return false;
//         TypeArray that = (TypeArray)other;
//         if (!(this.getBase().equals(that.getBase())))
//             return false;
//         if (!(this.getLength().equals(that.getLength())))
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         return base.hashCode() ^ length.hashCode();
//     }
};
}
#endif
