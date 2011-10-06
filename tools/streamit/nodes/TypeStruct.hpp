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
#ifndef _TYPESTRUCT_HPP_
#define _TYPESTRUCT_HPP_

#include "FEContext.hpp"
#include "Type.hpp"

#include <map>
using std::map;

namespace streamit {

typedef vector<string> NameList;

/**
 * A hetereogeneous structure type.  This type has a name for itself,
 * and an ordered list of field names and types.  You can retrieve the
 * list of names, and the type a particular name maps to.  The names
 * must be unique within a given structure.
 *<p>
 * There is an important assumption in testing for equality and
 * type promotions: two structure types are considered equal if
 * they have the same name, regardless of any other characteristics.
 * This allows structures and associated structure-reference types
 * to sanely compare equal.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: TypeStruct.java,v 1.6 2006/01/25 17:04:25 thies Exp $
 */
class TypeStruct : public Type
{
private:
    FEContext *context;
    string name;
    NameList *fields;
    map<string,Type*> types;

public:
    /**
     * Creates a new structured type.  The fields and ftypes lists must
     * be the same length; a field in a given position in the fields
     * list has the type in the equivalent position in the ftypes list.
     *
     * @param context  file and line number the structure was declared in
     * @param name     name of the structure
     * @param fields   list of <code>String</code> containing the names
     *                 of the fields
     * @param ftypes   list of <code>Type</code> containing the types of
     *                 the fields
     */
    TypeStruct(FEContext *context, string name, NameList *fields, TypeList *ftypes)
    {
        this->context = context;
        this->name = name;
        this->fields = fields;
        //this->types = new HashMap();
        for (unsigned int i = 0; i < fields->size(); i++)
            this->types[(*fields)[i]] = (*ftypes)[i];
    }

    /**
     * Returns the context of the structure in the original source code.
     *
     * @return file name and line number the structure was declared in
     */
    FEContext *getContext()
    {
        return context;
    }
    
    /**
     * Returns the name of the structure.
     *
     * @return the name of the structure
     */
    string getName()
    {
        return name;
    }
    
    /**
     * Returns the number of fields.
     *
     * @return the number of fields in the structure
     */
    int getNumFields()
    {
        return fields->size();
    }
    
    /**
     * Returns the name of the specified field.
     *
     * @param n zero-based index of the field to get the name of
     * @return  the name of the nth field
     */
    string getField(int n)
    {
        return (*fields)[n];
    }
    
    /**
     * Returns the type of the field with the specified name.
     *
     * @param f the name of the field to get the type of
     * @return  the type of the field named f
     */
    Type *getType(string f)
    {
        return types[f];
    }

//     // Remember, equality and such only test on the name.
//     public boolean equals(Object other)
//     {
//         if (other instanceof TypeStruct)
//             {
//                 TypeStruct that = (TypeStruct)other;
//                 return this.name.equals(that.name);
//             }
        
//         if (other instanceof TypeStructRef)
//             {
//                 TypeStructRef that = (TypeStructRef)other;
//                 return name.equals(that.getName());
//             }
        
//         if (this.isComplex() && other instanceof Type)
//             return ((Type)other).isComplex();
        
//         return false;
//     }
    
//     int hashCode()
//     {
//         return name.hashCode();
//     }
    
    string toString()
    {
        return name;
    }

    void *accept(TypeVisitor *v)
    {
	return v->visitTypeStruct(this);
    }
};

typedef vector<TypeStruct*> TypeStructList;

}

#endif
