FILE (WRITE ${HEADER_FILE} "#ifndef COUNTRY_FLAGS_H\n")
FILE (APPEND ${HEADER_FILE} "#define COUNTRY_FLAGS_H\n")
FILE (APPEND ${HEADER_FILE} "\n")
FILE (APPEND ${HEADER_FILE} "namespace flags {\n")

FILE (GLOB FLAGS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.xpm)

# XPM arrays declare `static char *<name>[] = {"...", ...}`; the
# string literals are `const char[]` in C++ so initializing the
# non-const `char *` slots fires -Wwrite-strings on every entry.
# The data is read-only at runtime in practice (wxImage / wxBitmap
# take `const char* const*`), so suppress the warning across the
# 247 generated #includes that follow.
FILE (APPEND ${HEADER_FILE} "#pragma GCC diagnostic push\n")
FILE (APPEND ${HEADER_FILE} "#pragma GCC diagnostic ignored \"-Wwrite-strings\"\n")

FOREACH (FLAG ${FLAGS})
	FILE (APPEND ${HEADER_FILE} "#include \"${FLAG}\"\n")
ENDFOREACH (FLAG ${FLAGS})

FILE (APPEND ${HEADER_FILE} "#pragma GCC diagnostic pop\n")
FILE (APPEND ${HEADER_FILE} "\n")

FILE (APPEND ${HEADER_FILE} "struct FlagXPMCode\n")
FILE (APPEND ${HEADER_FILE} "{\n")
FILE (APPEND ${HEADER_FILE} "\tconst char *const *xpm;\n")
FILE (APPEND ${HEADER_FILE} "\tconst char *code;\n")
FILE (APPEND ${HEADER_FILE} "};\n")
FILE (APPEND ${HEADER_FILE} "\n")
FILE (APPEND ${HEADER_FILE} "static struct FlagXPMCode flagXPMCodeVector[] = {\n")

SET (FIRST TRUE)

FOREACH (FLAG ${FLAGS})
	STRING (REGEX REPLACE ".xpm" "" CODE ${FLAG})

	IF (FIRST)
		FILE (APPEND ${HEADER_FILE} "\t {${CODE}, \"${CODE}\"}")
		SET (FIRST FALSE)
	ELSE (FIRST)
		IF (${CODE} MATCHES "do")
			FILE (APPEND ${HEADER_FILE} ",\n\t {do_, \"do\"}")
		ELSE (${CODE} MATCHES "do")
			FILE (APPEND ${HEADER_FILE} ",\n\t {${CODE}, \"${CODE}\"}")
		ENDIF (${CODE} MATCHES "do")
	ENDIF (FIRST)
ENDFOREACH (FLAG ${FLAGS})

FILE (APPEND ${HEADER_FILE} "\n")
FILE (APPEND ${HEADER_FILE} "};\n")
FILE (APPEND ${HEADER_FILE} "\n")
FILE (APPEND ${HEADER_FILE} "static const int FLAGS_XPM_SIZE = (sizeof flagXPMCodeVector) / (sizeof flagXPMCodeVector[0]);\n")
FILE (APPEND ${HEADER_FILE} "\n")
FILE (APPEND ${HEADER_FILE} "}	// namespace flags\n")
FILE (APPEND ${HEADER_FILE} "#endif // COUNTRY_FLAGS_H\n")
