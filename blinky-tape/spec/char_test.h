#ifndef CharTest_h
#define CharTest_h

#include <cppunit/extensions/HelperMacros.h>
#include "../src/char.h"

class CharTest : public CPPUNIT_NS::TestFixture {
  CPPUNIT_TEST_SUITE( CharTest );
  CPPUNIT_TEST( test_constructor );
  CPPUNIT_TEST( test_char_is_last_when_last );
  CPPUNIT_TEST( test_char_is_last_when_not_last );
  CPPUNIT_TEST( test_get_next_when_not_last );
  CPPUNIT_TEST_SUITE_END();

public:
  Char *character;
  Char *next_character;

  void setUp() {
    character = new Char('h');
    next_character = new Char('n');
    character->set_next(next_character);
  }

  void tearDown() {
    delete character;
    delete next_character;
  }

  void test_constructor() {
    CPPUNIT_ASSERT( character->get_character() == 'h' );
    CPPUNIT_ASSERT( next_character->get_character() == 'n' );
  }

  void test_char_is_last_when_not_last() {
    CPPUNIT_ASSERT( !character->is_last() );
  }

  void test_char_is_last_when_last() {
    CPPUNIT_ASSERT( next_character->is_last() );
  }

  void test_get_next_when_not_last() {
    CPPUNIT_ASSERT(character->get_next()->get_character() == 'n');
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CharTest );

#endif
