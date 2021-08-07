#include "test_type.h"
#include "test_helpers.h"

static const char config_string[] = "aString = \"hello world\";\n"
                                    "i8 = -12\n"
                                    "u8 = 12\n"
                                    "anInt = 42\n"
                                    "aDouble = 42.5\n"
                                    "aBool = true\n";

static const char config_wrong[] = "aString = \"hello world\";\n"
                                   "i8 = -22\n"
                                   "u8 = 12\n"
                                   "anInt = 42\n"
                                   "aDouble = 42.5\n"
                                   "aBool = true\n";

int main()
{
	auto obj  = parse(config_string, sizeof(config_string));
	auto conf = getConfig(obj);
	assert(conf.aString() == "hello world");
	assert(conf.anInt() == 42);
	assert(conf.i8().value_or(0) == -12);
	assert(conf.u8().value_or(0) == 12);
	assert(conf.aDouble().value_or(0) == 42.5);
	assert(conf.aBool().value_or(false));
	checkInvalidConfig(parse(config_wrong, sizeof(config_wrong)));
	return EXIT_SUCCESS;
}
