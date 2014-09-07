#include <cstdlib>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "char_test.h"
#include "char_queue_test.h"

int main(int argc, char* argv[]) {
  CPPUNIT_NS::Test *suite = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;
  runner.addTest(suite);
  bool wasSucessful = runner.run();
  return wasSucessful ? 0 : 1;
}
