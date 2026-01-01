// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code (AI Assistant)

#include <gmock/gmock.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "backports/atomic_flag.h"
#include "util/GTestHelpers.h"
#include "util/SingleFlight.h"
#include "util/Timer.h"

using namespace std::literals;
using namespace std::chrono_literals;

// Helper class to coordinate test thread synchronization
class TestSignal {
  ql::atomic_flag flag_;

 public:
  void notify() {
    flag_.test_and_set();
    flag_.notify_all();
  }

  void wait() { flag_.wait(false); }
};

// Pair of signals to coordinate computation start and finish
struct StartStopSignal {
  TestSignal hasStarted_;
  TestSignal mayFinish_;
};

// _____________________________________________________________________________
TEST(SingleFlight, basicComputation) {
  ad_utility::SingleFlight<std::string, int> sf;

  // Simple computation - should be executed once
  int computeCount = 0;
  auto result = sf.getOrCompute("key1", [&computeCount]() {
    computeCount++;
    return 42;
  });

  ASSERT_EQ(result, 42);
  ASSERT_EQ(computeCount, 1);
  ASSERT_EQ(sf.inFlightCount(), 0u);  // No in-flight after completion
}

// _____________________________________________________________________________
TEST(SingleFlight, multipleSequentialCalls) {
  ad_utility::SingleFlight<std::string, std::string> sf;

  int computeCount = 0;
  auto computeFn = [&computeCount]() {
    computeCount++;
    return "result"s;
  };

  // First call - should compute
  auto result1 = sf.getOrCompute("key", computeFn);
  ASSERT_EQ(result1, "result");
  ASSERT_EQ(computeCount, 1);

  // Second call with same key - should compute again (no caching in
  // SingleFlight)
  auto result2 = sf.getOrCompute("key", computeFn);
  ASSERT_EQ(result2, "result");
  ASSERT_EQ(computeCount, 2);  // SingleFlight doesn't cache, each call
                               // computes
}

// _____________________________________________________________________________
TEST(SingleFlight, differentKeys) {
  ad_utility::SingleFlight<int, std::string> sf;

  auto result1 = sf.getOrCompute(1, []() { return "one"s; });
  auto result2 = sf.getOrCompute(2, []() { return "two"s; });
  auto result3 = sf.getOrCompute(3, []() { return "three"s; });

  ASSERT_EQ(result1, "one");
  ASSERT_EQ(result2, "two");
  ASSERT_EQ(result3, "three");
  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, exceptionHandling) {
  ad_utility::SingleFlight<std::string, int> sf;

  auto throwingFn = []() -> int {
    throw std::runtime_error("Computation failed");
  };

  // Should propagate the exception
  ASSERT_THROW(sf.getOrCompute("key", throwingFn), std::runtime_error);

  // In-flight map should be cleaned up after exception
  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, concurrentSameKeyWaitsForResult) {
  ad_utility::SingleFlight<std::string, std::string> sf;
  StartStopSignal signal;
  std::atomic<int> computeCount{0};

  auto computeFn = [&signal, &computeCount]() {
    computeCount++;
    signal.hasStarted_.notify();
    std::this_thread::sleep_for(10ms);
    signal.mayFinish_.wait();
    return "result"s;
  };

  // Start first thread that will compute
  auto future1 = std::async(std::launch::async,
                            [&sf, &computeFn]() {
                              return sf.getOrCompute("key", computeFn);
                            });

  // Wait for computation to start
  signal.hasStarted_.wait();

  // Verify the key is in-flight
  ASSERT_EQ(sf.inFlightCount(), 1u);

  // Start second thread with same key - should wait, not compute
  auto computeFn2 = [&computeCount]() {
    computeCount++;
    return "should not be called"s;
  };
  auto future2 = std::async(std::launch::async,
                            [&sf, &computeFn2]() {
                              return sf.getOrCompute("key", computeFn2);
                            });

  // Give second thread time to reach the wait point
  std::this_thread::sleep_for(5ms);

  // computeCount should still be 1 (only first thread computed)
  ASSERT_EQ(computeCount.load(), 1);

  // Allow first computation to finish
  signal.mayFinish_.notify();

  // Both threads should get the same result
  auto result1 = future1.get();
  auto result2 = future2.get();

  ASSERT_EQ(result1, "result");
  ASSERT_EQ(result2, "result");
  ASSERT_EQ(computeCount.load(), 1);  // Only computed once
  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, stampedeTestTwentyThreads) {
  ad_utility::SingleFlight<std::string, int> sf;
  StartStopSignal signal;
  std::atomic<int> computeCount{0};
  constexpr int numThreads = 20;
  constexpr int computeTimeMs = 100;

  auto computeFn = [&signal, &computeCount]() {
    int count = computeCount.fetch_add(1);
    if (count == 0) {
      // Only the first thread signals start
      signal.hasStarted_.notify();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(computeTimeMs));
    signal.mayFinish_.wait();
    return 42;
  };

  ad_utility::Timer timer{ad_utility::Timer::Started};

  // Launch all threads requesting the same key
  std::vector<std::future<int>> futures;
  futures.reserve(numThreads);

  for (int i = 0; i < numThreads; i++) {
    futures.push_back(std::async(std::launch::async, [&sf, &computeFn]() {
      return sf.getOrCompute("same-key", computeFn);
    }));
  }

  // Wait for first computation to start
  signal.hasStarted_.wait();

  // Give other threads time to queue up
  std::this_thread::sleep_for(20ms);

  // Allow computation to finish
  signal.mayFinish_.notify();

  // Collect all results
  std::vector<int> results;
  for (auto& future : futures) {
    results.push_back(future.get());
  }

  auto elapsedMs = timer.msecs();

  // Verify all threads got the same result
  ASSERT_EQ(results.size(), numThreads);
  for (const auto& result : results) {
    ASSERT_EQ(result, 42);
  }

  // CRITICAL: Verify compute function was called exactly once
  ASSERT_EQ(computeCount.load(), 1)
      << "Stampede prevention failed: compute function called "
      << computeCount.load() << " times instead of 1";

  // Verify timing: should take ~1x compute time, not 20x
  // Allow some overhead for thread coordination (2x is generous upper bound)
  ASSERT_LT(elapsedMs, 2 * computeTimeMs * 1ms)
      << "Execution took " << elapsedMs.count()
      << "ms, expected ~" << computeTimeMs << "ms (threads may have run serially)";

  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, exceptionPropagationToWaiters) {
  ad_utility::SingleFlight<std::string, int> sf;
  StartStopSignal signal;

  auto throwingFn = [&signal]() -> int {
    signal.hasStarted_.notify();
    signal.mayFinish_.wait();
    throw std::runtime_error("Intentional failure");
  };

  // Start first thread that will throw
  auto future1 = std::async(std::launch::async, [&sf, &throwingFn]() {
    return sf.getOrCompute("key", throwingFn);
  });

  // Wait for computation to start
  signal.hasStarted_.wait();

  // Start second thread - should wait for result
  auto normalFn = []() -> int {
    ADD_FAILURE() << "Second thread should not compute";
    return 0;
  };
  auto future2 = std::async(std::launch::async, [&sf, &normalFn]() {
    return sf.getOrCompute("key", normalFn);
  });

  // Allow first thread to throw
  signal.mayFinish_.notify();

  // Both threads should see the exception
  ASSERT_THROW(future1.get(), std::runtime_error);
  ASSERT_THROW(future2.get(), std::runtime_error);

  // In-flight map should be cleaned up
  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, clearInFlight) {
  ad_utility::SingleFlight<std::string, int> sf;
  StartStopSignal signal;

  auto computeFn = [&signal]() {
    signal.hasStarted_.notify();
    signal.mayFinish_.wait();
    return 42;
  };

  // Start a computation
  auto future = std::async(std::launch::async,
                           [&sf, &computeFn]() {
                             return sf.getOrCompute("key", computeFn);
                           });

  signal.hasStarted_.wait();
  ASSERT_EQ(sf.inFlightCount(), 1u);

  // Clear the in-flight map
  sf.clear();
  ASSERT_EQ(sf.inFlightCount(), 0u);

  // The computation should still complete successfully
  signal.mayFinish_.notify();
  auto result = future.get();
  ASSERT_EQ(result, 42);
}

// _____________________________________________________________________________
TEST(SingleFlight, concurrentDifferentKeys) {
  ad_utility::SingleFlight<int, std::string> sf;
  constexpr int numKeys = 10;

  // Launch threads computing different keys concurrently
  std::vector<std::future<std::string>> futures;
  for (int i = 0; i < numKeys; i++) {
    futures.push_back(std::async(std::launch::async, [&sf, i]() {
      return sf.getOrCompute(i, [i]() {
        std::this_thread::sleep_for(10ms);
        return "result-" + std::to_string(i);
      });
    }));
  }

  // All should complete with different results
  for (int i = 0; i < numKeys; i++) {
    auto result = futures[i].get();
    ASSERT_EQ(result, "result-" + std::to_string(i));
  }

  ASSERT_EQ(sf.inFlightCount(), 0u);
}

// _____________________________________________________________________________
TEST(SingleFlight, timingVerificationSingleVsMultiple) {
  ad_utility::SingleFlight<std::string, int> sf;
  constexpr int computeTimeMs = 50;

  // Measure single computation time
  ad_utility::Timer singleTimer{ad_utility::Timer::Started};
  auto result1 = sf.getOrCompute("key1", [computeTimeMs]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(computeTimeMs));
    return 1;
  });
  auto singleTime = singleTimer.msecs();

  ASSERT_EQ(result1, 1);
  ASSERT_GE(singleTime, computeTimeMs * 1ms);

  // Measure concurrent computation time with 10 threads
  StartStopSignal signal;
  std::atomic<int> computeCount{0};

  auto concurrentComputeFn = [&signal, &computeCount, computeTimeMs]() {
    if (computeCount.fetch_add(1) == 0) {
      signal.hasStarted_.notify();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(computeTimeMs));
    signal.mayFinish_.wait();
    return 2;
  };

  ad_utility::Timer concurrentTimer{ad_utility::Timer::Started};

  std::vector<std::future<int>> futures;
  for (int i = 0; i < 10; i++) {
    futures.push_back(
        std::async(std::launch::async, [&sf, &concurrentComputeFn]() {
          return sf.getOrCompute("key2", concurrentComputeFn);
        }));
  }

  signal.hasStarted_.wait();
  signal.mayFinish_.notify();

  for (auto& future : futures) {
    ASSERT_EQ(future.get(), 2);
  }

  auto concurrentTime = concurrentTimer.msecs();

  // Concurrent time should be roughly the same as single time (not 10x)
  ASSERT_EQ(computeCount.load(), 1);  // Only one computation
  ASSERT_LT(concurrentTime, 2 * singleTime)
      << "Concurrent execution took " << concurrentTime.count()
      << "ms, expected ~" << singleTime.count() << "ms";
}

// _____________________________________________________________________________
TEST(SingleFlight, complexValueType) {
  struct ComplexValue {
    std::string name;
    std::vector<int> data;
    int count;

    bool operator==(const ComplexValue& other) const {
      return name == other.name && data == other.data && count == other.count;
    }
  };

  ad_utility::SingleFlight<std::string, ComplexValue> sf;

  auto result = sf.getOrCompute("complex", []() {
    return ComplexValue{"test", {1, 2, 3, 4, 5}, 42};
  });

  ASSERT_EQ(result.name, "test");
  ASSERT_EQ(result.data, std::vector<int>({1, 2, 3, 4, 5}));
  ASSERT_EQ(result.count, 42);
}
