#ifndef ZFP_CACHE2_H
#define ZFP_CACHE2_H

#include "cache.h"
#include "store2.h"

namespace zfp {

template <typename Scalar, class Codec>
class BlockCache2 {
public:
  // constructor of cache of given size
  BlockCache2(BlockStore2<Scalar, Codec>& store, size_t bytes = 0) :
    cache((uint)((bytes + sizeof(CacheLine) - 1) / sizeof(CacheLine))),
    store(store),
    codec(0)
  {
    alloc();
  }

  // destructor
  ~BlockCache2() { free(); }

  // cache size in number of bytes
  size_t size() const { return cache.size() * sizeof(CacheLine); }

  // set minimum cache size in bytes (inferred from blocks if zero)
  void resize(size_t bytes)
  {
    flush();
    cache.resize(lines(bytes, store.blocks()));
  }

  // rate in bits per value
  double rate() const { return store.rate(); }

  // set rate in bits per value
  double set_rate(double rate)
  {
    cache.clear();
    free();
    rate = store.set_rate(rate);
    alloc();
    return rate;
  }

  // empty cache without compressing modified cached blocks
  void clear() const { cache.clear(); }

  // flush cache by compressing all modified cached blocks
  void flush() const
  {
    for (typename zfp::Cache<CacheLine>::const_iterator p = cache.first(); p; p++) {
      if (p->tag.dirty()) {
        uint block_index = p->tag.index() - 1;
        store.encode(codec, block_index, p->line->data());
      }
      cache.flush(p->line);
    }
  }

  // perform a deep copy
  void deep_copy(const BlockCache2& c)
  {
    free();
    cache = c.cache;
    alloc();
  }

  // inspector
  Scalar get(uint i, uint j) const
  {
    const CacheLine* p = line(i, j, false);
    return (*p)(i, j);
  }

  // mutator
  void set(uint i, uint j, Scalar val)
  {
    CacheLine* p = line(i, j, true);
    (*p)(i, j) = val;
  }

  // reference to cached element
  Scalar& ref(uint i, uint j)
  {
    CacheLine* p = line(i, j, true);
    return (*p)(i, j);
  }

  // copy block from cache, if cached, or fetch from persistent storage without caching
  void get_block(uint block_index, Scalar* p, ptrdiff_t sx, ptrdiff_t sy) const
  {
    const CacheLine* line = cache.lookup(block_index + 1, false);
    if (line)
      line->get(p, sx, sy, store.block_shape(block_index));
    else
      store.decode(codec, block_index, p, sx, sy);
  }

  // copy block to cache, if cached, or store to persistent storage without caching
  void put_block(uint block_index, const Scalar* p, ptrdiff_t sx, ptrdiff_t sy)
  {
    CacheLine* line = cache.lookup(block_index + 1, true);
    if (line)
      line->put(p, sx, sy, store.block_shape(block_index));
    else
      store.encode(codec, block_index, p, sx, sy);
  }

protected:
  // allocate codec
  void alloc()
  {
    codec = new Codec(store.compressed_data(), store.compressed_size());
    codec->set_rate(store.rate());
  }

  // free allocated data
  void free()
  {
    if (codec) {
      delete codec;
      codec = 0;
    }
  }

  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    // accessors
    Scalar operator()(uint i, uint j) const { return a[index(i, j)]; }
    Scalar& operator()(uint i, uint j) { return a[index(i, j)]; }

    // pointer to decompressed block data
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }

    // copy whole block from cache line
    void get(Scalar* p, ptrdiff_t sx, ptrdiff_t sy) const
    {
      const Scalar* q = a;
      for (uint y = 0; y < 4; y++, p += sy - 4 * sx)
        for (uint x = 0; x < 4; x++, p += sx, q++)
          *p = *q;
    }

    // copy partial block from cache line
    void get(Scalar* p, ptrdiff_t sx, ptrdiff_t sy, uint shape) const
    {
      if (!shape)
        get(p, sx, sy);
      else {
        // determine block dimensions
        uint nx = 4 - (shape & 3u); shape >>= 2;
        uint ny = 4 - (shape & 3u); shape >>= 2;
        const Scalar* q = a;
        for (uint y = 0; y < ny; y++, p += sy - (ptrdiff_t)nx * sx, q += 4 - nx)
          for (uint x = 0; x < nx; x++, p += sx, q++)
            *p = *q;
      }
    }

    // copy whole block to cache line
    void put(const Scalar* p, ptrdiff_t sx, ptrdiff_t sy)
    {
      Scalar* q = a;
      for (uint y = 0; y < 4; y++, p += sy - 4 * sx)
        for (uint x = 0; x < 4; x++, p += sx, q++)
          *q = *p;
    }

    // copy partial block to cache line
    void put(const Scalar* p, ptrdiff_t sx, ptrdiff_t sy, uint shape)
    {
      if (!shape)
        put(p, sx, sy);
      else {
        // determine block dimensions
        uint nx = 4 - (shape & 3u); shape >>= 2;
        uint ny = 4 - (shape & 3u); shape >>= 2;
        Scalar* q = a;
        for (uint y = 0; y < ny; y++, p += sy - (ptrdiff_t)nx * sx, q += 4 - nx)
          for (uint x = 0; x < nx; x++, p += sx, q++)
            *q = *p;
      }
    }

  protected:
    static uint index(uint i, uint j) { return (i & 3u) + 4 * (j & 3u); }
    Scalar a[4 * 4];
  };

  // return cache line for (i, j); may require write-back and fetch
  CacheLine* line(uint i, uint j, bool write) const
  {
    CacheLine* p = 0;
    uint block_index = store.block_index(i, j);
    typename zfp::Cache<CacheLine>::Tag tag = cache.access(p, block_index + 1, write);
    uint stored_block_index = tag.index() - 1;
    if (stored_block_index != block_index) {
      // write back occupied cache line if it is dirty
      if (tag.dirty())
        store.encode(codec, stored_block_index, p->data());
      // fetch cache line
      store.decode(codec, block_index, p->data());
    }
    return p;
  }

  // default number of cache lines for array with given number of blocks
  static uint lines(size_t blocks)
  {
    // compute m = O(sqrt(n))
    size_t m;
    for (m = 1; m * m < blocks; m *= 2);
    return static_cast<uint>(m);
  }

  // number of cache lines corresponding to size (or suggested size if zero)
  static uint lines(size_t bytes, size_t blocks)
  {
    uint n = bytes ? uint((bytes + sizeof(CacheLine) - 1) / sizeof(CacheLine)) : lines(blocks);
    return std::max(n, 1u);
  }

  mutable Cache<CacheLine> cache;    // cache of decompressed blocks
  BlockStore2<Scalar, Codec>& store; // store backed by cache
  Codec* codec;                      // compression codec
};

}

#endif
