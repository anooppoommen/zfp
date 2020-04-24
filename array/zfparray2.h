#ifndef ZFP_ARRAY2_H
#define ZFP_ARRAY2_H

#include <cstddef>
#include <cstring>
#include <iterator>
#include "zfparray.h"
#include "zfpcodec.h"
#include "zfp/cache2.h"
#include "zfp/store2.h"

namespace zfp {

// compressed 2D array of scalars
template < typename Scalar, class Codec = zfp::zfp_codec<Scalar, 2> >
class array2 : public array {
public:
  // types utilized by nested classes
  typedef array2 container_type;
  typedef Scalar value_type;
  typedef Codec codec_type;
  typedef typename Codec::header header;

  // forward declarations
  class const_reference;
  class const_pointer;
  class const_iterator;
  class reference;
  class pointer;
  class iterator;
  class view;

  #include "zfp/handle2.h"
  #include "zfp/reference2.h"
  #include "zfp/pointer2.h"
  #include "zfp/iterator2.h"
  #include "zfp/view2.h"

  // default constructor
  array2() :
    array(2, Codec::type),
    cache(store)
  {}

  // constructor of nx * ny array using rate bits per value, at least
  // cache_size bytes of cache, and optionally initialized from flat array p
  array2(uint nx, uint ny, double rate, const Scalar* p = 0, size_t cache_size = 0) :
    array(2, Codec::type),
    store(nx, ny, rate),
    cache(store, cache_size)
  {
    this->nx = nx;
    this->ny = ny;
    if (p)
      set(p);
  }

  // constructor, from previously-serialized compressed array
  array2(const zfp::array::header& header, const void* buffer = 0, size_t buffer_size_bytes = 0) :
    array(2, Codec::type, header),
    store(header.size_x(), header.size_y(), header.rate()),
    cache(store)
  {
    if (buffer) {
      if (buffer_size_bytes && buffer_size_bytes < store.compressed_size())
        throw zfp::exception("buffer size is smaller than required");
      std::memcpy(store.compressed_data(), buffer, store.compressed_size());
    }
  }

  // copy constructor--performs a deep copy
  array2(const array2& a) :
    cache(store)
  {
    deep_copy(a);
  }

  // construction from view--perform deep copy of (sub)array
  template <class View>
  array2(const View& v) :
    array(2, Codec::type),
    store(v.size_x(), v.size_y(), v.rate()),
    cache(store)
  {
    this->nx = v.size_x();
    this->ny = v.size_y();
    // initialize array in its preferred order
    for (iterator it = begin(); it != end(); ++it)
      *it = v(it.i(), it.j());
  }

  // virtual destructor
  virtual ~array2() {}

  // assignment operator--performs a deep copy
  array2& operator=(const array2& a)
  {
    if (this != &a)
      deep_copy(a);
    return *this;
  }

  // total number of elements in array
  size_t size() const { return size_t(nx) * size_t(ny); }

  // array dimensions
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }

  // resize the array (all previously stored data will be lost)
  void resize(uint nx, uint ny, bool clear = true)
  {
    this->nx = nx;
    this->ny = ny;
    store.resize(nx, ny, clear);
    cache.clear();
  }

  // rate in bits per value
  double rate() const { return cache.rate(); }

  // set rate in bits per value
  double set_rate(double rate) { return cache.set_rate(rate); }

  // number of bytes of compressed data
  size_t compressed_size() const { return store.compressed_size(); }

  // pointer to compressed data for read or write access
  void* compressed_data() const
  {
    cache.flush();
    return store.compressed_data();
  }

  // cache size in number of bytes
  size_t cache_size() const { return cache.size(); }

  // set minimum cache size in bytes (array dimensions must be known)
  void set_cache_size(size_t bytes)
  {
    cache.flush();
    cache.resize(bytes);
  }

  // empty cache without compressing modified cached blocks
  void clear_cache() const { cache.clear(); }

  // flush cache by compressing all modified cached blocks
  void flush_cache() const { cache.flush(); }

  // decompress array and store at p
  void get(Scalar* p) const
  {
    const uint bx = (uint)store.block_size_x();
    const uint by = (uint)store.block_size_y();
    uint block_index = 0;
    for (uint j = 0; j < by; j++, p += 4 * (nx - bx))
      for (uint i = 0; i < bx; i++, p += 4)
        cache.get_block(block_index++, p, 1, nx);
  }

  // initialize array by copying and compressing data stored at p
  void set(const Scalar* p)
  {
    const uint bx = (uint)store.block_size_x();
    const uint by = (uint)store.block_size_y();
    uint block_index = 0;
    for (uint j = 0; j < by; j++, p += 4 * (nx - bx))
      for (uint i = 0; i < bx; i++, p += 4)
        cache.put_block(block_index++, p, 1, nx);
  }

  // (i, j) accessors
  Scalar operator()(uint i, uint j) const { return get(i, j); }
  reference operator()(uint i, uint j) { return reference(this, i, j); }

  // flat index accessors
  Scalar operator[](uint index) const
  {
    uint i, j;
    ij(i, j, index);
    return get(i, j);
  }
  reference operator[](uint index)
  {
    uint i, j;
    ij(i, j, index);
    return reference(this, i, j);
  }

  // sequential iterators
  const_iterator cbegin() const { return const_iterator(this, 0, 0); }
  const_iterator cend() const { return const_iterator(this, 0, ny); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  iterator begin() { return iterator(this, 0, 0); }
  iterator end() { return iterator(this, 0, ny); }

protected:
  // perform a deep copy
  void deep_copy(const array2& a)
  {
    // copy base class members
    array::deep_copy(a);
    // copy persistent storage
    store.deep_copy(a.store);
    // copy cached data
    cache.deep_copy(a.cache);
  }

  // inspector
  Scalar get(uint i, uint j) const { return cache.get(i, j); }

  // mutators (called from proxy reference)
  void set(uint i, uint j, Scalar val) { cache.set(i, j, val); }
  void add(uint i, uint j, Scalar val) { cache.ref(i, j) += val; }
  void sub(uint i, uint j, Scalar val) { cache.ref(i, j) -= val; }
  void mul(uint i, uint j, Scalar val) { cache.ref(i, j) *= val; }
  void div(uint i, uint j, Scalar val) { cache.ref(i, j) /= val; }

  // convert flat index to (i, j)
  void ij(uint& i, uint& j, uint index) const
  {
    i = index % nx; index /= nx;
    j = index;
  }

  BlockStore2<Scalar, Codec> store; // persistent storage of compressed blocks
  BlockCache2<Scalar, Codec> cache; // cache of decompressed blocks
};

typedef array2<float> array2f;
typedef array2<double> array2d;

}

#endif
