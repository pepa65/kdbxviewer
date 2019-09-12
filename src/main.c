// main.c
# define VERSION "0.0.3"

#include <stdio.h> // for puts/(f)printf
#include <stdlib.h> // for exit
#include <unistd.h> // for getopt
#include <string.h>

#include <cx9r.h>
#include <key_tree.h>
#include "tui.h"
#include "helper.h"

#define diep(code, msg...) do{fprintf(stderr, msg); exit(code);}while(0)

char* filter_str = NULL;
int filter_mode = 1;
int show_passwords = 0;

#define HIDEPW show_passwords ? "" : "\033[47;37m"
#define GROUP "\033[1m\033[31m"
#define TITLE "\033[1m\033[33m"
#define FIELD "\033[36m"
#define RESET "\033[0m"

// Display Help
void print_help(char* command) {
	printf("%s %s - Dump KeePass2 .kdbx databases in various formats\n",
			command, VERSION);
	puts("Usage:");
	printf("  %s [-i|-t|-x|-c] [-p PW] [-u] [-s|-S STR] [-v|-V|-h] KDBX\n",
			command);
	puts("Commands:");
	puts("  -i        Interactive viewing (default if no -s/-S is used)");
	puts("  -t        Output as Tree (default if -s/-S is used)");
	puts("  -x        Output as XML");
	puts("  -c        Output as CSV");
	puts("Options:");
	puts("  -p PW     Decrypt file KDBX using PW  (Never use on shared");
	puts("            computers as PW can be seen in the process list!)");
	puts("  -s STR    Select only entries with STR in the Title");
	puts("  -S STR    Select only entries with STR in any field");
	puts("  -u        Display Password fields Unmasked");
	puts("  -V        Display Version");
	puts("  -v        More Verbose/debug output");
	puts("  -h        Display this Help text");
	puts("Website:    https://gitlab.com/pepa65/kdbxviewer");
}

int check_filter(cx9r_kt_entry* e, cx9r_kt_group* G) {
	if (filter_str == NULL) return 1;
	if (e->name != NULL && strstr(e->name, filter_str)) return 1;
	if (!filter_mode) return 0;
	cx9r_kt_group* g = G;
	while(g != NULL) {
		DEBUG("searching in group name %s...", cx9r_kt_group_get_name(g));
		if (strstr(cx9r_kt_group_get_name(g), filter_str)) return 1;
		DEBUG("no match\n");
		g = cx9r_kt_group_get_parent(g);
	}
	cx9r_kt_field* f = cx9r_kt_entry_get_fields(e);
	while(f != NULL) {
		const char* val = cx9r_kt_field_get_value(f);
		if (val != NULL && strstr(val, filter_str)) return 1;
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
			char* username = dq(getfield(e, "UserName"));
			char* password = dq(getfield(e, "Password"));
			char* url = dq(getfield(e, "URL"));
			char* notes = dq(getfield(e, "Notes"));
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
int main(int argc, char** argv) {
	int c, flags = 0;
	char mode = 0;
	char* pass = NULL;
	char* command = argv[0] + strlen(argv[0]);
	while (command >= argv[0] && *command != '/') --command;
	++command;
	while ((c = getopt(argc, argv, "xictp:us:S:vVh")) != -1) {
		switch (c) {
		case 'v':
			g_enable_verbose=1;
			break;
		case 'x': flags = 2;
		case 'c':
		case 't':
		case 'i':
			if (mode != 0) diep(-1, "Multiple modes not allowed\n");
			mode = c;
			break;
		case '?':
		case 'h':
			print_help(command);
			return 0;
		case 'V':
			printf("%s %s\n", command, VERSION);
			return 0;
		case 'u':
			show_passwords = 1;
			break;
		case 'p':
			pass = optarg;
			break;
		case 's':
			filter_str = optarg;
			filter_mode = 0;
			break;
		case 'S':
			filter_str = optarg;
			break;
		}
	}
	if (mode==0) mode = filter_str ? 't' : 'i';
	if (optind >= argc) diep(-2, "Missing FILENAME argument\n");
	FILE* file = fopen(argv[optind], "r");
	if (file == NULL) {
		fprintf(stderr, "Error opening %s\n", argv[optind]);
		perror("fopen");
		return -3;
	}
	//int chr, idx=0;
	//while(EOF != (chr = fgetc(file))) {
	//	printf("%02X ", chr);
	//	if (++idx%16==0) printf("\n");
	//}
	//fclose(file);
	//return 0;

	//char pass[100];
	//fprintf(stderr, "Password: ");
	//scanf("%s", &pass);
	//fprintf(stderr, "pwd: >%s<\n", pass);
	if (pass == NULL) {
		fprintf(stderr, "%sPassword: %s", FIELD, RESET);
		pass = getpass("");
	}
	cx9r_key_tree *kt = NULL;
	int res = cx9r_kdbx_read(file, pass, flags, &kt);
	if (res == 0 && mode=='t') dump_tree_group(&kt->root, 0);
	if (res == 0 && mode=='c') print_key_table(cx9r_key_tree_get_root(kt), 0);
	if (res == 0 && mode=='i') run_interactive_mode(argv[optind], kt);
	if (kt != NULL) cx9r_key_tree_free(kt);
	if (res == 3) puts("Wrong password");
	//fprintf(stderr, "\nResult: %d\n", res);
	return res;
}
