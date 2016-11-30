/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef INDEX_COUNTS_DEFINED
#define INDEX_COUNTS_DEFINED

namespace cc4s {
  class IndexCounts {
  public:
    IndexCounts() {
      for (int i(0); i < INDICES_COUNT; ++i) counts[i] = 0;
    }

    void add(const std::string &indices, const int step = +1) {
      for (unsigned int d(0); d < indices.length(); ++d) {
        counts[static_cast<uint8_t>(indices[d])] += step;
      }
    }

    void add(const char *indices, const int step = +1) {
      for (int d(0); indices[d] != 0; ++d) {
        counts[static_cast<uint8_t>(indices[d])] += step;
      }
    }

    int operator [](char index) const {
      return counts[static_cast<uint8_t>(index)];
    }

    static constexpr int INDICES_COUNT = UINT8_MAX;
    int counts[INDICES_COUNT];
  };
}

#endif
