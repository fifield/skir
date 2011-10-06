#ifndef _TYPEVISITOR_HPP_
#define _TYPEVISITOR_HPP_

namespace streamit {

class Type;
class TypeArray;
class TypeHelper;
class TypePrimitive;
class TypeStructRef;
class TypeStruct;

class TypeVisitor
{
public:
    virtual void *visitType(Type *t) { return NULL; }
    virtual void *visitTypeArray(TypeArray *t) { return NULL; }
    virtual void *visitTypeHelper(TypeHelper *t) { return NULL; }
    virtual void *visitTypePrimitive(TypePrimitive *t) { return NULL; }
    virtual void *visitTypeStructRef(TypeStructRef *t) { return NULL; }
    virtual void *visitTypeStruct(TypeStruct *t) { return NULL; }
};

}
#endif
