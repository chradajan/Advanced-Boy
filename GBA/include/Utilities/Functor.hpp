#pragma once

/// @brief Primary template of BaseFunctor pure virtual base class.
template <typename>
class BaseFunctor;

/// @brief Primary template of MemberFunctor.
template<typename>
class MemberFunctor;

/// @brief Pure virtual base class of all functors.
/// @tparam ReturnT Return type of functor call operation.
/// @tparam ...TArgs Args to pass into functor call operation.
template <typename ReturnT, typename... TArgs>
class BaseFunctor<ReturnT(TArgs...)>
{
public:
    virtual ~BaseFunctor() = default;

    virtual ReturnT operator()(TArgs... params) = 0;

protected:
    BaseFunctor() = default;
    BaseFunctor(BaseFunctor const&) = default;
    BaseFunctor(BaseFunctor&&) = default;
    BaseFunctor& operator=(BaseFunctor const&) = default;
    BaseFunctor& operator=(BaseFunctor&&) = default;
};

/// @brief Functor to create a callback into a class member function.
/// @tparam ReturnT Return type of callback function.
/// @tparam ObjT Class type that contains callback function.
/// @tparam ...TArgs Args to pass into callback.
template<typename ReturnT, typename ObjT, typename... TArgs>
class MemberFunctor<ReturnT (ObjT::*)(TArgs...)> : public BaseFunctor<ReturnT(TArgs...)>
{
    using PtrToMemberFunctionType = ReturnT (ObjT::*)(TArgs...);
    using ThisType = MemberFunctor<ReturnT (ObjT::*)(TArgs...)>;

public:
    MemberFunctor(PtrToMemberFunctionType memberFunctionPtr, ObjT& objectRef);
    MemberFunctor() = delete;
    virtual ~MemberFunctor() = default;

    ReturnT operator()(TArgs... params) override;

private:
    PtrToMemberFunctionType memberFunctionPtr_;
    ObjT* objectPtr_;
};

#include <Utilities/Functor.tpp>
