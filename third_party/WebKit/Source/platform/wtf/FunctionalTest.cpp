/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/wtf/Functional.h"

#include <memory>
#include <utility>
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/WTFTestHelper.h"
#include "platform/wtf/WeakPtr.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {

class UnwrappedClass {
 public:
  explicit UnwrappedClass(int value) : value_(value) {}

  int Value() const { return value_; }

 private:
  int value_;
};

class HasWeakPtrSupport {
 public:
  HasWeakPtrSupport() : weak_ptr_factory_(this) {}

  WTF::WeakPtr<HasWeakPtrSupport> CreateWeakPtr() {
    return weak_ptr_factory_.CreateWeakPtr();
  }

  void RevokeAll() { weak_ptr_factory_.RevokeAll(); }

  void Increment(int* counter) { ++*counter; }

 private:
  WTF::WeakPtrFactory<HasWeakPtrSupport> weak_ptr_factory_;
};

}  // namespace WTF

namespace WTF {
namespace {

class A {
 public:
  explicit A(int i) : i_(i) {}
  virtual ~A() {}

  int F() { return i_; }
  int AddF(int j) { return i_ + j; }
  virtual int Overridden() { return 42; }

 private:
  int i_;
};

class B : public A {
 public:
  explicit B(int i) : A(i) {}

  int F() { return A::F() + 1; }
  int AddF(int j) { return A::AddF(j) + 1; }
  int Overridden() override { return 43; }
};

class Number {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();
  static scoped_refptr<Number> Create(int value) {
    return base::AdoptRef(new Number(value));
  }

  ~Number() { value_ = 0; }

  int Value() const { return value_; }

  int RefCount() const { return ref_count_; }
  bool HasOneRef() const { return ref_count_ == 1; }

  // For RefPtr support.
  void Adopted() const {}
  void AddRef() const { ++ref_count_; }
  void Release() const {
    if (--ref_count_ == 0)
      delete this;
  }

 private:
  explicit Number(int value) : value_(value) {}

  int value_;
  mutable int ref_count_ = 1;
};

class CountGeneration {
 public:
  CountGeneration() : copies_(0) {}
  CountGeneration(const CountGeneration& other) : copies_(other.copies_ + 1) {}

  int Copies() const { return copies_; }

 private:
  // Copy/move-assignment is not needed in the test.
  CountGeneration& operator=(const CountGeneration&) = delete;
  CountGeneration& operator=(CountGeneration&&) = delete;

  int copies_;
};

TEST(FunctionalTest, WeakPtr) {
  HasWeakPtrSupport obj;
  int counter = 0;
  WTF::RepeatingClosure bound =
      WTF::Bind(&HasWeakPtrSupport::Increment, obj.CreateWeakPtr(),
                WTF::Unretained(&counter));

  bound.Run();
  EXPECT_FALSE(bound.IsCancelled());
  EXPECT_EQ(1, counter);

  obj.RevokeAll();
  EXPECT_TRUE(bound.IsCancelled());
  bound.Run();
  EXPECT_EQ(1, counter);
}

}  // anonymous namespace

}  // namespace WTF
