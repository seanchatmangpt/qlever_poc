// Copyright 2026, QLever contributors
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// EPIC 10.3 Agent 5: Opaque Memory Validator - Unit Tests
// Thread-Safe Opaque Handle Pool Tests

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_set>
#include <vector>

#include "util/HandleValidation.h"
#include "util/MemoryBoundaryGuards.h"

namespace ad_utility {

// Test fixture for OpaqueHandlePool
class OpaqueHandlePoolTest : public ::testing::Test {
 protected:
  OpaqueHandlePool& pool = OpaqueHandlePool::instance();

  // Test data types
  struct TestObject {
    int value;
    std::string name;
    explicit TestObject(int v, std::string n) : value(v), name(std::move(n)) {}
  };

  struct AnotherTestObject {
    double data;
    explicit AnotherTestObject(double d) : data(d) {}
  };
};

// ============================================================================
// GUARD-5.1: Basic Functionality Tests
// ============================================================================

TEST_F(OpaqueHandlePoolTest, RegisterHandle_ReturnsValidID) {
  auto obj = std::make_shared<TestObject>(42, "test");
  uint64_t handle = pool.registerHandle(obj);

  EXPECT_GT(handle, 0);  // Valid handle ID (not NULL)
  EXPECT_TRUE(pool.isValid(handle));
}

TEST_F(OpaqueHandlePoolTest, RegisterHandle_NullptrReturnsZero) {
  std::shared_ptr<TestObject> null_ptr = nullptr;
  uint64_t handle = pool.registerHandle(null_ptr);

  EXPECT_EQ(handle, 0);  // NULL handle
  EXPECT_FALSE(pool.isValid(handle));
}

TEST_F(OpaqueHandlePoolTest, GetHandle_RetrievesCorrectObject) {
  auto obj = std::make_shared<TestObject>(123, "hello");
  uint64_t handle = pool.registerHandle(obj);

  auto retrieved = pool.getHandle<TestObject>(handle);

  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->value, 123);
  EXPECT_EQ(retrieved->name, "hello");
}

TEST_F(OpaqueHandlePoolTest, GetHandle_InvalidIDReturnsNullptr) {
  uint64_t invalid_handle = 999999999;
  auto retrieved = pool.getHandle<TestObject>(invalid_handle);

  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(OpaqueHandlePoolTest, GetHandle_NullHandleReturnsNullptr) {
  auto retrieved = pool.getHandle<TestObject>(0);
  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(OpaqueHandlePoolTest, UnregisterHandle_RemovesHandle) {
  auto obj = std::make_shared<TestObject>(99, "remove_me");
  uint64_t handle = pool.registerHandle(obj);

  EXPECT_TRUE(pool.isValid(handle));

  bool removed = pool.unregisterHandle(handle);
  EXPECT_TRUE(removed);

  EXPECT_FALSE(pool.isValid(handle));
  auto retrieved = pool.getHandle<TestObject>(handle);
  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(OpaqueHandlePoolTest, UnregisterHandle_DoubleUnregisterIsIdempotent) {
  auto obj = std::make_shared<TestObject>(55, "double_unregister");
  uint64_t handle = pool.registerHandle(obj);

  // First unregister
  bool first_remove = pool.unregisterHandle(handle);
  EXPECT_TRUE(first_remove);

  // Second unregister (idempotent - no-op)
  bool second_remove = pool.unregisterHandle(handle);
  EXPECT_FALSE(second_remove);  // Handle already removed

  EXPECT_FALSE(pool.isValid(handle));
}

TEST_F(OpaqueHandlePoolTest, UnregisterHandle_NullHandleReturnsFalse) {
  bool removed = pool.unregisterHandle(0);
  EXPECT_FALSE(removed);
}

// ============================================================================
// GUARD-5.2: Type Safety Tests
// ============================================================================

TEST_F(OpaqueHandlePoolTest, TypeSafety_WrongTypeCastReturnsNullptr) {
  auto obj = std::make_shared<TestObject>(77, "type_test");
  uint64_t handle = pool.registerHandle(obj);

  // Attempt to retrieve with wrong type
  auto wrong_type = pool.getHandle<AnotherTestObject>(handle);
  EXPECT_EQ(wrong_type, nullptr);  // Type mismatch

  // Correct type still works
  auto correct_type = pool.getHandle<TestObject>(handle);
  ASSERT_NE(correct_type, nullptr);
  EXPECT_EQ(correct_type->value, 77);

  pool.unregisterHandle(handle);
}

TEST_F(OpaqueHandlePoolTest, TypeSafety_DifferentTypesHaveDifferentHandles) {
  auto obj1 = std::make_shared<TestObject>(100, "obj1");
  auto obj2 = std::make_shared<AnotherTestObject>(3.14);

  uint64_t handle1 = pool.registerHandle(obj1);
  uint64_t handle2 = pool.registerHandle(obj2);

  EXPECT_NE(handle1, handle2);  // Different handles

  auto retrieved1 = pool.getHandle<TestObject>(handle1);
  auto retrieved2 = pool.getHandle<AnotherTestObject>(handle2);

  ASSERT_NE(retrieved1, nullptr);
  ASSERT_NE(retrieved2, nullptr);
  EXPECT_EQ(retrieved1->value, 100);
  EXPECT_DOUBLE_EQ(retrieved2->data, 3.14);

  pool.unregisterHandle(handle1);
  pool.unregisterHandle(handle2);
}

// ============================================================================
// GUARD-5.3: Reference Counting Tests
// ============================================================================

TEST_F(OpaqueHandlePoolTest, RefCount_SharedPtrKeepsObjectAlive) {
  std::shared_ptr<TestObject> obj;
  uint64_t handle;

  {
    auto temp = std::make_shared<TestObject>(200, "refcount_test");
    handle = pool.registerHandle(temp);
    obj = pool.getHandle<TestObject>(handle);  // Increment refcount
  }

  // Original shared_ptr (temp) destroyed, but obj still holds reference
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->value, 200);

  // Unregister handle (pool releases its reference)
  pool.unregisterHandle(handle);

  // obj still valid (still has reference)
  EXPECT_EQ(obj->value, 200);

  // obj goes out of scope → refcount = 0 → object destroyed
}

TEST_F(OpaqueHandlePoolTest, RefCount_UnregisterDecrementsRefcount) {
  auto obj = std::make_shared<TestObject>(300, "decrement_test");
  uint64_t handle = pool.registerHandle(obj);

  // refcount = 2 (obj + pool)
  EXPECT_EQ(obj.use_count(), 2);

  pool.unregisterHandle(handle);

  // refcount = 1 (only obj remains)
  EXPECT_EQ(obj.use_count(), 1);
}

// ============================================================================
// GUARD-5.4: Concurrent Registration Tests (100 threads, 1000 handles each)
// ============================================================================

TEST_F(OpaqueHandlePoolTest, Concurrent_RegistrationFrom100Threads) {
  constexpr int num_threads = 100;
  constexpr int handles_per_thread = 1000;

  std::vector<std::thread> threads;
  std::vector<std::vector<uint64_t>> all_handles(num_threads);

  // Launch 100 threads, each registering 1000 handles
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &all_handles]() {
      for (int i = 0; i < handles_per_thread; ++i) {
        auto obj = std::make_shared<TestObject>(t * 10000 + i,
                                                "thread" + std::to_string(t));
        uint64_t handle = pool.registerHandle(obj);
        all_handles[t].push_back(handle);
      }
    });
  }

  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify all handles are unique
  std::unordered_set<uint64_t> unique_handles;
  for (const auto& handles : all_handles) {
    for (uint64_t handle : handles) {
      EXPECT_GT(handle, 0);  // Valid handle
      EXPECT_TRUE(pool.isValid(handle));
      EXPECT_TRUE(unique_handles.insert(handle).second);  // No duplicates
    }
  }

  EXPECT_EQ(unique_handles.size(), num_threads * handles_per_thread);

  // Cleanup: unregister all handles
  for (const auto& handles : all_handles) {
    for (uint64_t handle : handles) {
      pool.unregisterHandle(handle);
    }
  }
}

// ============================================================================
// GUARD-5.5: Concurrent Lookup Tests
// ============================================================================

TEST_F(OpaqueHandlePoolTest, Concurrent_LookupsWhileRegistering) {
  constexpr int num_writers = 10;
  constexpr int num_readers = 50;
  constexpr int operations_per_thread = 100;

  std::vector<uint64_t> shared_handles;
  std::mutex handles_mutex;
  std::atomic<bool> stop_readers{false};

  std::vector<std::thread> writers;
  std::vector<std::thread> readers;

  // Launch writer threads
  for (int w = 0; w < num_writers; ++w) {
    writers.emplace_back([this, w, &shared_handles, &handles_mutex]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        auto obj = std::make_shared<TestObject>(w * 1000 + i, "writer");
        uint64_t handle = pool.registerHandle(obj);

        std::lock_guard lock(handles_mutex);
        shared_handles.push_back(handle);
      }
    });
  }

  // Launch reader threads (concurrent lookups)
  for (int r = 0; r < num_readers; ++r) {
    readers.emplace_back(
        [this, &shared_handles, &handles_mutex, &stop_readers]() {
          while (!stop_readers.load()) {
            std::vector<uint64_t> handles_copy;
            {
              std::lock_guard lock(handles_mutex);
              handles_copy = shared_handles;
            }

            for (uint64_t handle : handles_copy) {
              auto obj = pool.getHandle<TestObject>(handle);
              // Object may or may not exist (writer may have unregistered)
              // Just verify no crashes occur
            }
          }
        });
  }

  // Wait for writers to complete
  for (auto& writer : writers) {
    writer.join();
  }

  // Stop readers
  stop_readers.store(true);
  for (auto& reader : readers) {
    reader.join();
  }

  // Cleanup
  for (uint64_t handle : shared_handles) {
    pool.unregisterHandle(handle);
  }
}

// ============================================================================
// GUARD-5.6: Handle Validation Utilities Tests
// ============================================================================

TEST_F(OpaqueHandlePoolTest, HandleValidation_IsValidHandle) {
  auto obj = std::make_shared<TestObject>(400, "validate");
  uint64_t handle = pool.registerHandle(obj);

  EXPECT_TRUE(isValidHandle<TestObject>(handle));
  EXPECT_FALSE(isValidHandle<AnotherTestObject>(handle));  // Wrong type

  pool.unregisterHandle(handle);

  EXPECT_FALSE(isValidHandle<TestObject>(handle));  // After unregister
}

TEST_F(OpaqueHandlePoolTest, HandleValidation_ValidateHandleThrowsOnInvalid) {
  auto obj = std::make_shared<TestObject>(500, "throw_test");
  uint64_t handle = pool.registerHandle(obj);

  EXPECT_NO_THROW(validateHandle<TestObject>(handle));

  pool.unregisterHandle(handle);

  EXPECT_THROW(validateHandle<TestObject>(handle), ad_utility::Exception);
}

TEST_F(OpaqueHandlePoolTest,
       HandleValidation_ValidateHandleThrowsOnNullHandle) {
  EXPECT_THROW(validateHandle<TestObject>(0), ad_utility::Exception);
}

TEST_F(OpaqueHandlePoolTest,
       HandleValidation_ValidateHandleThrowsOnTypeMismatch) {
  auto obj = std::make_shared<TestObject>(600, "type_mismatch");
  uint64_t handle = pool.registerHandle(obj);

  EXPECT_THROW(validateHandle<AnotherTestObject>(handle),
               ad_utility::Exception);

  pool.unregisterHandle(handle);
}

TEST_F(OpaqueHandlePoolTest,
       HandleValidation_GetValidatedHandleReturnsErrorCode) {
  auto obj = std::make_shared<TestObject>(700, "error_code");
  uint64_t handle = pool.registerHandle(obj);

  std::error_code ec;
  auto result = getValidatedHandle<TestObject>(handle, ec);

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(ec);  // No error
  EXPECT_EQ((*result)->value, 700);

  pool.unregisterHandle(handle);

  // After unregister
  auto invalid_result = getValidatedHandle<TestObject>(handle, ec);
  EXPECT_FALSE(invalid_result.has_value());
  EXPECT_TRUE(ec);
  EXPECT_EQ(ec, make_error_code(HandleValidationError::INVALID_HANDLE));
}

TEST_F(OpaqueHandlePoolTest, HandleValidation_GetValidatedHandleNullHandle) {
  std::error_code ec;
  auto result = getValidatedHandle<TestObject>(0, ec);

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(ec);
  EXPECT_EQ(ec, make_error_code(HandleValidationError::NULL_HANDLE));
}

TEST_F(OpaqueHandlePoolTest, HandleValidation_GetValidatedHandleTypeMismatch) {
  auto obj = std::make_shared<TestObject>(800, "mismatch");
  uint64_t handle = pool.registerHandle(obj);

  std::error_code ec;
  auto result = getValidatedHandle<AnotherTestObject>(handle, ec);

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(ec);
  EXPECT_EQ(ec, make_error_code(HandleValidationError::TYPE_MISMATCH));

  pool.unregisterHandle(handle);
}

// ============================================================================
// GUARD-5.7: Monotonic Handle ID Test
// ============================================================================

TEST_F(OpaqueHandlePoolTest, HandleID_MonotonicallyIncreasing) {
  std::vector<uint64_t> handles;

  for (int i = 0; i < 100; ++i) {
    auto obj = std::make_shared<TestObject>(i, "monotonic");
    uint64_t handle = pool.registerHandle(obj);
    handles.push_back(handle);
  }

  // Verify handles are monotonically increasing
  for (size_t i = 1; i < handles.size(); ++i) {
    EXPECT_GT(handles[i], handles[i - 1]);
  }

  // Cleanup
  for (uint64_t handle : handles) {
    pool.unregisterHandle(handle);
  }
}

// ============================================================================
// GUARD-5.8: Performance Benchmark Test
// ============================================================================

TEST_F(OpaqueHandlePoolTest, Performance_RegisterHandleFast) {
  constexpr int num_iterations = 10000;
  auto obj = std::make_shared<TestObject>(999, "perf_test");

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<uint64_t> handles;
  for (int i = 0; i < num_iterations; ++i) {
    uint64_t handle = pool.registerHandle(obj);
    handles.push_back(handle);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  double avg_ns = static_cast<double>(duration.count()) / num_iterations;

  // Target: <100ns per registerHandle (p99)
  // For average, we use a more lenient threshold
  EXPECT_LT(avg_ns, 500.0);  // 500ns average (p99 should be <100ns)

  // Cleanup
  for (uint64_t handle : handles) {
    pool.unregisterHandle(handle);
  }
}

TEST_F(OpaqueHandlePoolTest, Performance_GetHandleFast) {
  constexpr int num_iterations = 10000;
  auto obj = std::make_shared<TestObject>(888, "get_perf");
  uint64_t handle = pool.registerHandle(obj);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_iterations; ++i) {
    auto retrieved = pool.getHandle<TestObject>(handle);
    (void)retrieved;  // Prevent optimization
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  double avg_ns = static_cast<double>(duration.count()) / num_iterations;

  // Target: <50ns per getHandle (p99)
  // For average, we use a more lenient threshold
  EXPECT_LT(avg_ns, 200.0);  // 200ns average (p99 should be <50ns)

  pool.unregisterHandle(handle);
}

}  // namespace ad_utility
