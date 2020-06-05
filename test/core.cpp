#include <CppUTest/TestHarness.h>

extern "C" {
   #include <atum.h>
}

TEST_GROUP(InjectorGroup)
{
};

TEST(InjectorGroup, InjectFailsWithNullLibrary)
{
   CHECK(inject_library((pid_t) 123, NULL) == ATUM_FAILURE);
}
