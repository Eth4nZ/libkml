// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains the unit tests for the Referent class and the
// functions used by boost::intrusive_ptr.

#include "kml/dom/referent.h"
#include <vector>
#include "boost/intrusive_ptr.hpp"
#include "kml/util/unit_test.h"

namespace kmldom {

class ReferentTest : public CPPUNIT_NS::TestFixture {
  CPPUNIT_TEST_SUITE(ReferentTest);
  CPPUNIT_TEST(TestReferent);
  CPPUNIT_TEST(TestIntrusivePointerFunctions);
  CPPUNIT_TEST(TestDelete);
  CPPUNIT_TEST(TestVector);
  CPPUNIT_TEST(TestFunc);
  CPPUNIT_TEST(TestBasicParentChild);
  CPPUNIT_TEST(TestGetChild);
  CPPUNIT_TEST_SUITE_END();

 public:
  // Called before each test.
  void setUp() {
    derived_ = new Derived;
  }

  // Called after each test.
  void tearDown() {
    // The derived_ member is a smart pointer which deletes the underlying
    // object when this ReferentTest instance is destroyed.
  }

 protected:
  void TestReferent();
  void TestIntrusivePointerFunctions();
  void TestDelete();
  void TestVector();
  void TestFunc();
  void TestBasicParentChild();
  void TestGetChild();

 private:
  // This forward declaration is for the benefit of the typedef.
  class Derived;
  typedef boost::intrusive_ptr<Derived> DerivedPtr;

  // This is a test class for excercising expected usage scenarios of the
  // Referent class.
  class Derived : public Referent {
   public:
    void set_child(DerivedPtr child) {
      child_ = child;
    }
    void clear_child() {
      child_ = NULL;
    }
    DerivedPtr child() const {
      return child_;
    }
    // This method avoids returning child_ by value for tests that just
    // wish to verify the ref count of the child.  The act of calling
    // child() introduces yet another value and reference.
    // This is valid to call only if Derived has a child.
    int child_ref_count() const {
      return child_->ref_count();
    }

   private:
    DerivedPtr child_;
  };

  DerivedPtr derived_;
  void FuncByValue(DerivedPtr d, int expected_ref_count);
  void FuncByReference(DerivedPtr& d, int expected_ref_count);
};

CPPUNIT_TEST_SUITE_REGISTRATION(ReferentTest);

// This test the methods of the Referent class itself.
void ReferentTest::TestReferent() {
  Referent referent;
  // Without intrusive_ptr nothing increases the ref count.
  // This verifies the initial state of the Referent class.
  CPPUNIT_ASSERT_EQUAL(0, referent.ref_count());

  // The add_ref() method increments the reference count.
  referent.add_ref();
  CPPUNIT_ASSERT_EQUAL(1, referent.ref_count());
  referent.add_ref();
  CPPUNIT_ASSERT_EQUAL(2, referent.ref_count());

  // The release method decrements the reference count.
  referent.release();
  CPPUNIT_ASSERT_EQUAL(1, referent.ref_count());
}

// This tests the intrusive_ptr_add_ref() and intrusive_ptr_release()
// functions used by boost::intrusive_ptr.
void ReferentTest::TestIntrusivePointerFunctions() {
  Referent* referent = new Referent;
  CPPUNIT_ASSERT_EQUAL(0, referent->ref_count());
  intrusive_ptr_add_ref(referent);
  CPPUNIT_ASSERT_EQUAL(1, referent->ref_count());
  intrusive_ptr_add_ref(referent);
  CPPUNIT_ASSERT_EQUAL(2, referent->ref_count());

  intrusive_ptr_release(referent);
  CPPUNIT_ASSERT_EQUAL(1, referent->ref_count());
  // This deletes referent:
  intrusive_ptr_release(referent);
}

void ReferentTest::TestDelete() {
  // setUp created on instance of Derived.
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());

  {
    // Copy the pointer and verify that both see the same underlying object and
    // use count.
    DerivedPtr copy = derived_;
    CPPUNIT_ASSERT_EQUAL(copy.get(), derived_.get());
    CPPUNIT_ASSERT_EQUAL(2, derived_->ref_count());
    CPPUNIT_ASSERT_EQUAL(2, copy->ref_count());
  }

  // copy is now out of scope so use count is back to 1
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
}

void ReferentTest::TestVector() {
  const int kCount = 101;
  {
    std::vector<DerivedPtr> derived_vec;
    int n = kCount;
    while (n--) {
      // STL vector makes a copy of the smart pointer hence bumps ref count.
      derived_vec.push_back(derived_);
      CPPUNIT_ASSERT_EQUAL(kCount - n + 1, derived_->ref_count());
    }
  // End of scope of vector deletes all vector members.
  }
  // derived_vec is now out of scope so use count is back to 1
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
}

// Helper function for TestFunc().  This verifies that passing a DerivedPtr
// by value increases the reference count.
void ReferentTest::FuncByValue(DerivedPtr d, int expected_ref_count) {
  CPPUNIT_ASSERT_EQUAL(expected_ref_count, derived_->ref_count());
  CPPUNIT_ASSERT_EQUAL(d.get(), derived_.get());
}

// Helper function for TestFunc().  This verifies that passing a DerivedPtr
// by reference does not increase the reference count.
void ReferentTest::FuncByReference(DerivedPtr& d, int expected_ref_count) {
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
  CPPUNIT_ASSERT_EQUAL(d.get(), derived_.get());
}

// This verifies Referent's reference count is proper for pass by value
// and pass by reference.
void ReferentTest::TestFunc() {
  // Verify initial conditions:
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
  // Pass a boost::intrusive_ptr by value thus increasing the reference count.
  FuncByValue(derived_, 2);
  // Pass by value now out of scope to reference count back to where it was:
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
  // Pass by reference does not increase the reference count.
  FuncByReference(derived_, 1);
}

// This test verifies that giving a given child object to a parent increments
// the reference count.
void ReferentTest::TestBasicParentChild() {
  Derived* bare_pointer;
  {
    DerivedPtr child = new Derived;
    bare_pointer = child.get();

    // Verify that adding this child to the parent increments the reference
    // count.
    derived_->set_child(child);
    CPPUNIT_ASSERT_EQUAL(2, child->ref_count());

    // Verify that deleting this child from the parent decrements the reference
    // count.
    derived_->clear_child();
    CPPUNIT_ASSERT_EQUAL(1, child->ref_count());

    // Set it to the the parent again.
    derived_->set_child(child);
    CPPUNIT_ASSERT_EQUAL(2, child->ref_count());
    // End scope for child.
  }

  // The parent now owns the only reference.
  CPPUNIT_ASSERT_EQUAL(bare_pointer, derived_->child().get());
  CPPUNIT_ASSERT_EQUAL(1, derived_->child_ref_count());
}

// This verifies that holding a reference to a child of a given parent is still
// valid even after the parent is destroyed.
void ReferentTest::TestGetChild() {
  // Verify initial conditions: test fixture is only owner of object.
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
  // Introduce a block to own child.
  {
    DerivedPtr child;
    // Introduce a block to own parent.
    {
      DerivedPtr parent = new Derived;
      parent->set_child(derived_);
      child = parent->child();
      CPPUNIT_ASSERT_EQUAL(derived_, parent->child());
      // ReferentTest derived_, child, and child_ within parent.
      CPPUNIT_ASSERT_EQUAL(3, parent->child_ref_count());
      // parent now goes out of scope and is destroyed releasing its reference
      // to the child.
    }
    // child is now the only user of the object.
    CPPUNIT_ASSERT_EQUAL(2, child->ref_count());
  }
  // Only the test fixture refers to the object.
  CPPUNIT_ASSERT_EQUAL(1, derived_->ref_count());
  // The object is released when child goes out of scope.
}

}  // end namespace kmldom

TEST_MAIN