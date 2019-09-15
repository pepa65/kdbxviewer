// main.c
# define VERSION "0.1.0"
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
bool searchall = FALSE, unmask = FALSE;

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
#define ERR BRED
#define PW YELLOW
#define WARN BYELLOW

#define abort(code, msg...) do {fprintf(stderr, msg); exit(code);} while(0)
#define warn(msg...) do {fprintf(stderr, msg);} while(0)

// Display Help
void print_help(char *self, char *configfile) {
	printf("%s %s - Dump KeePass2 .kdbx databases in various formats\n",
			self, VERSION);
	puts("Usage:");
	printf("  %s [-i|-t|-x|-c] [-p PW] [-u] [[-s|-S] STR] [-v|-V|-h] [KDBX]\n",
			self);
	puts("Commands:");
	puts("  -i          Interactive viewing (default if no -s/-S is used)");
	puts("  -t          Output as Tree (default if -s/-S is used)");
	puts("  -x          Output as XML");
	puts("  -c          Output as CSV");
	puts("Options:");
	puts("  -p PW       Decrypt file KDBX using PW  (Never use on shared");
	puts("              computers as PW can be seen in the process list!)");
	puts("  [-s] STR    Select only entries with STR in the Title");
	puts("  -S STR      Select only entries with STR in any field");
	puts("  -u          Display Password fields Unmasked");
	puts("  -V          Display Version");
	puts("  -v          More Verbose/debug output");
	puts("  -h          Display this Help text");
	printf("The configfile %s is used for reading and storing KDBX files.\n",
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
	int c, flags = 0;
	char command = 0, *password = NULL, *self = argv[0] + strlen(argv[0]),
		*configfile = strcat(getenv("HOME"), CONFIGFILE);
	while (self >= argv[0] && *self != '/') --self;
	++self;
	while ((c = getopt(argc, argv, "xictp:us:S:vVh")) != -1) {
		switch (c) {
		case 'v':
			g_enable_verbose=1;
			break;
		case 'x': flags = 2;
		case 'c':
		case 't':
		case 'i':
			if (command != 0) abort(-1, "%sMultiple commands not allowed\n", ERR);
			command = c;
			break;
		case '?':
		case 'h':
			print_help(self, configfile);
			return 0;
		case 'V':
			printf("%s %s\n", self, VERSION);
			return 0;
		case 'u':
			unmask = TRUE;
			break;
		case 'p':
			password = optarg;
			break;
		case 's':
			searchall = TRUE;
		case 'S':
			if (search != NULL) abort(-3, "%sExtraneous search term: -S %s\n", ERR,
					optarg);
			search = optarg;
		}
	}

	// Try configfile for database filename
	FILE *config = NULL, *kdbx = NULL;
	bool notinconfig = TRUE, configopened = FALSE;
	char *kdbxfile;
	int n = PATHLEN;
	if ((config = fopen(configfile, "r")) != NULL) {
		configopened = TRUE;
		while (getline(&kdbxfile, &n, config) != -1) {
			*(kdbxfile+strlen(kdbxfile)-1) = 0;
			if ((kdbx = fopen(kdbxfile, "r")) != NULL) break;
		}
		if (kdbx != NULL) notinconfig = FALSE;
	}

	// Check the rest of the commandline
	if (optind < argc) {
		if (notinconfig) {
			strcpy(kdbxfile, argv[optind]);
			if ((kdbx = fopen(kdbxfile, "r")) == NULL)
				abort(-4, "%sCan't open database file: %s\n", ERR, kdbxfile);
		}
		else if (search == NULL) search = argv[optind];
			else strcpy(kdbxfile, argv[optind]);
	}
	else if (notinconfig)
		abort(-5, "%sNo database specified on commandline or in the configfile\n",
				ERR);
	if (++optind < argc)
		abort(-6, "%sExtraneous commandline argument: %s\n", ERR, argv[optind]);
	if (command == 0) command = (search == NULL) ? 'i' : 't';

	// Open the database
	if (password == NULL) {
		warn("%sPassword: %s", PW, RESET);
		password = getpass("");
	}
	cx9r_key_tree *kt = NULL;
	cx9r_err err = cx9r_kdbx_read(kdbx, password, flags, &kt);
	if (!err) {
		if (notinconfig)
			if ((config = fopen(configfile, "a")) == NULL)
				warn("%sCan't write to configfile %s%s\n", WARN, configfile, RESET);
			else fprintf(config, "%s\n", kdbxfile);
		if (command == 't') dump_tree_group(&kt->root, 0);
		if (command == 'c') print_key_table(cx9r_key_tree_get_root(kt), 0);
		if (command == 'i') run_interactive_mode(kdbxfile, kt);
	}
	else {
		if (err < 3) warn("%sInvalid database%s\n", WARN, RESET);
		if (err == 3) warn("%sWrong password%s\n", WARN, RESET);
	}
	if (kt != NULL) cx9r_key_tree_free(kt);
	return err;
}
