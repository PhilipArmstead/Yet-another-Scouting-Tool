file(WRITE "${OUTPUT}" "#include \"app/resources.h\"\n#include <string.h>\n")
set(entries "")
set(index 0)
foreach(path IN LISTS INPUTS)
	file(READ "${ROOT}/${path}" bytes HEX)
	string(REGEX REPLACE "(..)" "0x\\1," bytes "${bytes}")
	file(APPEND "${OUTPUT}" "static const unsigned char data${index}[] = {${bytes}};\n")
	string(APPEND entries "{\"${path}\", data${index}, sizeof(data${index})},\n")
	math(EXPR index "${index}+1")
endforeach()
file(APPEND "${OUTPUT}" "static const Resource resources[] = {\n${entries}};\n"
	"const Resource *resources_get(const char *name) {\n"
	"for (size_t i = 0; i < sizeof(resources)/sizeof(resources[0]); ++i) {\n"
	"if (strcmp(resources[i].name, name) == 0) return &resources[i];\n}\nreturn NULL;\n}\n")
