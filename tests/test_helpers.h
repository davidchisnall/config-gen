#include <cassert>
#include <iostream>

ucl_object_t *parse(const char *str, size_t len)
{
	struct ucl_parser *p = ucl_parser_new(UCL_PARSER_NO_IMPLICIT_ARRAYS);
	ucl_parser_add_string(p, str, len);
	if (ucl_parser_get_error(p))
	{
		std::cerr << "Parse error: " << ucl_parser_get_error(p) << std::endl;
		exit(EXIT_FAILURE);
	}
	return ucl_parser_get_object(p);
}

Config getConfig(ucl_object_t *obj)
{
	auto confOrError = make_config(obj);
	if (std::holds_alternative<ucl_schema_error>(confOrError))
	{
		auto &err = std::get<ucl_schema_error>(confOrError);
		std::cerr << "Schema validation failed " << err.msg << std::endl
		          << (char *)ucl_object_emit(err.obj, UCL_EMIT_CONFIG)
		          << std::endl;
	}
	assert(std::holds_alternative<Config>(confOrError));
	return get<Config>(confOrError);
}

void checkInvalidConfig(ucl_object_t *obj)
{
	auto confOrError = make_config(obj);
	assert(std::holds_alternative<ucl_schema_error>(make_config(obj)));
}
