// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code (AI Assistant)

#ifndef QLEVER_SINGLEFLIGHT_H
#define QLEVER_SINGLEFLIGHT_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include "util/HashMap.h"
#include "util/Synchronized.h"

namespace ad_utility {

/**
 * @brief Prevents cache stampedes by ensuring only one computation per key
 * happens at a time.
 *
 * When multiple threads request the same key concurrently:
 * - The first thread executes the compute function
 * - Subsequent threads wait for the first thread to complete
 * - All threads receive the same result
 *
 * This is a generic utility that can be used with any cache layer to prevent
 * multiple concurrent computations of the same expensive operation.
 *
 * @tparam KeyT The key type (must be hashable and equality comparable)
 * @tparam ValueT The computed result type
 *
 * Thread-safe: All operations are protected by internal synchronization.
 *
 * Example usage:
 * @code
 *   SingleFlight<std::string, ExpensiveResult> sf;
 *   auto result = sf.getOrCompute("key", []() {
 *     return computeExpensiveResult();
 *   });
 * @endcode
 */
template <typename KeyT, typename ValueT>
class SingleFlight {
 private:
  /**
   * @brief Represents an in-flight computation that multiple threads can wait
   * for.
   *
   * This class handles the coordination between the computing thread and
   * waiting threads using condition variables and promises. It ensures that:
   * - Only one thread computes the result
   * - All waiting threads receive the same result or exception
   * - Proper cleanup happens on both success and failure
   */
  class InFlightComputation {
   public:
    InFlightComputation() = default;

    /**
     * @brief Signal successful completion of the computation.
     * @param result The computed result (moved into shared_ptr)
     *
     * Notifies all waiting threads that the result is ready.
     * Must be called exactly once by the computing thread.
     */
    void finish(std::shared_ptr<ValueT> result) {
      std::unique_lock lock(mutex_);
      AD_CONTRACT_CHECK(status_ == Status::IN_PROGRESS);
      status_ = Status::FINISHED;
      result_ = std::move(result);
      lock.unlock();
      cv_.notify_all();
    }

    /**
     * @brief Signal failure of the computation.
     * @param exception The exception to propagate to waiting threads
     *
     * Stores the exception and notifies all waiting threads.
     * Must be called exactly once by the computing thread if computation
     * fails.
     */
    void abort(std::exception_ptr exception) {
      std::unique_lock lock(mutex_);
      AD_CONTRACT_CHECK(status_ == Status::IN_PROGRESS);
      status_ = Status::ABORTED;
      exception_ = std::move(exception);
      lock.unlock();
      cv_.notify_all();
    }

    /**
     * @brief Wait for the computation to complete and get the result.
     * @return The computed result (shared_ptr to allow sharing)
     * @throws The original exception if the computation failed
     *
     * Blocks until the computing thread calls finish() or abort().
     * If the computation failed, re-throws the original exception.
     */
    std::shared_ptr<ValueT> getResult() {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return status_ != Status::IN_PROGRESS; });

      if (status_ == Status::ABORTED) {
        AD_CONTRACT_CHECK(exception_ != nullptr);
        std::rethrow_exception(exception_);
      }

      AD_CONTRACT_CHECK(status_ == Status::FINISHED);
      AD_CONTRACT_CHECK(result_ != nullptr);
      return result_;
    }

   private:
    enum class Status { IN_PROGRESS, FINISHED, ABORTED };

    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    Status status_ = Status::IN_PROGRESS;
    std::shared_ptr<ValueT> result_;
    std::exception_ptr exception_;
  };

  // Type alias for the in-flight computation map
  using InFlightMap =
      HashMap<KeyT, std::shared_ptr<InFlightComputation>>;

 public:
  SingleFlight() = default;

  /**
   * @brief Get the result for a key, computing it if necessary.
   *
   * This is the core single-flight pattern implementation:
   * 1. Check if key is already being computed by another thread
   * 2. If yes: wait for that computation to finish
   * 3. If no: start the computation and notify other waiters when done
   *
   * @param key The unique identifier for this computation
   * @param computeFn Function that computes the result (called at most once per
   * key)
   * @return The computed result (by value, safe to use after call)
   * @throws Any exception thrown by computeFn (propagated to all waiters)
   *
   * Thread-safe: Multiple threads can call this concurrently with the same or
   * different keys.
   *
   * Performance: Only the computing thread executes computeFn. Waiting threads
   * block efficiently on a condition variable.
   */
  ValueT getOrCompute(const KeyT& key, std::function<ValueT()> computeFn) {
    bool mustCompute = false;
    std::shared_ptr<InFlightComputation> computation;

    // Critical section: determine if we need to compute or wait
    {
      auto lock = inFlightMap_.wlock();

      auto it = lock->find(key);
      if (it != lock->end()) {
        // Another thread is already computing this key
        mustCompute = false;
        computation = it->second;
      } else {
        // We are the first thread to request this key
        mustCompute = true;
        computation = std::make_shared<InFlightComputation>();
        lock->insert({key, computation});
      }
    }  // Release lock - we don't hold it during computation

    if (mustCompute) {
      // We are responsible for computing the result
      try {
        // Compute the result (this may take a long time)
        auto result = std::make_shared<ValueT>(computeFn());

        // Clean up the in-flight map
        inFlightMap_.wlock()->erase(key);

        // Notify all waiting threads with the result
        computation->finish(result);

        // Return a copy of the result
        return *result;
      } catch (...) {
        // Computation failed - clean up and propagate exception
        inFlightMap_.wlock()->erase(key);

        // Notify waiting threads about the failure
        computation->abort(std::current_exception());

        // Re-throw the exception
        throw;
      }
    } else {
      // Another thread is computing - wait for the result
      auto result = computation->getResult();
      return *result;
    }
  }

  /**
   * @brief Clear all in-flight computations.
   *
   * This is primarily useful for testing and epoch boundaries.
   * Note: This does NOT cancel ongoing computations - it only removes
   * them from the registry. Waiting threads will still receive results.
   */
  void clear() { inFlightMap_.wlock()->clear(); }

  /**
   * @brief Get the number of in-flight computations (for testing/monitoring)
   * @return The number of keys currently being computed
   */
  size_t inFlightCount() const { return inFlightMap_.wlock()->size(); }

 private:
  // Map of currently in-flight computations, protected by Synchronized
  ad_utility::Synchronized<InFlightMap, std::mutex> inFlightMap_;
};

}  // namespace ad_utility

#endif  // QLEVER_SINGLEFLIGHT_H
