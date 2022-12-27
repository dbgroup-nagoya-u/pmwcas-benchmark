/*
 * Copyright 2022 Database Group, Nagoya University
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

// C++ standard libraries
#include <atomic>
#include <memory>

template <class T>
class ElementHolder
{
 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  ElementHolder(  //
      const size_t pos,
      const std::shared_ptr<std::atomic_bool[]> &reserved_arr,
      T element)
      : pos_{pos}, reserved_arr_{reserved_arr}, element_{std::move(element)}
  {
  }

  ElementHolder(const ElementHolder &) = delete;
  ElementHolder(ElementHolder &&) = delete;
  auto operator=(const ElementHolder &) -> ElementHolder & = delete;
  auto operator=(ElementHolder &&) -> ElementHolder & = delete;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~ElementHolder() { reserved_arr_[pos_].store(false, std::memory_order_relaxed); }

  /*####################################################################################
   * Public getters
   *##################################################################################*/

  constexpr auto
  Get()  //
      -> T &
  {
    return element_;
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// the position of a descriptor in a pool.
  size_t pos_{0};

  /// an actual reservation flag for this object.
  std::shared_ptr<std::atomic_bool[]> reserved_arr_{nullptr};

  /// the hold element.
  T element_{};
};
