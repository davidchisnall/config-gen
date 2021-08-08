UCL Configuration Generator
===========================

This repository provides tooling for generating C++ classes that expose a UCL
object tree defined by a JSON schema (which can itself be written in any UCL
format, rather than plain JSON).

Usage
-----

The built `config-gen` tool is invoked with any of the following options, followed by the path to a JSON Schema file:

 - `--config-class` or `-c` followed by the name of the class to generate for the config.
   If this is not specified then the class will be called `Config`.
 - `--detail-namespace` or `-d` followed by the name of the namespace used for the helpers from `config-generic.h`.
   If this is not specified, it will default to `::config::detail`.
 - `--output` or `-o` followed by the name of a file to which the output should be written.
   If this is not specified, the output is written to standard out.
   If you are committing the generated file to revision control, piping it directly to `clang-format` is probably better than writing the unreadable version to a file.
 - `--embed-schema` or `-e` indicates that the tool should embed a minified version of the schema and provide a `make_config` file in the generated header that parses the config and validates it against the provided schema.

The output file depends on `config-generic.h` from this repository.

Limitations
-----------

This tool intends to support as much of JSON Schema as makes sense for config files.
It currently does not support several things that are useful and will hopefully be added at some point (PRs welcome!):

 - [ ] Cross references
 - [ ] Enumerations
 - [ ] Enumerations as keys for defining a class
 - [ ] Arrays of anything other than a single type.
 - [ ] `additionalProperties` on objects.
 - [ ] Any of the schema composition operators.
