#include "test_object.h"
#include "test_helpers.h"

static const char config_string[] = "aString = \"hello world\";\n"
                                    "anObject {\n"
                                    "  aString = \"Inner string\";\n"
                                    "  anInt = 42;\n"
                                    "}\n";

static const char config_wrong[] = "aString = \"hello world\";\n"
                                   "anObject {\n"
                                   "  aString = 12;\n"
                                   "  anInt = 42;\n"
                                   "}\n";

int main()
{
	auto obj  = parse(config_string, sizeof(config_string));
	auto conf = getConfig(obj);
	assert(conf.aString() == "hello world");
	assert(conf.anObject().aString() == "Inner string");
	assert(conf.anObject().anInt() == 42);
	checkInvalidConfig(parse(config_wrong, sizeof(config_wrong)));
	return EXIT_SUCCESS;
}
