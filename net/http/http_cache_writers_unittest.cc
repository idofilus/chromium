// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_cache_writers.h"

#include <memory>
#include <string>
#include <vector>

#include "base/run_loop.h"
#include "net/http/http_cache.h"
#include "net/http/http_cache_transaction.h"
#include "net/http/http_response_info.h"
#include "net/http/http_transaction.h"
#include "net/http/http_transaction_test_util.h"
#include "net/http/mock_http_cache.h"
#include "net/http/partial_data.h"
#include "net/test/gtest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsError;
using net::test::IsOk;

namespace net {

class WritersTest;

class TestHttpCacheTransaction : public HttpCache::Transaction {
  typedef WebSocketHandshakeStreamBase::CreateHelper CreateHelper;

 public:
  TestHttpCacheTransaction(RequestPriority priority, HttpCache* cache)
      : HttpCache::Transaction(priority, cache){};
  ~TestHttpCacheTransaction() override{};

  Transaction::Mode mode() const override { return Transaction::READ_WRITE; }
};

class TestHttpCache : public HttpCache {
 public:
  TestHttpCache(std::unique_ptr<HttpTransactionFactory> network_layer,
                std::unique_ptr<BackendFactory> backend_factory)
      : HttpCache(std::move(network_layer), std::move(backend_factory), false) {
  }

  void WritersDoneWritingToEntry(ActiveEntry* entry,
                                 bool success,
                                 bool should_keep_entry,
                                 TransactionSet) override {}

  void WritersDoomEntryRestartTransactions(ActiveEntry* entry) override {}
};

class WritersTest : public testing::Test {
 public:
  enum class DeleteTransactionType { NONE, ACTIVE, WAITING, IDLE };
  WritersTest()
      : disk_entry_(nullptr),
        test_cache_(base::MakeUnique<MockNetworkLayer>(),
                    base::MakeUnique<MockBackendFactory>()),
        request_(kSimpleGET_Transaction) {
    ScopedMockTransaction transaction(kSimpleGET_Transaction);
    transaction.response_headers =
        "Last-Modified: Wed, 28 Nov 2007 00:40:09 GMT\n"
        "Content-Length: 22\n"
        "Etag: \"foopy\"\n";
    request_ = MockHttpRequest(transaction);
  }

  ~WritersTest() override {
    if (disk_entry_)
      disk_entry_->Close();
  }

  void CreateWriters(const std::string& url) {
    cache_.CreateBackendEntry(kSimpleGET_Transaction.url, &disk_entry_,
                              nullptr);
    entry_ = std::make_unique<HttpCache::ActiveEntry>(disk_entry_);
    (static_cast<MockDiskEntry*>(disk_entry_))->AddRef();
    writers_ = std::make_unique<HttpCache::Writers>(&test_cache_, entry_.get());
  }

  std::unique_ptr<HttpTransaction> CreateNetworkTransaction() {
    std::unique_ptr<HttpTransaction> transaction;
    MockNetworkLayer* network_layer = cache_.network_layer();
    network_layer->CreateTransaction(DEFAULT_PRIORITY, &transaction);
    return transaction;
  }

  void CreateWritersAddTransaction(bool is_exclusive = false) {
    TestCompletionCallback callback;

    // Create and Start a mock network transaction.
    std::unique_ptr<HttpTransaction> network_transaction;
    network_transaction = CreateNetworkTransaction();
    network_transaction->Start(&request_, callback.callback(),
                               NetLogWithSource());
    base::RunLoop().RunUntilIdle();
    response_info_ = *(network_transaction->GetResponseInfo());

    // Create a mock cache transaction.
    std::unique_ptr<TestHttpCacheTransaction> transaction =
        std::make_unique<TestHttpCacheTransaction>(DEFAULT_PRIORITY,
                                                   cache_.http_cache());

    CreateWriters(kSimpleGET_Transaction.url);
    EXPECT_TRUE(writers_->IsEmpty());
    HttpCache::Writers::TransactionInfo info(transaction->partial(),
                                             transaction->is_truncated(),
                                             *(transaction->GetResponseInfo()));

    info.response_info = response_info_;
    writers_->AddTransaction(transaction.get(), is_exclusive,
                             transaction->priority(), info);
    writers_->SetNetworkTransaction(transaction.get(),
                                    std::move(network_transaction));
    EXPECT_TRUE(writers_->HasTransaction(transaction.get()));
    transactions_.push_back(std::move(transaction));
  }

  void CreateWritersAddTransactionPriority(net::RequestPriority priority,
                                           bool is_exclusive = false) {
    CreateWritersAddTransaction(is_exclusive);
    TestHttpCacheTransaction* transaction = transactions_.begin()->get();
    transaction->SetPriority(priority);
  }

  void AddTransactionToExistingWriters() {
    EXPECT_TRUE(writers_);

    // Create a mock cache transaction.
    std::unique_ptr<TestHttpCacheTransaction> transaction =
        std::make_unique<TestHttpCacheTransaction>(DEFAULT_PRIORITY,
                                                   cache_.http_cache());

    HttpCache::Writers::TransactionInfo info(transaction->partial(),
                                             transaction->is_truncated(),
                                             *(transaction->GetResponseInfo()));
    info.response_info = response_info_;
    writers_->AddTransaction(transaction.get(), false, transaction->priority(),
                             info);
    transactions_.push_back(std::move(transaction));
  }

  int Read(std::string* result) {
    EXPECT_TRUE(transactions_.size() >= (size_t)1);
    TestHttpCacheTransaction* transaction = transactions_.begin()->get();
    TestCompletionCallback callback;

    std::string content;
    int rv = 0;
    do {
      scoped_refptr<IOBuffer> buf(new IOBuffer(kDefaultBufferSize));
      rv = writers_->Read(buf.get(), kDefaultBufferSize, callback.callback(),
                          transaction);
      if (rv == ERR_IO_PENDING) {
        rv = callback.WaitForResult();
        base::RunLoop().RunUntilIdle();
      }

      if (rv > 0)
        content.append(buf->data(), rv);
      else if (rv < 0)
        return rv;
    } while (rv > 0);

    result->swap(content);
    return OK;
  }

  void ReadVerifyTwoDifferentBufferLengths(
      const std::vector<int>& buffer_lengths) {
    EXPECT_EQ(2u, buffer_lengths.size());
    EXPECT_EQ(2u, transactions_.size());

    std::vector<std::string> results(buffer_lengths.size());

    // Check only the 1st Read and not the complete response because the smaller
    // buffer transaction will need to read the remaining response from the
    // cache which will be tested when integrated with TestHttpCacheTransaction
    // layer.

    int rv = 0;

    std::vector<scoped_refptr<IOBuffer>> bufs;
    for (auto buffer_length : buffer_lengths)
      bufs.push_back(new IOBuffer(buffer_length));

    std::vector<TestCompletionCallback> callbacks(buffer_lengths.size());

    // Multiple transactions should be able to read with different sized
    // buffers.
    for (size_t i = 0; i < transactions_.size(); i++) {
      rv = writers_->Read(bufs[i].get(), buffer_lengths[i],
                          callbacks[i].callback(), transactions_[i].get());
      EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.
    }

    // If first buffer is smaller, then the second one will only read the
    // smaller length as well.
    std::vector<int> expected_lengths = {buffer_lengths[0],
                                         buffer_lengths[0] < buffer_lengths[1]
                                             ? buffer_lengths[0]
                                             : buffer_lengths[1]};

    for (size_t i = 0; i < callbacks.size(); i++) {
      rv = callbacks[i].WaitForResult();
      EXPECT_EQ(expected_lengths[i], rv);
      results[i].append(bufs[i]->data(), expected_lengths[i]);
    }

    EXPECT_EQ(results[0].substr(0, expected_lengths[1]), results[1]);

    std::string expected(kSimpleGET_Transaction.data);
    EXPECT_EQ(expected.substr(0, expected_lengths[1]), results[1]);
  }

  // Each transaction invokes Read simultaneously. If |deleteType| is not NONE,
  // then it deletes the transaction of given type during the read process.
  void ReadAllDeleteTransaction(DeleteTransactionType deleteType) {
    EXPECT_TRUE(transactions_.size() >= 3u);

    unsigned int delete_index = std::numeric_limits<unsigned int>::max();
    switch (deleteType) {
      case DeleteTransactionType::NONE:
        break;
      case DeleteTransactionType::ACTIVE:
        delete_index = 0;
        break;
      case DeleteTransactionType::WAITING:
        delete_index = 1;
        break;
      case DeleteTransactionType::IDLE:
        delete_index = 2;
        break;
    }

    std::vector<std::string> results(transactions_.size());
    int rv = 0;
    bool first_iter = true;
    do {
      std::vector<scoped_refptr<IOBuffer>> bufs;
      std::vector<TestCompletionCallback> callbacks(transactions_.size());

      for (size_t i = 0; i < transactions_.size(); i++) {
        bufs.push_back(new IOBuffer(kDefaultBufferSize));

        // If we have deleted a transaction in the first iteration, then do not
        // invoke Read on it, in subsequent iterations.
        if (!first_iter && deleteType != DeleteTransactionType::NONE &&
            i == delete_index)
          continue;

        // For it to be an idle transaction, do not invoke Read.
        if (deleteType == DeleteTransactionType::IDLE && i == delete_index)
          continue;

        rv = writers_->Read(bufs[i].get(), kDefaultBufferSize,
                            callbacks[i].callback(), transactions_[i].get());
        EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.
      }

      if (first_iter && deleteType != DeleteTransactionType::NONE) {
        writers_->RemoveTransaction(transactions_.at(delete_index).get(),
                                    false /* success */);
      }

      // Verify Add Transaction should succeed mid-read.
      AddTransactionToExistingWriters();

      std::vector<int> rvs;
      for (size_t i = 0; i < callbacks.size(); i++) {
        if (i == delete_index && deleteType != DeleteTransactionType::NONE)
          continue;
        rv = callbacks[i].WaitForResult();
        rvs.push_back(rv);
      }

      // Verify all transactions should read the same length buffer.
      for (size_t i = 1; i < rvs.size(); i++) {
        ASSERT_EQ(rvs[i - 1], rvs[i]);
      }

      if (rv > 0) {
        for (size_t i = 0; i < results.size(); i++) {
          if (i == delete_index && deleteType != DeleteTransactionType::NONE &&
              deleteType != DeleteTransactionType::ACTIVE) {
            continue;
          }
          results.at(i).append(bufs[i]->data(), rv);
        }
      }
      first_iter = false;
    } while (rv > 0);

    for (size_t i = 0; i < results.size(); i++) {
      if (i == delete_index && deleteType != DeleteTransactionType::NONE &&
          deleteType != DeleteTransactionType::ACTIVE) {
        continue;
      }
      EXPECT_EQ(kSimpleGET_Transaction.data, results[i]);
    }

    EXPECT_EQ(OK, rv);
  }

  void MidReadDeleteActiveTransaction() {
    EXPECT_TRUE(transactions_.size() == 1u);
    TestHttpCacheTransaction* transaction = transactions_[0].get();

    // Read a few bytes so that truncation is possible.
    TestCompletionCallback callback;
    scoped_refptr<IOBuffer> buf = new IOBuffer(5);
    int rv = writers_->Read(buf.get(), 5, callback.callback(), transaction);
    EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.

    writers_->RemoveTransaction(transaction, false /* success */);

    EXPECT_TRUE(writers_->IsEmpty());

    // Cannot add more writers while we are in truncation pending state.
    EXPECT_FALSE(writers_->CanAddWriters());

    // Complete the Read and the entry should be truncated.
    base::RunLoop().RunUntilIdle();
  }

  void MidReadStopCaching() {
    EXPECT_TRUE(transactions_.size() == 1u);
    TestHttpCacheTransaction* transaction = transactions_[0].get();

    // Read a few bytes so that truncation is possible.
    TestCompletionCallback callback;
    scoped_refptr<IOBuffer> buf = new IOBuffer(5);
    int rv = writers_->Read(buf.get(), 5, callback.callback(), transaction);
    EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.

    writers_->StopCaching(false /* keep_entry */);

    // Cannot add more writers while we are in network read only state.
    EXPECT_FALSE(writers_->CanAddWriters());

    // Complete the Read and the entry should be truncated.
    base::RunLoop().RunUntilIdle();
  }

  void ReadAll() { ReadAllDeleteTransaction(DeleteTransactionType::NONE); }

  int ReadCacheWriteFailure(std::vector<std::string>* results) {
    int rv = 0;
    int active_transaction_rv = 0;
    bool first_iter = true;
    do {
      std::vector<scoped_refptr<IOBuffer>> bufs;
      std::vector<TestCompletionCallback> callbacks(results->size());

      // Fail the request.
      cache_.disk_cache()->set_soft_failures(true);

      // We have to open the entry again to propagate the failure flag.
      disk_cache::Entry* en;
      cache_.OpenBackendEntry(kSimpleGET_Transaction.url, &en);
      en->Close();

      for (size_t i = 0; i < transactions_.size(); i++) {
        bufs.push_back(new IOBuffer(30));

        if (!first_iter && i > 0)
          break;
        rv = writers_->Read(bufs[i].get(), 30, callbacks[i].callback(),
                            transactions_[i].get());
        EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.
      }

      for (size_t i = 0; i < callbacks.size(); i++) {
        // Only active transaction should succeed.
        if (i == 0) {
          active_transaction_rv = callbacks[i].WaitForResult();
          EXPECT_TRUE(active_transaction_rv >= 0);
          results->at(0).append(bufs[i]->data(), active_transaction_rv);
        } else if (first_iter) {
          rv = callbacks[i].WaitForResult();
          EXPECT_EQ(ERR_CACHE_WRITE_FAILURE, rv);
        }
      }

      first_iter = false;
    } while (active_transaction_rv > 0);

    return active_transaction_rv;
  }

  int ReadNetworkFailure(std::vector<std::string>* results, Error error) {
    int rv = 0;
    std::vector<scoped_refptr<IOBuffer>> bufs;
    std::vector<TestCompletionCallback> callbacks(results->size());

    for (size_t i = 0; i < transactions_.size(); i++) {
      bufs.push_back(new IOBuffer(30));

      rv = writers_->Read(bufs[i].get(), 30, callbacks[i].callback(),
                          transactions_[i].get());
      EXPECT_EQ(ERR_IO_PENDING, rv);  // Since the default is asynchronous.
    }

    for (auto& callback : callbacks) {
      rv = callback.WaitForResult();
      EXPECT_EQ(error, rv);
    }

    return error;
  }

  bool StopCaching() {
    TestHttpCacheTransaction* transaction = transactions_.begin()->get();
    EXPECT_TRUE(transaction);
    return writers_->StopCaching(transaction);
  }

  void RemoveFirstTransaction() {
    TestHttpCacheTransaction* transaction = transactions_.begin()->get();
    EXPECT_TRUE(transaction);
    writers_->RemoveTransaction(transaction, false /* success */);
  }

  void UpdateAndVerifyPriority(RequestPriority priority) {
    writers_->UpdatePriority();
    EXPECT_EQ(priority, writers_->priority_);
  }

  bool ShouldKeepEntry() { return writers_->should_keep_entry_; }

  void TruncateEntryNoStrongValidators() {
    writers_->InitiateTruncateEntry();
    EXPECT_FALSE(ShouldKeepEntry());
  }

  MockHttpCache cache_;
  std::unique_ptr<HttpCache::Writers> writers_;
  disk_cache::Entry* disk_entry_;
  std::unique_ptr<HttpCache::ActiveEntry> entry_;
  TestHttpCache test_cache_;

  // Should be before transactions_ since it is accessed in the network
  // transaction's destructor.
  MockHttpRequest request_;

  HttpResponseInfo response_info_;
  static const int kDefaultBufferSize = 256;

  std::vector<std::unique_ptr<TestHttpCacheTransaction>> transactions_;
};

// Tests successful addition of a transaction.
TEST_F(WritersTest, AddTransaction) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  // Verify keep_entry_ is true by default.
  EXPECT_TRUE(ShouldKeepEntry());
}

// Tests successful addition of multiple transactions.
TEST_F(WritersTest, AddManyTransactions) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  for (int i = 0; i < 5; i++)
    AddTransactionToExistingWriters();

  EXPECT_EQ(6, writers_->GetTransactionsCount());
}

// Tests that CanAddWriters should return false if it is writing exclusively.
TEST_F(WritersTest, AddTransactionsExclusive) {
  CreateWritersAddTransaction(true /* is_exclusive */);
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_FALSE(writers_->CanAddWriters());
}

// Tests StopCaching should not stop caching if there are multiple writers.
TEST_F(WritersTest, StopCachingMultipleWriters) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();

  EXPECT_FALSE(StopCaching());
  EXPECT_TRUE(writers_->CanAddWriters());
}

// Tests StopCaching should stop caching if there is a single writer.
TEST_F(WritersTest, StopCaching) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(StopCaching());
  EXPECT_FALSE(writers_->CanAddWriters());
}

// Tests StopCaching should be successful when invoked mid-read.
TEST_F(WritersTest, StopCachingMidRead) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());
  MidReadStopCaching();
}

// Tests removing of an idle transaction and change in priority.
TEST_F(WritersTest, RemoveIdleTransaction) {
  CreateWritersAddTransactionPriority(HIGHEST);
  UpdateAndVerifyPriority(HIGHEST);

  AddTransactionToExistingWriters();
  UpdateAndVerifyPriority(HIGHEST);

  EXPECT_FALSE(writers_->IsEmpty());
  EXPECT_EQ(2, writers_->GetTransactionsCount());

  RemoveFirstTransaction();
  EXPECT_EQ(1, writers_->GetTransactionsCount());

  UpdateAndVerifyPriority(DEFAULT_PRIORITY);
}

// Tests that Read is successful.
TEST_F(WritersTest, Read) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  std::string content;
  int rv = Read(&content);

  EXPECT_THAT(rv, IsOk());
  std::string expected(kSimpleGET_Transaction.data);
  EXPECT_EQ(expected, content);
}

// Tests that multiple transactions can read the same data simultaneously.
TEST_F(WritersTest, ReadMultiple) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  ReadAll();
}

// Tests that multiple transactions can read the same data simultaneously.
TEST_F(WritersTest, ReadMultipleDifferentBufferSizes) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();

  std::vector<int> buffer_lengths{20, 10};
  ReadVerifyTwoDifferentBufferLengths(buffer_lengths);
}

// Same as above but tests the first transaction having smaller buffer size
// than the next.
TEST_F(WritersTest, ReadMultipleDifferentBufferSizes1) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();

  std::vector<int> buffer_lengths{10, 20};
  ReadVerifyTwoDifferentBufferLengths(buffer_lengths);
}

// Tests that ongoing Read completes even when active transaction is deleted
// mid-read. Any transactions waiting should be able to get the read buffer.
TEST_F(WritersTest, ReadMultipleDeleteActiveTransaction) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  ReadAllDeleteTransaction(DeleteTransactionType::ACTIVE);
}

// Tests that ongoing Read completes even when active transaction is deleted
// mid-read and there are no more transactions. It should also successfully
// initiate truncation of the entry.
TEST_F(WritersTest, MidReadDeleteActiveTransaction) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());
  EXPECT_TRUE(writers_->CanAddWriters());
  MidReadDeleteActiveTransaction();
}

// Tests that removing a waiting for read transaction does not impact other
// transactions.
TEST_F(WritersTest, ReadMultipleDeleteWaitingTransaction) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  std::vector<std::string> contents(4);
  ReadAllDeleteTransaction(DeleteTransactionType::WAITING);
}

// Tests that removing an idle transaction does not impact other transactions.
TEST_F(WritersTest, ReadMultipleDeleteIdleTransaction) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  std::vector<std::string> contents(3);
  ReadAllDeleteTransaction(DeleteTransactionType::IDLE);
}

// Tests cache write failure.
TEST_F(WritersTest, ReadMultipleCacheWriteFailed) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  std::vector<std::string> contents(3);
  int rv = ReadCacheWriteFailure(&contents);

  EXPECT_THAT(rv, IsOk());
  std::string expected(kSimpleGET_Transaction.data);

  // Only active_transaction_ should succeed.
  EXPECT_EQ(expected, contents.at(0));
}

// Tests that network read failure fails all transactions: active, waiting and
// idle.
TEST_F(WritersTest, ReadMultipleNetworkReadFailed) {
  ScopedMockTransaction transaction(kSimpleGET_Transaction);
  transaction.read_return_code = ERR_INTERNET_DISCONNECTED;
  MockHttpRequest request(transaction);
  request_ = request;

  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_TRUE(writers_->CanAddWriters());
  AddTransactionToExistingWriters();
  AddTransactionToExistingWriters();

  std::vector<std::string> contents(3);
  int rv = ReadNetworkFailure(&contents, ERR_INTERNET_DISCONNECTED);

  EXPECT_EQ(ERR_INTERNET_DISCONNECTED, rv);
}

// Tests GetLoadState.
TEST_F(WritersTest, GetLoadState) {
  CreateWritersAddTransaction();
  EXPECT_FALSE(writers_->IsEmpty());

  EXPECT_EQ(LOAD_STATE_IDLE, writers_->GetLoadState());
}

// Tests truncating logic.
TEST_F(WritersTest, TruncateEntryFail) {
  CreateWritersAddTransaction();

  EXPECT_FALSE(writers_->IsEmpty());

  RemoveFirstTransaction();

  // Should return false since the entry does not have strong validators and
  // thus cannot be resumed.
  TruncateEntryNoStrongValidators();
}

// Set network read only.
TEST_F(WritersTest, StopCachingWithKeepEntry) {
  CreateWritersAddTransaction(true /* is exclusive */);
  EXPECT_FALSE(writers_->network_read_only());

  writers_->StopCaching(true /* keep_entry */);
  EXPECT_TRUE(writers_->network_read_only());
  EXPECT_TRUE(ShouldKeepEntry());
}

TEST_F(WritersTest, StopCachingWithNotKeepEntry) {
  CreateWritersAddTransaction(true /* is exclusive */);
  EXPECT_FALSE(writers_->network_read_only());

  writers_->StopCaching(false /* keep_entry */);
  EXPECT_TRUE(writers_->network_read_only());
  EXPECT_FALSE(ShouldKeepEntry());
}

}  // namespace net
