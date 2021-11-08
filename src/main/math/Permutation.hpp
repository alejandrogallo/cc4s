/* Copyright 2021 cc4s.org
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

#ifndef PERMUTATION_DEFINED
#define PERMUTATION_DEFINED

#include <iostream>
#include <initializer_list>

namespace cc4s {
  template <int N>
  class Map {
  public:
    Map() {
    }
    Map(std::initializer_list<int> images_) {
      std::copy(images_.begin(), images_.end(), images);
    }
    Map(const Map &p) {
      for (int i(0); i < N; ++i) images[i] = p.images[i];
    }

    int operator ()(int i) const {
      return images[i];
    }
    int &operator ()(int i) {
      return images[i];
    }

    bool operator ==(const Map &m) const {
      for (int i(0); i < N; ++i) if (images[i] != m.images[i]) return false;
      return true;
    }

    int images[N];
  };

  template <int N>
  inline std::ostream &operator <<(std::ostream &stream, const Map<N> &f) {
    stream << "(";
    for (int i(0); i < N-1; ++i) stream << f(i) << ",";
    stream << f(N-1) << ")";
    return stream;
  }


  template <int N>
  class Permutation;

  template <int N>
  Permutation<N> operator *(const Permutation<N> &, const Permutation<N> &);
  template <int N>
  Permutation<N> operator /(const Permutation<N> &, const Permutation<N> &);
  template <int N>
  Permutation<N> operator /(const int, const Permutation<N> &);

  template <int N>
  class Permutation: public Map<N> {
  public:
    Permutation(const int64_t p) {
      Permutation<N-1> subPermutation(p/N);
      int i;
      for (i = 0; i < p%N; ++i) {
        this->images[i] = subPermutation.images[i] + 1;
      }
      this->images[p%N] = 0;
      for (++i; i < N; ++i) {
        this->images[i] = subPermutation.images[i-1] + 1;
      }
    }

    int invariantElementsCount() const {
      int count(0);
      for (int i(0); i < N; ++i) count += this->images[i] == i;
      return count;
    }

    static constexpr int64_t ORDER = N * Permutation<N-1>::ORDER;
  private:
    Permutation() {
    }
    friend Permutation operator *<N>(const Permutation &, const Permutation &);
    friend Permutation operator /<N>(const Permutation &, const Permutation &);
    friend Permutation operator /<N>(const int, const Permutation &);
  };

  template <>
  class Permutation<1>: public Map<1> {
  public:
    Permutation(const int64_t p) {
      images[0] = 0;
    }
    static constexpr int64_t ORDER = 1;
  };

  // sigma after tau
  template <int N>
  inline Permutation<N> operator *(
    const Permutation<N> &sigma, const Permutation<N> &tau
  ) {
    Permutation<N> pi;
    for (int i(0); i < N; ++i) pi(i) = sigma(tau(i));
    return pi;
  }

  // general map f after tau
  template <int N>
  inline Map<N> operator *(const Map<N> &f, const Permutation<N> &tau) {
    Map<N> g;
    for (int i(0); i < N; ++i) g(i) = f(tau(i));
    return g;
  }

  // inverse of a permutation tau
  template <int N>
  inline Permutation<N> operator /(const int , const Permutation<N> &tau) {
    Permutation<N> tauInverse;
    for (int i(0); i < N; ++i) tauInverse(tau(i)) = i;
    return tauInverse;
  }

  // sigma after inverse of tau
  template <int N>
  inline Permutation<N> operator /(
    const Permutation<N> &sigma, const Permutation<N> &tau
  ) {
    Permutation<N> pi;
    for (int i(0); i < N; ++i) pi(tau(i)) = sigma(i);
    return pi;
  }
}

#endif

