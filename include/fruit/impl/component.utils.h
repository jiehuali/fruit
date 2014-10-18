/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRUIT_COMPONENT_UTILS_H
#define FRUIT_COMPONENT_UTILS_H

#include "../fruit_forward_decls.h"
#include "fruit_assert.h"

#include <memory>

#include "metaprogramming/logical_operations.h"
#include "metaprogramming/metaprogramming.h"
#include "metaprogramming/list.h"
#include "metaprogramming/set.h"

namespace fruit {

namespace impl {

// Represents a dependency from the binding of type T to the list of types Rs.
// Rs must be a set.
template <typename T, typename Rs>
struct ConsDep {
  using Type = T;
  using Requirements = Rs;
};

template <typename I, typename C>
struct ConsBinding {
  using Interface = I;
  using Impl = C;
};

// General case, if none of the following apply.
// When adding a specialization here, make sure that the ComponentStorage
// can actually get<> the specified type when the class was registered.
template <typename T>
struct GetClassForTypeHelper {using type = T;};

template <typename T>
struct GetClassForTypeHelper<const T> {using type = T;};

template <typename T>
struct GetClassForTypeHelper<T*> {using type = T;};

template <typename T>
struct GetClassForTypeHelper<T&> {using type = T;};

template <typename T>
struct GetClassForTypeHelper<const T*> {using type = T;};

template <typename T>
struct GetClassForTypeHelper<const T&> {using type = T;};

template <typename T>
struct GetClassForTypeHelper<std::shared_ptr<T>> {using type = T;};

template <typename T>
using GetClassForType = typename GetClassForTypeHelper<T>::type;

template <typename L>
struct GetClassForTypeListHelper {}; // Not used.

template <typename... Ts>
struct GetClassForTypeListHelper<List<Ts...>> {
  using type = List<GetClassForType<Ts>...>;
};

template <typename L>
using GetClassForTypeList = typename GetClassForTypeListHelper<L>::type;

template <typename Signature>
struct IsValidSignature : std::false_type {};

template <typename T, typename... Args>
struct IsValidSignature<T(Args...)> : public static_and<!is_list<T>::value, !is_list<Args>::value...> {};

// Non-assisted case
template <typename T>
struct ExtractRequirementFromAssistedParamHelper {
  using type = GetClassForType<T>;
};

template <typename T>
struct ExtractRequirementFromAssistedParamHelper<Assisted<T>> {
  using type = None;
};

template <typename L>
struct ExtractRequirementsFromAssistedParamsHelper {};

template <typename... Ts>
struct ExtractRequirementsFromAssistedParamsHelper<List<Ts...>> {
  using type = List<typename ExtractRequirementFromAssistedParamHelper<Ts>::type...>;
};

// Takes a list of types, considers only the assisted ones, transforms them to classes with
// GetClassForType and returns the resulting list. Note: the output list might contain some None elements.
template <typename L>
using ExtractRequirementsFromAssistedParams = typename ExtractRequirementsFromAssistedParamsHelper<L>::type;

template <typename L>
struct RemoveNonAssistedHelper {};

template <>
struct RemoveNonAssistedHelper<List<>> {
  using type = List<>;
};

// Non-assisted case
template <typename T, typename... Ts>
struct RemoveNonAssistedHelper<List<T, Ts...>> {
  using type = typename RemoveNonAssistedHelper<List<Ts...>>::type;
};

// Assisted case
template <typename T, typename... Ts>
struct RemoveNonAssistedHelper<List<Assisted<T>, Ts...>> {
  using type = add_to_list<T, typename RemoveNonAssistedHelper<List<Ts...>>::type>;
};

template <typename L>
using RemoveNonAssisted = typename RemoveNonAssistedHelper<L>::type;

template <typename L>
struct RemoveAssistedHelper {};

template <>
struct RemoveAssistedHelper<List<>> {
  using type = List<>;
};

// Non-assisted case
template <typename T, typename... Ts>
struct RemoveAssistedHelper<List<T, Ts...>> {
  using type = add_to_list<T, typename RemoveAssistedHelper<List<Ts...>>::type>;
};

// Assisted case
template <typename T, typename... Ts>
struct RemoveAssistedHelper<List<Assisted<T>, Ts...>> {
  using type = typename RemoveAssistedHelper<List<Ts...>>::type;
};

template <typename L>
using RemoveAssisted = typename RemoveAssistedHelper<L>::type;

template <typename T>
struct UnlabelAssistedSingleType {
  using type = T;
};

template <typename T>
struct UnlabelAssistedSingleType<Assisted<T>> {
  using type = T;
};

template <typename L>
struct UnlabelAssistedHelper {};

template <typename... Ts>
struct UnlabelAssistedHelper<List<Ts...>> {
  using type = List<typename UnlabelAssistedSingleType<Ts>::type...>;
};

template <typename L>
using UnlabelAssisted = typename UnlabelAssistedHelper<L>::type;

template <typename AnnotatedSignature>
using RequiredArgsForAssistedFactory = UnlabelAssisted<SignatureArgs<AnnotatedSignature>>;

template <typename AnnotatedSignature>
using RequiredSignatureForAssistedFactory = ConstructSignature<SignatureType<AnnotatedSignature>,
                                                               RequiredArgsForAssistedFactory<AnnotatedSignature>>;

template <typename AnnotatedSignature>
using InjectedFunctionArgsForAssistedFactory = RemoveNonAssisted<SignatureArgs<AnnotatedSignature>>;

template <typename AnnotatedSignature>
using InjectedSignatureForAssistedFactory = ConstructSignature<SignatureType<AnnotatedSignature>,
                                                               InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>;

template <int index, typename L>
class NumAssistedBefore {}; // Not used. Instantiated only if index is out of bounds.

template <typename T, typename... Ts>
class NumAssistedBefore<0, List<T, Ts...>> : public std::integral_constant<int, 0> {};

// This is needed because the previous is not more specialized than the specialization with assisted T.
template <typename T, typename... Ts>
class NumAssistedBefore<0, List<Assisted<T>, Ts...>> : public std::integral_constant<int, 0> {};

// Non-assisted T, index!=0.
template <int index, typename T, typename... Ts>
class NumAssistedBefore<index, List<T, Ts...>> : public NumAssistedBefore<index-1, List<Ts...>> {};

// Assisted T, index!=0.
template <int index, typename T, typename... Ts>
class NumAssistedBefore<index, List<Assisted<T>, Ts...>> : public std::integral_constant<int, 1 + NumAssistedBefore<index-1, List<Ts...>>::value> {};

// Exposes a bool `value' (whether C is injectable with annotation)
template <typename C>
struct HasInjectAnnotation {
    typedef char yes[1];
    typedef char no[2];

    template <typename C1>
    static yes& test(typename C1::Inject*);

    template <typename>
    static no& test(...);
    
    static const bool value = sizeof(test<C>(0)) == sizeof(yes);
};

template <typename C>
struct GetInjectAnnotation {
    using S = typename C::Inject;
    FruitDelegateCheck(InjectTypedefNotASignature<C, S>);
    using A = SignatureArgs<S>;
    FruitDelegateCheck(InjectTypedefForWrongClass<C, SignatureType<S>>);
    FruitDelegateCheck(NoConstructorMatchingInjectSignature<C, S>);
    static constexpr bool ok = true
        && IsValidSignature<S>::value
        && std::is_same<C, SignatureType<S>>::value
        && is_constructible_with_list<C, UnlabelAssisted<A>>::value;
    // Don't even provide them if the asserts above failed. Otherwise the compiler goes ahead and may go into a long loop,
    // e.g. with an Inject=int(C) in a class C.
    using Signature = typename std::enable_if<ok, S>::type;
    using Args = typename std::enable_if<ok, A>::type;
};

template <typename C, typename Dep>
using RemoveRequirementFromDep = ConsDep<typename Dep::Type, remove_from_list<C, typename Dep::Requirements>>;

template <typename C, typename Deps>
struct RemoveRequirementFromDepsHelper {}; // Not used

template <typename C, typename... Deps>
struct RemoveRequirementFromDepsHelper<C, List<Deps...>> {
  using type = List<RemoveRequirementFromDep<C, Deps>...>;
};

template <typename C, typename Deps>
using RemoveRequirementFromDeps = typename RemoveRequirementFromDepsHelper<C, Deps>::type;

template <typename P, typename Rs>
using ConstructDep = ConsDep<P, list_to_set<Rs>>;

template <typename Rs, typename... P>
using ConstructDeps = List<ConstructDep<P, Rs>...>;

template <typename Dep>
struct HasSelfLoop : is_in_list<typename Dep::Type, typename Dep::Requirements> {
};

template <typename Requirements, typename D1>
using CanonicalizeDepRequirementsWithDep = replace_with_set<Requirements, typename D1::Type, typename D1::Requirements>;

template <typename D, typename D1>
using CanonicalizeDepWithDep = ConsDep<typename D::Type, CanonicalizeDepRequirementsWithDep<typename D::Requirements, D1>>;

template <typename Deps, typename Dep>
struct CanonicalizeDepsWithDep {}; // Not used.

template <typename... Deps, typename Dep>
struct CanonicalizeDepsWithDep<List<Deps...>, Dep> {
  using type = List<CanonicalizeDepWithDep<Deps, Dep>...>;
};

template <typename Requirements, typename Deps>
struct CanonicalizeDepRequirementsWithDeps {}; // Not used.

template <typename Requirements>
struct CanonicalizeDepRequirementsWithDeps<Requirements, List<>> {
  using type = Requirements;
};

template <typename Requirements, typename D1, typename... Ds>
struct CanonicalizeDepRequirementsWithDeps<Requirements, List<D1, Ds...>> {
  using recursion_result = typename CanonicalizeDepRequirementsWithDeps<Requirements, List<Ds...>>::type;
  using type = CanonicalizeDepRequirementsWithDep<recursion_result, D1>;
};

template <typename Requirements, typename OtherDeps>
struct CanonicalizeDepRequirementsWithDepsHelper {}; // Not used.

template <typename Requirements, typename... OtherDeps>
struct CanonicalizeDepRequirementsWithDepsHelper<Requirements, List<OtherDeps...>> {
  using type = List<typename std::conditional<is_in_list<typename OtherDeps::Type, Requirements>::value,
                                              typename OtherDeps::Requirements,
                                              List<>>::type...>;
};

template <typename Dep, typename Deps, typename DepsTypes>
using CanonicalizeDepWithDeps = ConsDep<typename Dep::Type,
  set_union<
    list_of_sets_union<
      typename CanonicalizeDepRequirementsWithDepsHelper<typename Dep::Requirements, Deps>::type
    >,
    set_difference<
      typename Dep::Requirements,
      DepsTypes
    >
  >
>;

template <typename Dep, typename Deps, typename DepsTypes>
struct AddDepHelper {
  using CanonicalizedDep = CanonicalizeDepWithDeps<Dep, Deps, DepsTypes>;
  FruitDelegateCheck(CheckHasNoSelfLoop<!HasSelfLoop<CanonicalizedDep>::value, typename CanonicalizedDep::Type>);
  // At this point CanonicalizedDep doesn't have as arguments any types appearing as heads in Deps,
  // but the head of CanonicalizedDep might appear as argument of some Deps.
  // A single replacement step is sufficient.
  using type = add_to_list<CanonicalizedDep, typename CanonicalizeDepsWithDep<Deps, CanonicalizedDep>::type>;
};

template <typename Dep, typename Deps, typename DepsTypes>
using AddDep = typename AddDepHelper<Dep, Deps, DepsTypes>::type;

template <typename Deps, typename OtherDeps, typename OtherDepsTypes>
struct AddDepsHelper {};

template <typename OtherDepsList, typename OtherDepsTypes>
struct AddDepsHelper<List<>, OtherDepsList, OtherDepsTypes> {
  using type = OtherDepsList;
};

template <typename Dep, typename... Deps, typename OtherDepList, typename OtherDepsTypes>
struct AddDepsHelper<List<Dep, Deps...>, OtherDepList, OtherDepsTypes> {
  using step = AddDep<Dep, OtherDepList, OtherDepsTypes>;
  using type = typename AddDepsHelper<List<Deps...>, step, add_to_list<typename Dep::Type, OtherDepsTypes>>::type;
};

template <typename Deps, typename OtherDeps, typename OtherDepsTypes>
using AddDeps = typename AddDepsHelper<Deps, OtherDeps, OtherDepsTypes>::type;

#ifdef FRUIT_EXTRA_DEBUG

template <typename D, typename Deps>
struct CheckDepEntailed {
  FruitStaticAssert(false && sizeof(D), "bug! should never instantiate this.");
};

template <typename D>
struct CheckDepEntailed<D, List<>> {
  static_assert(false && sizeof(D), "The dep D has no match in Deps");
};

// DType is not D1Type, not the dep that we're looking for.
template <typename DType, typename... DArgs, typename D1Type, typename... D1Args, typename... Ds>
struct CheckDepEntailed<ConsDep<DType, List<DArgs...>>, List<ConsDep<D1Type, List<D1Args...>>, Ds...>> 
: public CheckDepEntailed<ConsDep<DType, List<DArgs...>>, List<Ds...>> {};

// Found the dep that we're looking for, check that the args are a subset.
template <typename DType, typename... DArgs, typename... D1Args, typename... Ds>
struct CheckDepEntailed<ConsDep<DType, List<DArgs...>>, List<ConsDep<DType, List<D1Args...>>, Ds...>> {
  static_assert(is_empty_list<set_difference<List<D1Args...>, List<DArgs...>>>::value, "Error, the args in the new dep are not a superset of the ones in the old one");
};

// General case: DepsSubset is empty.
template <typename DepsSubset, typename Deps>
struct CheckDepsSubset {
  static_assert(is_empty_list<DepsSubset>::value, "");
};

template <typename D1, typename... D, typename Deps>
struct CheckDepsSubset<List<D1, D...>, Deps> : CheckDepsSubset<List<D...>, Deps> {
  FruitDelegateCheck(CheckDepEntailed<D1, Deps>);
};

// General case: DepsSubset is empty.
template <typename Comp, typename EntailedComp>
struct CheckComponentEntails {
  using AdditionalProvidedTypes = set_difference<typename EntailedComp::Ps, typename Comp::Ps>;
  FruitDelegateCheck(CheckNoAdditionalProvidedTypes<AdditionalProvidedTypes>);
  using AdditionalBindings = set_difference<typename EntailedComp::Bindings, typename Comp::Bindings>;
  FruitDelegateCheck(CheckNoAdditionalBindings<AdditionalBindings>);
  using NoLongerRequiredTypes = set_difference<typename Comp::Rs, typename EntailedComp::Rs>;
  FruitDelegateCheck(CheckNoTypesNoLongerRequired<NoLongerRequiredTypes>);
  FruitDelegateCheck(CheckDepsSubset<typename EntailedComp::Deps, typename Comp::Deps>);
};

#endif // FRUIT_EXTRA_DEBUG

template <typename... Ts>
struct ExpandProvidersInParamsHelper {};

template <>
struct ExpandProvidersInParamsHelper<> {
  using type = List<>;
};

// Non-empty list, T is not of the form Provider<Ts...>
template <typename T, typename... OtherTs>
struct ExpandProvidersInParamsHelper<T, OtherTs...> {
  using recursion_result = typename ExpandProvidersInParamsHelper<OtherTs...>::type;
  using type = add_to_set<T, recursion_result>;
};

// Non-empty list, type of the form Provider<Ts...>
template <typename... Ts, typename... OtherTs>
struct ExpandProvidersInParamsHelper<fruit::Provider<Ts...>, OtherTs...> {
  using recursion_result = typename ExpandProvidersInParamsHelper<OtherTs...>::type;
  using type = add_to_set_multiple<recursion_result, Ts...>;
};

template <typename L>
struct ExpandProvidersInParamsImpl {};

// Non-empty list, T is not of the form Provider<Ts...>
template <typename... Ts>
struct ExpandProvidersInParamsImpl<List<Ts...>> {
  using type = typename ExpandProvidersInParamsHelper<Ts...>::type;
};

template <typename L>
using ExpandProvidersInParams = typename ExpandProvidersInParamsImpl<L>::type;

template <typename I, typename Bindings>
struct HasBinding {};

template <typename I, typename... Bindings>
struct HasBinding<I, List<Bindings...>> {
  static constexpr bool value = static_or<std::is_same<I, typename Bindings::Interface>::value...>::value;
};

template <typename I, typename... Bindings>
struct GetBindingHelper {};

template <typename I, typename C, typename... Bindings>
struct GetBindingHelper<I, ConsBinding<I, C>, Bindings...> {
  using type = C;
};

template <typename I, typename OtherBinding, typename... Bindings>
struct GetBindingHelper<I, OtherBinding, Bindings...> {
  using type = typename GetBindingHelper<I, Bindings...>::type;
};

template <typename I, typename Bindings>
struct GetBindingImpl {};

template <typename I, typename... Bindings>
struct GetBindingImpl<I, List<Bindings...>> {
  using type = typename GetBindingHelper<I, Bindings...>::type;
};

template <typename I, typename Bindings>
using GetBinding = typename GetBindingImpl<I, Bindings>::type;

template <typename T>
static inline void standardDeleter(void* p) {
  T* t = reinterpret_cast<T*>(p);
  delete t;
}

} // namespace impl
} // namespace fruit


#endif // FRUIT_COMPONENT_UTILS_H
