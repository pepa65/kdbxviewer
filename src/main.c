// main.c
# define VERSION "0.1.3"
# define CONFIGFILE "/.kdbxviewer"
# define PATHLEN 2048

#include <stdio.h> // for puts/(f)printf/fopen/getline
#include <stdlib.h> // for exit
#include <unistd.h> // for getopt
#include <string.h>

#include <cx9r.h>
#include <key_tree.h>
#include "tui.h"
#include "helper.h"

char *search = NULL;
bool searchall = FALSE;
int unmask = 0;

#define BGREEN "\033[1m\033[92m"
#define BRED "\033[1m\033[91m"
#define BYELLOW "\033[1m\033[93m"
#define BCYAN "\033[1m\033[36m"
#define CYAN "\033[36m"
#define YELLOW "\033[33m"
#define DWHITE "\033[47;37m"
#define RESET "\033[0m"

#define HIDEPW unmask ? "" : DWHITE
#define GROUP BGREEN
#define TITLE BYELLOW
#define FIELD CYAN
#define ERRC BRED
#define PWC YELLOW
#define WARNC BYELLOW

// Display Help
void print_help(char *self, char *configfile) {
	printf("%s %s - View KeePass2 .kdbx databases in various formats and ways\n",
			self, VERSION);
	puts("Usage:");
	printf("  %s [-i|-t|-x|-c|-h|-V] [-p PW] [-u] [[-s|-S] STR] [-d KDBX]\n",
			self);
	puts("Commands:");
	puts("  -i          Interactive viewing (default if no search is used)");
	puts("  -t          Output as Tree (default if search is used)");
	puts("  -x          Output as XML");
	puts("  -c          Output as CSV");
	puts("  -h          Display this Help text");
	puts("  -V          Display Version");
	puts("Options:");
	puts("  -p PW       Decrypt file KDBX using PW  (Never use on shared");
	puts("                computers as PW can be seen in the process list!)");
	puts("  -u          Display Password fields Unmasked");
	puts("  [-s] STR    Select only entries with STR in the Title");
	puts("  -S STR      Select only entries with STR in any field");
	puts("  -d KDBX     Use KDBX as the path/filename for the Database");
	printf("The configfile %s is used for storing KDBX database filenames.\n",
			configfile);
	puts("Website:      https://gitlab.com/pepa65/kdbxviewer");
}

int check_filter(cx9r_kt_entry *e, cx9r_kt_group *g) {
	if (search == NULL) return 1;
	if (e->name != NULL && strstr(e->name, search)) return 1;
	if (!searchall) return 0;
	while(g != NULL) {
		if (strstr(cx9r_kt_group_get_name(g), search)) return 1;
		g = cx9r_kt_group_get_parent(g);
	}
	cx9r_kt_field *f = cx9r_kt_entry_get_fields(e);
	while(f != NULL) {
		const char *val = cx9r_kt_field_get_value(f);
		if (val != NULL && strstr(val, search)) return 1;
		f = cx9r_kt_field_get_next(f);
	}
	return 0;
}

// Print Tree
static void indent(int n) {
	while(n-- > 0) printf("%s|%s ", GROUP, RESET);
}
static void dump_tree_field(cx9r_kt_field *f, int depth) {
	if (f->value != NULL) {
		if (strcmp(f->name, "Notes") == 0) indent(depth-1);
		else {
			indent(depth-1);
			printf("%s%s: \"%s", FIELD, f->name, RESET);
		}
		if (strcmp(f->name, "Password") == 0)
			printf("%s%s%s", HIDEPW, f->value, RESET);
		else printf("%s", f->value);
		if (strcmp(f->name, "Notes") == 0) puts("");
		else printf("%s\"%s\n", FIELD, RESET);
	}
	if (f->next != NULL) dump_tree_field(f->next, depth);
}

static void dump_tree_entry(cx9r_kt_group *g, cx9r_kt_entry *e, int depth) {
	if (check_filter(e, g)) {
		indent(depth-1);
		if (e->name != NULL) printf("%s%s%s\n", TITLE, e->name, RESET);
		if (e->fields != NULL) dump_tree_field(e->fields, depth);
		else puts("");
	}
	if (e->next != NULL) dump_tree_entry(g, e->next, depth);
}

static void dump_tree_group(cx9r_kt_group *g, int depth) {
	indent(depth);
	if (g->name != NULL) printf("%s%s%s", GROUP, g->name, RESET);
	puts("");
	if (g->entries != NULL) dump_tree_entry(g, g->entries, depth + 1);
	if (g->next != NULL) dump_tree_group(g->next, depth);
	if (g->children != NULL) dump_tree_group(g->children, depth + 1);
}

// Print CSV
void print_key_table(cx9r_kt_group *g, int level) {
	cx9r_kt_entry *e = cx9r_kt_group_get_entries(g);
	puts("\"Group\",\"Title\",\"Username\",\"Password\",\"URL\",\"Notes\"");
	while (e != NULL) {
		if (check_filter(e, g)) {
			char *username = dq(getfield(e, "UserName")),
				*password = dq(getfield(e, "Password")),
				*url = dq(getfield(e, "URL")),
				*notes = dq(getfield(e, "Notes"));
			printf("\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
					cx9r_kt_group_get_name(g), cx9r_kt_entry_get_name(e),
					username, password, url, notes);
			// Allocated in helper.c::dq()
			free(username);
			free(password);
			free(url);
			free(notes);
		}
		e = cx9r_kt_entry_get_next(e);
	}
	cx9r_kt_group *c = cx9r_kt_group_get_children(g);
	while(c != NULL) {
		print_key_table(c, level + 1);
		c = cx9r_kt_group_get_next(c);
	}
}

// Process commandline
int main(int argc, char **argv) {
	long unsigned int len = PATHLEN, opt, flags = 0;
	char *kdbxfile = malloc(len), *kdbxconf = malloc(len), command = 0,
		*password = NULL, *self = argv[0] + strlen(argv[0]),
		*configfile = strcat(getenv("HOME"), CONFIGFILE);
	FILE *config = NULL, *kdbx = NULL;
	*kdbxfile = 0;

#define abort(code, msg...) do {fprintf(stderr, msg); free(kdbxfile);\
	free(kdbxconf); exit(code);} while(0)
#define warn(msg...) do {fprintf(stderr, msg);} while(0)

	while (self >= argv[0] && *self != '/') --self;
	++self;
	while ((opt = getopt(argc, argv, "xictp:us:S:d:Vh")) != -1) {
		switch (opt) {
		case 'x': flags = 2;
		case 'c':
		case 't':
		case 'i':
		case 'h':
		case 'V':
			if (command != 0) abort(-1, "%sMultiple commands not allowed\n", ERRC);
			command = opt;
			break;
		case 'u':
			unmask = 1;
			break;
		case 'p':
			password = optarg;
			break;
		case 'S':
			searchall = TRUE;
		case 's':
			if (search != NULL)
				abort(-2, "%sSuperfluous search term: %s\n", ERRC, optarg);
			search = optarg;
			break;
		case 'd':
			if ((kdbx = fopen(optarg, "r")) == NULL)
				abort(-3, "%sCan't open database file: %s\n", ERRC, optarg);
			kdbxfile = optarg;
		}
	}

	if (command == 'h') {
		print_help(self, configfile);
		return 0;
	}
	if (command == 'V') {
		printf("%s %s\n", self, VERSION);
		return 0;
	}

	if (optind < argc) // Must be [-s] argument, unless already given
		if (search == NULL) search = argv[optind++];
		else abort(-4, "%sSuperfluous argument: %s\n", ERRC, argv[optind]);

	if (optind < argc)
		abort(-5, "%sSuperfluous argument: %s", ERRC, argv[optind]);

	if (*kdbxfile == 0) { // Try configfile for database filename
		*kdbxconf = 0;
		if ((config = fopen(configfile, "r")) != NULL)
			while (getline(&kdbxconf, &len, config) != -1) {
				*(kdbxconf+strlen(kdbxconf)-1) = 0;
				// Check the latest found file
				if ((kdbx = fopen(kdbxconf, "r")) != NULL) strcpy(kdbxfile, kdbxconf);
				*kdbxconf = 0;
			}
		if (*kdbxfile == 0)
			abort(-6, "%sNo database specified on commandline or in configfile\n",
				ERRC);
		else strcpy(kdbxconf, kdbxfile);
	}

	if (command == 0) command = (search == NULL) ? 'i' : 't';

	// Open the database
	if (password == NULL) {
		warn("%sPassword: %s", PWC, RESET);
		password = getpass("");
	}
	cx9r_key_tree *kt = NULL;
	cx9r_err err = cx9r_kdbx_read(kdbx, password, flags, &kt);
	if (!err) {
		if ((config = fopen(configfile, "a")) == NULL)
			warn("%sCan't write to configfile %s%s\n", WARNC, configfile, RESET);
		else if (strcmp(kdbxconf, kdbxfile) != 0)
			fprintf(config, "%s\n", kdbxfile);
		if (command == 't') dump_tree_group(&kt->root, 0);
		if (command == 'c') print_key_table(cx9r_key_tree_get_root(kt), 0);
		if (command == 'i') run_interactive_mode(kdbxfile, kt);
	}
	else {
		if (err == 16) warn("%sPassword invalid%s\n", WARNC, RESET);
		else warn("%sDatabase error%s\n", WARNC, RESET);
	}
	if (kt != NULL) cx9r_key_tree_free(kt);
	return err;
}
