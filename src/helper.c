// helper.c

#include <stdio.h> // for printf
#include <stdlib.h> // for exit
#include <unistd.h> // for getopt
#include <string.h>

#include <key_tree.h>

#include "helper.h"

const char *getfield(cx9r_kt_entry *e, char *name) {
	cx9r_kt_field *f = cx9r_kt_entry_get_fields(e);
	while(f != NULL) {
		if (strcmp(cx9r_kt_field_get_name(f), name) == 0)
			return cx9r_kt_field_get_value(f) ? cx9r_kt_field_get_value(f) : "";
		f = cx9r_kt_field_get_next(f);
	}
	return "";
}

// Double the doublequotes for CSV
char *dq(const char *field) {
	// Needs to be freed by the caller
	char *dqfield = malloc(2*strlen(field)), *pointer = dqfield;
	while (*field) {
		*pointer = *(field++);
		if (*(pointer++) == '"') *(pointer++) = '"';
	}
	*pointer = 0;
	return dqfield;
}
