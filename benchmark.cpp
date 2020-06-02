///////////////////////////////////////////////////////////////////////////////
// benchmark.cpp
//
// Main program that performs a timed benchmark of dictionary operations.
//
// Students: you do not need to modify this file.
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "hashes.hpp"

using namespace std;
using namespace hashes;

const uint32_t SEED{0};

void print_usage() {
  cout << "usage:" << endl
       << "    benchmark <STRUCTURE> <N>" << endl
       << endl
       << "where" << endl
       << "    <STRUCTURE> is one of: naive chain lp cuckoo" << endl
       << "    <N>: input size (positive integer)" << endl
       << endl;
}

int main(int argc, char* argv[]) {

  // parse commandline arguments

  vector<string> arguments(argv, argv + argc);

  if (arguments.size() != 3) {
    print_usage();
    return 1;
  }

  auto& structure = arguments[1],
        n_string = arguments[2];

  unsigned n;
  try {
    int parsed{stoi(n_string)};
    if (parsed <= 0) {
      cout << "error: input size " << parsed << " must be positive" << endl;
      return 1;
    }
    n = parsed;
  } catch (std::invalid_argument e) {
    cout << "error: '" << n_string << "' is not an integer" << endl;
    return 1;
  }
  assert(n > 0);

  unique_ptr<abstract_dict<uint32_t>> dict;
  if (structure == "naive") {
    dict.reset(new naive_dict<uint32_t>(n));
  } else if (structure == "chain") {
    dict.reset(new chain_dict<uint32_t>(n));
  } else if (structure == "lp") {
    dict.reset(new lp_dict<uint32_t>(n));
  } else if (structure == "cuckoo") {
    dict.reset(new cuckoo_dict<uint32_t>(n));
  } else {
    print_usage();
    return 1;
  }
  assert(dict);

  // print parameters
  cout << "== dictionary benchmark ==" << endl
       << "structure: " << structure << endl
       << "n: " << n << endl;


  cout << "generating input..." << flush;
  vector<uint32_t> insert, // n elements to insert
                   remove, // n/4 of those elements to remove
                   absent; // n/2 elements to search for, that were not inserted
  {
    const unsigned insert_n = n,
                   remove_n = n/4,
                   absent_n = n/2,
                   total_n = insert_n + absent_n;

    // Create a random permutation of [0, total_n). This guarantees all
    // values are distinct.
    mt19937 gen(SEED);
    std::vector<uint32_t> randoms(total_n);
    for (unsigned i = 0; i < total_n; ++i) {
      randoms[i] = i;
    }
    std::shuffle(randoms.begin(), randoms.end(), gen);

    // partition randoms into those to insert, and those that will be absent
    auto boundary = randoms.begin() + insert_n;
    insert.assign(randoms.begin(), boundary);
    absent.assign(boundary, randoms.end());

    // pick n/4 elements to remove
    remove.assign(insert.begin(), insert.begin() + remove_n);
    // reshuffle insert so the removals aren't all clustered at the beginning
    std::shuffle(insert.begin(), insert.end(), gen);
  }

  // start high resolution clock
  using clock = chrono::high_resolution_clock;
  auto start = clock::now();

  cout << endl << "insert..." << flush;
  for (auto x : insert) {
    try {
      dict->set(x, x+1);
    } catch (std::length_error e) {
      cout << "error: dict ran out of space" << endl;
      return 1;
    }
  }

  cout << endl << "search for absent elements..." << flush;
  for (auto x : absent) {
    try {
      auto& value = dict->search(x);
      cout << "error: search(" << x << ") found value " << value
           << ", but that key shouldn't be present" << endl;
      return 1;
    } catch (std::out_of_range e) {
      // expected to throw
    }
  }

  cout << endl << "search for present elements..." << flush;
  for (auto x : insert) {
    try {
      uint32_t expected_value = (x+1);
      auto& value = dict->search(x);
      if (value != expected_value) {
        cout << "error: search(" << x << ") found value " << value
             << ", which should be " << expected_value << endl;
        return 1;
      }
    } catch (std::out_of_range e) {
      cout << "error: search(" << x << ") failed";
      return 1;
    }
  }

  cout << endl <<  "remove..." << flush;
  for (auto x : remove) {
    try {
      dict->remove(x);
    } catch (std::out_of_range e) {
      cout << "error: remove(" << x << ") failed, even though that element"
           << " should be present" << endl;
      return 1;
    }
  }

  cout << endl << "search again..." << flush;
  for (auto x : insert) {
    try {
      dict->search(x);
    } catch (std::out_of_range e) {
      // we expect some searches to fail
    }
  }

  // stop the clock
  auto end = clock::now();

  // print elapsed time
  double seconds = chrono::duration_cast<chrono::duration<double>>(end - start).count();
  cout << endl << "elapsed time: " << seconds << " seconds" << endl;

  return 0;
}
