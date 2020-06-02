///////////////////////////////////////////////////////////////////////////////
// hashes.hpp
//
// Implementations of four dictionary data structures: naive, chained hash
// table, linear probing hash table, and cuckoo hash table.
//
// Students: Your task is to replace all the TODO comments with working code.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <list>
#include <math.h>

namespace hashes {

  const uint32_t LARGE_PRIME = 2147483647; // largest prime less than 2^31

  class key_exception { };

  // One entry in a dictionary.
  template <typename T>
  class entry {
  public:

    entry(uint32_t key, T&& value) noexcept
    : key_(key), value_(value) { }

    uint32_t key() const noexcept { return key_; }

    const T& value() const noexcept { return value_; }
    T& value() noexcept { return value_; }
    void set_value(T&& value) noexcept { value_ = value; }

  private:
    uint32_t key_;
    T value_;
  };

  // Abstract base class for hash functions.
  class abstract_hash_func {
  public:

    virtual ~abstract_hash_func() { }

    // Evaluate the hash function for the given key.
    virtual uint32_t hash(uint32_t key) const noexcept = 0;
  };

  // p
  // h(x) = a0 + a1*x
  class poly2_hash_func : public abstract_hash_func {
  public:

    poly2_hash_func() noexcept {
      a_0 = 10;
      a_1 = 20;
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      uint32_t hashValue = a_0 + a_1*key;
      return hashValue;
    }

  private:
    int a_0;
    int a_1;
  };

  // Order-5 polynomial, i.e.
  // h(x) = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4
  class poly5_hash_func : public abstract_hash_func {
  public:

    poly5_hash_func() noexcept {
        a_0 = 5;
        a_1 = 6;
        a_2 = 7;
        a_3 = 8;
        a_4 = 9;
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      uint32_t hashValue = a_0 + a_1*key + a_2*key*key + a_3*key*key*key + a_4*key*key*key*key;
      return hashValue;
    }

  private:
    int a_0;
    int a_1;
    int a_2;
    int a_3;
    int a_4;
  };

  // Tabular-hash function, i.e. (4) 256-element arrays whose elements
  // are XORed together.
  class tabular_hash_func : public abstract_hash_func {
  public:

    tabular_hash_func() noexcept {
        for(int i = 0; i < 256; i++) {
            v1.push_back(rand()%2);
            v2.push_back(rand()%2);
            v3.push_back(rand()%2);
            v4.push_back(rand()%2);
        }
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      int n1 = (key >> 24) & 0xFF;
      int n2 = (key >> 16) & 0xFF;
      int n3 = (key >> 8) & 0xFF;
      int n4 = key & 0xFF;
      uint32_t hashValue = v1[n1%256]^v2[n2%256]^v3[n3%256]^v4[n4%256];
      return hashValue;
    }

  private:
    std::vector<int> v1;
    std::vector<int> v2;
    std::vector<int> v3;
    std::vector<int> v4;
  };

  // Abstract base class for a dictionary (hash table).
  template <typename T>
  class abstract_dict {
  public:

    virtual ~abstract_dict() { }

    // Search for the entry matching key, and return a reference to the
    // corresponding value.
    //
    // Throw std::out_of_range if there is no such key.
    //
    // (It would be better C++ practice to also provide a const overload of
    // this function, but that seems like busy-work for this experimental
    // project, so we're skipping that.)
    virtual T& search(uint32_t key) = 0;

    // Assign key to be associated with val. If key is already in the dictionary,
    // replace that association.
    //
    // Throw std::length_error if the dictionary is too full to add another
    // entry.
    virtual void set(uint32_t key, T&& val) = 0;

    // Remove the association with key.
    //
    // Throw std::out_of_range if key is not in the dictionary.
    virtual void remove(uint32_t key) = 0;
  };

  // Naive dictionary (unsorted vector).
  template <typename T>
  class naive_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    naive_dict(size_t capacity) {
    }

    virtual T& search(uint32_t key) {
      auto iter = search_iterator(key);
      if (iter != entries_.end()) {
        return iter->value();
      } else {
        throw std::out_of_range("key absent in naive_dict::search");
      }
    }

    virtual void set(uint32_t key, T&& val) {
      auto iter = search_iterator(key);
      if (iter != entries_.end()) {
        iter->set_value(std::move(val));
      } else {
        entries_.emplace_back(key, std::move(val));
      }
    }

    virtual void remove(uint32_t key) {
      auto iter = search_iterator(key);
      if (iter != entries_.end()) {
        entries_.erase(iter);
      } else {
        throw std::out_of_range("key absent in naive_dict::remove");
      }
    }

  private:

    std::vector<entry<T>> entries_;

    typename std::vector<entry<T>>::iterator search_iterator(uint32_t key) {
      return std::find_if(entries_.begin(),
                          entries_.end(),
                          [&](entry<T>& entry) { return entry.key() == key; });
    }

  };

  // Hash table with chaining.
  template <typename T>
  class chain_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    chain_dict(size_t capacity) {
      chainingTable = new std::list<entry<T>>[capacity];
      capacity_ = capacity;
    }

    virtual T& search(uint32_t key) {
        uint32_t hashValue =  poly2_hash_func().hash(key)%capacity_;
        auto iter = search_iterator(key, hashValue);
        if (iter != chainingTable[hashValue].end()) {
            return iter->value();
        } else {
            throw std::out_of_range("key absent in chain_dict::search");
        }
    }

    virtual void set(uint32_t key, T&& val) {
        int hashValue =  poly2_hash_func().hash(key)%capacity_;
        chainingTable[hashValue].emplace_back(key, std::move(val));
    }

    virtual void remove(uint32_t key) {
        uint32_t hashValue = poly2_hash_func().hash(key)%capacity_;
        auto iter = search_iterator(key, hashValue);
        if (iter != chainingTable[hashValue].end()) {
            chainingTable[hashValue].erase(iter);
        } else {
            throw std::out_of_range("key absent in chain_dict::remove");
        }
    }

  private:
    std::list<entry<T>> *chainingTable;
    size_t capacity_;
    typename std::list<entry<T>>::iterator search_iterator(uint32_t key, uint32_t hashValue) {
      return std::find_if(chainingTable[hashValue].begin(),
                          chainingTable[hashValue].end(),
                          [&](entry<T>& entry) { return entry.key() == key; });
    }
  };

  // Hash table with linear probing (LP).
  template <typename T>
  class lp_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    lp_dict(size_t capacity) {
        entry<T> tmp = entry<T>(INT_MIN, std::move(0));
        lpTable.resize(capacity,tmp);
        capacity_ = capacity;
    }

    virtual T& search(uint32_t key) {
        uint32_t hashValue =  poly5_hash_func().hash(key)%capacity_;
        int i=hashValue;
        int count = 0;
        while(lpTable[i].key()!=INT_MIN&&lpTable[i].key()!=key) {
            count ++;
            i=(i+1)%capacity_;
            if (count > capacity_) {
                throw std::out_of_range("key absent in lp_dict::search");
            }
        }
        if (lpTable[i].key()!=INT_MIN) {
            return lpTable[i].value();
        }
        throw std::out_of_range("key absent in lp_dict::search");
    }

    virtual void set(uint32_t key, T&& val) {
        uint32_t hashValue = poly5_hash_func().hash(key)%capacity_;
        int i=hashValue;
        while(lpTable[i].key()!=INT_MIN&&lpTable[i].key()!=key)
        {
            i=(i+1)%capacity_;
        }
        lpTable[i]=entry<T>(key, std::move(val));
    }

    virtual void remove(uint32_t key) {
        uint32_t hashValue =  poly5_hash_func().hash(key)%capacity_;
        int i=hashValue;
        int count = 0;
        while(lpTable[i].key()!=INT_MIN&&lpTable[i].key()!=key) {
            count++;
            i=(i+1)%capacity_;
            if (count > capacity_) {
                throw std::out_of_range("key absent in lp_dict::remove");
            }
        }
        entry<T> tmp = entry<T>(INT_MIN, std::move(0));
        if (lpTable[i].key()!=INT_MIN) {
            lpTable[i]=tmp;
        }
    }

  private:
      size_t capacity_;
      std::vector<entry<T>> lpTable;
  };

  // Cuckoo hash table.
  template <typename T>
  class cuckoo_dict : public abstract_dict<T> {
  public:
    int hash_cuckoo_1(int key) {
        return std::hash<int>()(key)%capacity_;
    }

    int hash_cuckoo_2(int key) {
      return (std::hash<int>()(key)/2)%capacity_;
    }
    // Create an empty dictionary, with the given capacity.
    cuckoo_dict(size_t capacity) {
      entry<T> tmp = entry<T>(INT_MIN, std::move(0));
      hashTable1.resize(capacity,tmp);
      hashTable2.resize(capacity,tmp);
      capacity_ = capacity;
      limit_ = log2(capacity);
    }

    virtual T& search(uint32_t key) {
        int hashValue1 = hash_cuckoo_1(key);
        int hashValue2 = hash_cuckoo_2(key);
        if (hashTable1[hashValue1].key() == key) {
            return hashTable1[hashValue1].value();
        } else if (hashTable2[hashValue2].key() == key) {
            return hashTable2[hashValue2].value();
        } else {
            throw std::out_of_range("key absent in cuckoo_dict::search");
        }
    }

    virtual void set(uint32_t key, T&& val) {
        int count = 0;
        while (count < limit_) {
            int hashValue1 = hash_cuckoo_1(key);
            if (hashTable1[hashValue1].key() == INT_MIN) {
                hashTable1[hashValue1] = entry<T>(key, std::move(val));
                break;
            } else if (hashTable1[hashValue1].key() != INT_MIN) {
                uint32_t tmp_key = hashTable1[hashValue1].key();
                T tmp_val = hashTable1[hashValue1].value();
                hashTable1[hashValue1] = entry<T>(key, std::move(val));
                key = tmp_key;
                val = tmp_val;
                int hashValue2 = hash_cuckoo_2(key);
                if (hashTable2[hashValue2].key() == INT_MIN) {
                    hashTable2[hashValue2] = entry<T>(key, std::move(val));
                    break;
                } else {
                    uint32_t tmp_key_2 = hashTable2[hashValue2].key();
                    T tmp_val_2 = hashTable2[hashValue2].value();
                    hashTable2[hashValue2] = entry<T>(key, std::move(val));
                    key = tmp_key_2;
                    val = tmp_val_2;
                    count++;
                    continue;
                }
            }
        }
    }

    virtual void remove(uint32_t key) {
        uint32_t hashValue1 = hash_cuckoo_1(key);
        uint32_t hashValue2 = hash_cuckoo_2(key);
        entry<T> tmp = entry<T>(INT_MIN, std::move(0));
        if (hashTable1[hashValue1].key() == key) {
            hashTable1[hashValue1] = tmp;
        } else if (hashTable2[hashValue2].key() == key) {
            hashTable2[hashValue2] = tmp;
        } else {
            throw std::out_of_range("key absent in cuckoo_dict::remove");
        }
    }

  private:
    // TODO: add data members, then delete this comment
    std::vector<entry<T>> hashTable1;
    std::vector<entry<T>> hashTable2;
    size_t capacity_;
    int limit_;
    int count_;
  };
}
