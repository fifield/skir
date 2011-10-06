#ifndef _TYPEHELPER_HPP_
#define _TYPEHELPER_HPP_

#include "FEContext.hpp"
#include "Type.hpp"
#include "Function.hpp"

namespace streamit {

/**
 * Declares helper functions.
 *
 * @author  Janis Sermulins
 */
class TypeHelper : public Type
{
private:
    FEContext *context;
    string name;
    FunctionList *funcs;
    int cls;

public:
    enum {
	NORMAL_HELPERS = 0,
	NATIVE_HELPERS = 1
    };
    
    /**
     * Creates a new helper. 
     *
     * @param context  file and line number the helper was declared in
     * @param name     name of the helper
     * @param funcs    list of <code>Function</code> containing the helper
     *                 functions
     */
    TypeHelper(FEContext *context, string name, FunctionList *funcs, int cls)
    {
        this->context = context;
        this->name = name;
        this->funcs = funcs;
        this->cls = cls;
    }

    TypeHelper(FEContext *context, string name, FunctionList *funcs)
    {
        this->context = context;
        this->name = name;
        this->funcs = funcs;
        this->cls = NORMAL_HELPERS;
    }

    int getCls() { return cls; }

    /**
     * Returns the context of the helper in the original source code.
     *
     * @return file name and line number the structure was declared in
     */
    FEContext *getContext()
    {
        return context;
    }
    
    /**
     * Returns the name of the helper package.
     *
     * @return the name of the helper package
     */
    string getName()
    {
        return name;
    }
    
    /**
     * Returns the number of functions.
     *
     * @return the number of functions in the helper
     */
    int getNumFuncs()
    {
        return funcs->size();
    }
    
    /**
     * Returns the specific functino.
     *
     * @param n zero-based index of the function to get
     * @return  the nth function field
     */
    Function *getFunction(int n)
    {
        return (*funcs)[n];
    }

    void setFunction(int n, Function *func)
    {
        (*funcs)[n] = func;
    }
    
    // Remember, equality and such only test on the name.
//     public boolean equals(Object other)
//     {
//         if (other instanceof TypeHelper)
//             {
//                 TypeHelper that = (TypeHelper)other;
//                 return this.name.equals(that.name);
//             }
        
//         return false;
//     }
    
//     public int hashCode()
//     {
//         return name.hashCode();
//     }
    
    string toString()
    {
        return name;
    }

    void *accept(TypeVisitor *v)
    {
	return v->visitTypeHelper(this);
    }
};

typedef vector<TypeHelper*> TypeHelperList;

}

#endif
