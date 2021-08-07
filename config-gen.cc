// Copyright David Chisnall
// SPDX-License-Identifier: MIT
#include "config-generic.h"
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <unordered_set>

using namespace config;
using namespace config::detail;

namespace Schema
{
	struct Object;
	struct Array;
	struct String;
	struct Integer;
	struct Boolean;
	struct Number;

	/**
	 * Base class for parts of a JSON Schema.
	 */
	class SchemaBase
	{
		protected:
		/**
		 * The UCL object that this represents.
		 */
		UCLPtr obj;

		public:
		/**
		 * Constructor, captures an owning reference to a UCL object.
		 */
		SchemaBase(const ucl_object_t *o) : obj(o) {}

		/**
		 * Type adaptor, allows dispatching to a visitor with overloads to all
		 * of the basic types of schema depending on the value of the `type`
		 * property.
		 */
		using TypeAdaptor = NamedTypeAdaptor<"type",
		                                     NamedType<"object", Object>,
		                                     NamedType<"array", Array>,
		                                     NamedType<"string", String>,
		                                     NamedType<"integer", Integer>,
		                                     NamedType<"boolean", Boolean>,
		                                     NamedType<"number", Number>>;

		/**
		 * Returns a type adaptor for this object that can be used to dispatch
		 * based on the value of the `type` field.
		 */
		TypeAdaptor get()
		{
			return TypeAdaptor(obj);
		}

		/**
		 * Types of JSON sub-schema.
		 */
		enum Type
		{
			/**
			 * A JSON object.
			 */
			TypeObject,
			/**
			 * A JSON string.
			 */
			TypeString,
			/**
			 * A JSON array.
			 */
			TypeArray,
			/**
			 * A JSON number.
			 */
			TypeNumber,
			/**
			 * An integer.  This is shorthand for a number with the constraint
			 * that it must increment in units of 1.
			 */
			TypeInteger,
			/**
			 * A JSON boolean.
			 */
			TypeBool,
		};

		/**
		 * Enum adaptor type, maps from a string value from a JSON schema to a
		 * value in the `Type` `enum`.
		 */
		using TypeEnumAdaptor =
		  EnumAdaptor<Type,
		              EnumValueMap<Enum{"object", TypeObject},
		                           Enum{"array", TypeArray},
		                           Enum{"string", TypeString},
		                           Enum{"integer", TypeInteger},
		                           Enum{"boolean", TypeBool},
		                           Enum{"number", TypeNumber}>>;

		/**
		 * Returns the type of this schema.
		 */
		Type type()
		{
			return TypeEnumAdaptor(obj["type"]);
		}

		/**
		 * Returns the title of this schema.
		 */
		std::string_view title()
		{
			return StringViewAdaptor(obj["title"]);
		}

		/**
		 * Returns the description of this schame.
		 */
		std::optional<std::string_view> description()
		{
			return make_optional<StringViewAdaptor>(obj["description"]);
		}
	};

	/**
	 * Array.  Represents a JSON schema array.  This defines a field `items`
	 * that describes elements of the array.
	 *
	 * *Note*: We currently support only a single element in the items
	 * property, describing the type of all items.
	 */
	struct Array : public SchemaBase
	{
		using SchemaBase::SchemaBase;

		/**
		 * The schema for the items of this array.
		 */
		SchemaBase items()
		{
			// FIXME: This only handles arrays that are arrays.
			// For config files, this is probably fine because arrays that are
			// tuples are better represented as objects.
			return SchemaBase(obj["items"]);
		}
	};

	/**
	 * A JSON schema string.  This is a trivial type.
	 */
	struct String : public SchemaBase
	{
		using SchemaBase::SchemaBase;
	};

	/**
	 * A JSON schema number.  This can define an allowed range, and a step size.
	 */
	struct Number : public SchemaBase
	{
		using SchemaBase::SchemaBase;

		/**
		 * The minimum value.  Valid numbers are >= this value.
		 */
		std::optional<double> minimum()
		{
			return make_optional<DoubleAdaptor>(obj["minimum"]);
		}

		/**
		 * The exclusive minimum value.  Valid numbers are > this value.
		 */
		std::optional<double> exclusiveMinimum()
		{
			return make_optional<DoubleAdaptor>(obj["exclusiveMinimum"]);
		}

		/**
		 * The maximum value.  Valid numbers are <= this value.
		 */
		std::optional<double> maximum()
		{
			return make_optional<DoubleAdaptor>(obj["maximum"]);
		}

		/**
		 * The exclusive maximum value.  Valid numbers are < this value.
		 */
		std::optional<double> exclusiveMaximum()
		{
			return make_optional<DoubleAdaptor>(obj["exclusiveMaximum"]);
		}

		/**
		 * The step size.  A valid value % this value == 0.
		 */
		std::optional<double> multipleOf()
		{
			return make_optional<DoubleAdaptor>(obj["multipleOf"]);
		}
	};

	/**
	 * Integer, a kind of number.
	 */
	struct Integer : public Number
	{
		using Number::Number;
	};

	/**
	 * Boolean, a trivial type in JSON schema.
	 */
	struct Boolean : public SchemaBase
	{
		using SchemaBase::SchemaBase;
	};

	/**
	 * A JSON Schema object, contains a set of properties some of which may be
	 * required, some optional.
	 */
	struct Object : public SchemaBase
	{
		public:
		using SchemaBase::SchemaBase;

		/**
		 * The type for the properties.  This provides an iterable range of
		 * key-value pairs mapping from name to property.
		 */
		using Properties =
		  Range<PropertyAdaptor<SchemaBase>, PropertyAdaptor<SchemaBase>, true>;

		/**
		 * The properties of this object.
		 */
		Properties properties()
		{
			return Properties(obj["properties"]);
		}

		/**
		 * The names of any properties that are required.  Properties not
		 * specified by this collection are optional.
		 */
		std::optional<Range<std::string_view, StringViewAdaptor>> required()
		{
			return make_optional<Range<std::string_view, StringViewAdaptor>>(
			  obj["required"]);
		}
	};

	/**
	 * The root of a schema.  This is an object that also defines a schema and a
	 * unique id.
	 */
	class Root : public Object
	{
		public:
		/**
		 * Constructor, takes an owning reference to a UCL object.
		 */
		Root(ucl_object_t *o) : Object(o) {}

		/**
		 * The schema property.  Should match the JSON Schema schema
		 */
		std::string_view schema()
		{
			return StringViewAdaptor(obj["$schema"]);
		}

		/**
		 * The id property.
		 */
		std::string_view id()
		{
			return StringViewAdaptor(obj["$id"]);
		}
	};
} // namespace Schema

using namespace Schema;

namespace
{
	/**
	 * The namespace for the helpers defined in `config-generic.h`.  This can
	 * be overridden on the command line.
	 */
	const char *configNamespace = "::config::detail::";

	template<typename T>
	void emit_class(Object o, std::string_view name, T &out);


	/**
	 * Schema visitor.  This visits a schema and collects the information
	 * required to provide the accessor for the described type.
	 */
	class SchemaVisitor
	{
		/**
		 * The name of a class emitted by this schema.  This exists because all
		 * of the other properties are non-owning string views and sometimes
		 * they need an owned string to reference.
		 */
		std::string        className;

		public:
		/**
		 * The return type for the accessor for this schema.
		 */
		std::string_view   return_type;

		/**
		 * The adaptor type to use for this schema.
		 */
		std::string_view   adaptor;

		/**
		 * The namespace in which the adaptor is defined. 
		 */
		std::string_view   adaptorNamespace = configNamespace;

		/**
		 * The lifetime attribute for this property, if one is required.  For
		 * strings, the returned `string_view` has a lifetime bound by the
		 * underlying schema.  Other types are typically either owning
		 * references or value types.
		 */
		std::string_view   lifetimeAttribute;

		/**
		 * The name of this property.
		 */
		std::string_view   name;

		/**
		 * Any new types that were declared to handle this property.
		 */
		std::stringstream &types;

		/**
		 * Construct a schema visitor with a specified name, writing to an
		 * output string stream.
		 */
		SchemaVisitor(std::string_view n, std::stringstream &t)
		  : name(n), types(t)
		{
		}

		/**
		 * Handle a number.  This is common code for all of the number
		 * subclasses.  It provides an adaptor that is the smallest type that
		 * satisfies all of the constraints.
		 */
		void handleNumber(Number &num, bool isInteger)
		{
			if (!isInteger)
			{
				auto multipleOf = num.multipleOf();
				if (multipleOf)
				{
					isInteger =
					  (((double)(uint64_t)*multipleOf) == *multipleOf);
				}
			}
			if (!isInteger)
			{
				return_type = "double";
				adaptor     = "DoubleAdaptor";
				return;
			}
			int64_t min = std::numeric_limits<int64_t>::min();
			int64_t max = std::numeric_limits<int64_t>::max();
			min =
			  std::max(min, static_cast<int64_t>(num.minimum().value_or(min)));
			min = std::max(
			  min, static_cast<int64_t>(num.exclusiveMinimum().value_or(min)));
			max =
			  std::min(max, static_cast<int64_t>(num.maximum().value_or(max)));
			max = std::min(
			  max, static_cast<int64_t>(num.exclusiveMaximum().value_or(max)));
			auto try_type =
			  [&](auto intty, std::string_view ty, std::string_view a) {
				  if ((min >= std::numeric_limits<decltype(intty)>::min()) &&
				      (max <= std::numeric_limits<decltype(intty)>::max()))
				  {
					  return_type = ty;
					  adaptor     = a;
				  }
			  };
			try_type(int64_t(), "int64_t", "Int64Adaptor");
			try_type(uint64_t(), "uint64_t", "UInt64Adaptor");
			try_type(int32_t(), "int32_t", "Int32Adaptor");
			try_type(uint32_t(), "uint32_t", "UInt32Adaptor");
			try_type(int16_t(), "int16_t", "Int16Adaptor");
			try_type(uint16_t(), "uint16_t", "UInt16Adaptor");
			try_type(int8_t(), "int8_t", "Int8Adaptor");
			try_type(uint8_t(), "uint8_t", "UInt8Adaptor");
		}

		/**
		 * Handle a string schema.
		 */
		void operator()(String)
		{
			return_type       = "std::string_view";
			adaptor           = "StringViewAdaptor";
			lifetimeAttribute = "CONFIG_LIFETIME_BOUND";
		}

		/**
		 * Handle a boolean schema.
		 */
		void operator()(Boolean)
		{
			return_type = "bool";
			adaptor     = "BoolAdaptor";
		}

		/**
		 * Handle an integer schema.
		 */
		void operator()(Integer i)
		{
			handleNumber(i, true);
		}

		/**
		 * Handle an integer schema.
		 */
		void operator()(Number n)
		{
			handleNumber(n, false);
		}

		/**
		 * Handle an object schema.  This does a recursive visit to generate a
		 * new class that represents the object.
		 */
		void operator()(Object o)
		{
			className = name;
			className += "Class";
			emit_class(o, className, types);
			return_type      = className;
			adaptor          = className;
			adaptorNamespace = "";
		}

		/**
		 * Handle an array.  This performs a recursive visit to generate a new
		 * class representing the array element type.
		 *
		 * Note that this currently handles only arrays of a single object
		 * type, not heterogeneous arrays.
		 */
		void operator()(Array a)
		{
			std::string itemName{name};
			itemName += "Item";
			SchemaVisitor item(itemName, types);
			auto          items = a.items();
			items.get().visit(item);
			className = configNamespace;
			className += "::Range<";
			className += item.return_type;
			className += ", ";
			className += item.adaptor;
			className += ", true>";
			return_type      = className;
			adaptor          = className;
			adaptorNamespace = "";
		}
	};

	/**
	 * Emit a class.  The class is defined by the object schema `o` and should
	 * have the name given by the `name` argument.  It will be written to the
	 * `out` stream.
	 */
	template<typename T>
	void emit_class(Object o, std::string_view name, T &out)
	{
		// Place to write new types.
		std::stringstream                    types;
		// Place to write methods.
		std::stringstream                    methods;
		// Set of the required properties.
		std::unordered_set<std::string_view> required_properties;

		// Collect the required properties in a set.
		if (auto required = o.required())
		{
			for (auto prop : *required)
			{
				required_properties.insert(prop);
			}
		}

		// Generate the class definition
		out << "class " << name << "{" << configNamespace
		    << "UCLPtr obj; public:\n";

		// Generate the constructor.
		out << name << "(const ucl_object_t *o) : obj(o) {}\n";

		// Generate a method for each property.
		for (auto prop : o.properties())
		{
			std::string_view prop_name   = prop.key();
			std::string_view method_name = prop_name;

			std::string method_name_buffer;

			bool isRequired = required_properties.contains(prop_name);

			// FIXME: Do a proper regex match
			if (method_name.find('-') != std::string::npos)
			{
				method_name_buffer = method_name;
				std::replace(method_name_buffer.begin(),
				             method_name_buffer.end(),
				             '-',
				             '_');
				method_name = method_name_buffer;
			}

			// If there is a description, put it in a doc comment
			if (auto description = prop.description())
			{
				methods << "\n/** " << *description << " */\n";
			}

			// Visit the schema describing this property to collect any types.
			SchemaVisitor v(method_name, types);
			prop.get().visit(v);
			// Generate the method.  If it is not a required property, it must
			// return a `std::optional<T>`.
			if (isRequired)
			{
				methods << v.return_type << ' ' << method_name << "() const "
				        << v.lifetimeAttribute << " {"
				        << "return " << v.adaptorNamespace << v.adaptor
				        << "(obj[\"" << prop_name << "\"]);}";
			}
			else
			{
				methods << "std::optional<" << v.return_type << "> "
				        << method_name << "() const " << v.lifetimeAttribute
				        << " {"
				        << "return " << configNamespace << "::make_optional<"
				        << v.adaptorNamespace << v.adaptor << ", "
				        << v.return_type << ">(obj[\"" << prop_name << "\"]);}";
			}
			methods << "\n\n";
		}

		out << types.str();
		out << methods.str();

		out << "};\n";
	}
} // namespace

int main(int argc, char **argv)
{

	std::unique_ptr<std::ofstream> file_out{nullptr};


	static struct option long_options[] = {
	  {"config-class", required_argument, nullptr, 'c'},
	  {"detail-namespace", required_argument, nullptr, 'd'},
	  {"output", required_argument, nullptr, 'o'},
	  {"embed-schema", required_argument, nullptr, 'e'},
	  {nullptr, 0, nullptr, 0},
	};

	const char *configClass = "Config";

	bool embedSchema = false;

	if (argc > 2)
	{
		int c = -1;
		int option_index;
		while ((c = getopt_long(
		          argc, argv, "d:ec:o:", long_options, &option_index)) != -1)
		{
			switch (c)
			{
				case 'c':
				{
					configClass = optarg;
					break;
				}
				case 'd':
				{
					configNamespace = optarg;
					fprintf(stderr, "Config namespace: '%s'\n", optarg);
					break;
				}
				case 'e':
				{
					embedSchema = true;
					break;
				}
				case 'o':
				{
					file_out = std::make_unique<std::ofstream>(optarg);
					break;
				}
			}
		}
	}
	 argc -= optind;
     argv += optind;

	if (argc < 1)
	{
		return -1;
	}

	const char *in_filename = argv[0];

	// Write to stdout if we weren't given a file name for explicit output
	std::ostream &out = file_out ? *file_out : std::cout;

	// Parse the schema
	struct ucl_parser *p = ucl_parser_new(UCL_PARSER_NO_IMPLICIT_ARRAYS);
	ucl_parser_add_file(p, in_filename);
	if (ucl_parser_get_error(p))
	{
		fprintf(stderr, "Error parsing schema: %s\n", ucl_parser_get_error(p));
		return EXIT_FAILURE;
	}

	auto obj = ucl_parser_get_object(p);
	ucl_parser_free(p);
	Root  conf(obj);
	char *schemaCString =
	  reinterpret_cast<char *>(ucl_object_emit(obj, UCL_EMIT_JSON_COMPACT));
	std::string schema(schemaCString);
	free(schemaCString);
	// Escape as a C string:
	auto replace = [&](std::string_view search, std::string_view replace) {
		size_t pos = 0;
		while ((pos = schema.find(search, pos)) != std::string::npos)
		{
			schema.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	};
	replace("\\", "\\\\");
	replace("\"", "\\\"");
	replace("\n", "\\n");
	ucl_object_unref(obj);

	// Generic headers
	out << "#include \"config-generic.h\"\n\n";
	out << "#include <variant>\n\n";
	out << "// Machine generated by "
	       "https://github.com/davidchisnall/config-gen DO NOT EDIT\n";
	out << "#ifdef CONFIG_NAMESPACE_BEGIN\nCONFIG_NAMESPACE_BEGIN\n#endif\n";

	// Emit the config class
	emit_class(conf, configClass, out);
	// If we've been asked to embed the schema and a constructor, do so
	if (embedSchema)
	{
		out << "inline std::variant<" << configClass
		    << ", ucl_schema_error> "
		       "make_config(ucl_object_t *obj) {"
		    << "static const ucl_object_t *schema = []() {"
		    << "static const char embeddedSchema[] = \"" << schema << "\";\n"
		    << "struct ucl_parser *p = "
		       "ucl_parser_new(UCL_PARSER_NO_IMPLICIT_ARRAYS);\n"
		    << "ucl_parser_add_string(p, embeddedSchema, "
		       "sizeof(embeddedSchema));\n"
		    << "if (ucl_parser_get_error(p)) { std::terminate(); }\n"
		    << "auto obj = ucl_parser_get_object(p);\n"
		    << "ucl_parser_free(p);\n"
		    << "return obj;\n"
		    << "}();"
		    << "ucl_schema_error err;\n"
		    << "if (!ucl_object_validate(schema, obj, &err)) { return err; }"
		    << "return " << configClass << "(obj);\n"
		    << "}\n\n";
	}
	out << "#ifdef CONFIG_NAMESPACE_END\nCONFIG_NAMESPACE_END\n#endif\n\n";
}
