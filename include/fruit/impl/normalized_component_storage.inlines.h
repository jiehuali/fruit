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

#ifndef FRUIT_NORMALIZED_COMPONENT_STORAGE_INLINES_H
#define FRUIT_NORMALIZED_COMPONENT_STORAGE_INLINES_H

// Redundant, but makes KDevelop happy.
#include "normalized_component_storage.h"

namespace fruit {
namespace impl {

inline BindingData::BindingData(const TypeInfo* typeInfo, create_t create)
: x1(reinterpret_cast<Unsigned>(typeInfo)), x2(reinterpret_cast<Unsigned>(create)) {
}
  
inline BindingData::BindingData(const TypeInfo* typeInfo, object_t object) 
: x1(reinterpret_cast<Unsigned>(typeInfo) | 1), x2(reinterpret_cast<Unsigned>(object)) {
}

inline bool BindingData::isCreated() const {
  return x1 & 1;
}

inline const TypeInfo* BindingData::getKey() const {
  return reinterpret_cast<const TypeInfo*>(x1 & ~Unsigned(1));
}

inline BindingData::Unsigned BindingData::getRawKey() const {
  return x1;
}

inline BindingData::create_t BindingData::getCreate() const {
  return reinterpret_cast<create_t>(x2);
}

inline BindingData::object_t BindingData::getStoredSingleton() const {
  return reinterpret_cast<object_t>(x2);
}

inline BindingData::destroy_t BindingData::create(
    InjectorStorage& storage) {
  destroy_t destroyOp;
  object_t obj;
  std::tie(obj, destroyOp) = getCreate()(storage);
  x1 ^= 1;
  x2 = reinterpret_cast<Unsigned>(obj);
  return destroyOp;
}

inline bool BindingData::operator==(const BindingData& other) const {
  return std::tie(x1, x2)
      == std::tie(other.x1, other.x2);
}

inline bool BindingData::operator<(const BindingData& other) const {
  // `destroy' is intentionally not compared.
  // If the others are equal it should also be equal. If it isn't, the two BindingData structs
  // are still equivalent because they produce the same injected object.
  return std::tie(x1, x2)
       < std::tie(other.x1, other.x2);
}

inline NormalizedComponentStorage::NormalizedComponentStorage() {
}

inline NormalizedComponentStorage::NormalizedComponentStorage(NormalizedComponentStorage&& other)
  : NormalizedComponentStorage() {
  swap(other);
}

inline NormalizedComponentStorage& NormalizedComponentStorage::operator=(NormalizedComponentStorage&& other) {
  swap(other);
  return *this;
}

inline NormalizedComponentStorage& NormalizedComponentStorage::operator=(const NormalizedComponentStorage& other) {
  NormalizedComponentStorage tmp(other);
  swap(tmp);
  return *this;
}

inline void NormalizedComponentStorage::swap(NormalizedComponentStorage& other) {
  std::swap(total_size, other.total_size);
  std::swap(typeRegistry, other.typeRegistry);
  typeRegistryForMultibindings.swap(other.typeRegistryForMultibindings);
}

} // namespace impl
} // namespace fruit


#endif // FRUIT_NORMALIZED_COMPONENT_STORAGE_INLINES_H
