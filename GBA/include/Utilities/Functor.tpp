#pragma once

#include <Utilities/Functor.hpp>
#include <utility>

template<typename ReturnT, typename ObjT, typename... TArgs>
MemberFunctor<ReturnT (ObjT::*)(TArgs...)>::MemberFunctor(PtrToMemberFunctionType memberFunctionPtr, ObjT& object) :
    memberFunctionPtr_(memberFunctionPtr),
    objectPtr_(&object)
{
}

template<typename ReturnT, typename ObjT, typename... TArgs>
ReturnT MemberFunctor<ReturnT (ObjT::*)(TArgs...)>::operator()(TArgs... params)
{
    return (objectPtr_->*memberFunctionPtr_)(std::forward<TArgs>(params)...);
}
