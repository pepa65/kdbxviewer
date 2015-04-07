
#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for getopt */

#include <cx9r.h>
#include <key_tree.h>
#include "tui.h"

void print_help();
char* getfield(cx9r_kt_entry* e, char* name);
void print_key_table(cx9r_kt_group *g, int level);

#define diep(code, msg...) do{ fprintf(stderr, msg); exit(code); }while(0)

int main(int argc, char** argv) {
    int c;
    int flags = 0; char* pass = NULL; char mode = 0;
    while ( (c = getopt(argc, argv, "xctp:v?h")) != -1) {
        switch (c) {
        case 'v':
            g_enable_verbose=1;
            break;
        case 'x':
            if (mode != 0) diep(-1,"Multiple modes not allowed\n");
            flags |= FLAG_DUMP_XML; mode ='x';
            break;
        case 'c':
            if (mode != 0) diep(-1,"Multiple modes not allowed\n");
            mode = 'c';
            break;
        case 't':
            if (mode != 0) diep(-1,"Multiple modes not allowed\n");
            mode = 't';
            break;
        case '?':case 'h':
            print_help(argv[0]);
            return 0;
        case 'p':
            pass = optarg;
            break;
        default:
            fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
            return 1;
        }
    }
    if (optind >= argc)diep(-2,"Missing FILENAME argument\n");
    
    
    FILE* file = fopen(argv[optind], "r");
    if (file == NULL) { printf("Error opening %s\n", argv[optind]); perror("fopen"); return -3; }
    /*
    int chr, idx=0;
    while(EOF != (chr = fgetc(file))) {
        printf("%02X ", chr);
        if (++idx%16==0) printf("\n");
    }
    fclose(file);
    return 0;
    */
    //char pass[100];
    //printf("Passphrase: ");
    //scanf("%s", &pass);
    //printf("pwd: >%s<\n", pass);
    if (pass == NULL)
        pass = getpass("Passphrase: ");
    
    cx9r_key_tree *kt = NULL;
    int res = cx9r_kdbx_read(file, pass, flags, &kt);
    if (res == 0 && mode=='t') {
        cx9r_dump_tree(kt);
    }
    if (res == 0 && mode=='c') {
        print_key_table(cx9r_key_tree_get_root(kt), 0);
    }
    if (kt != NULL) {
        cx9r_key_tree_free(kt);
    }
    //printf("\nResult: %d\n", res);
    return res;
}


char* trail[10];

static int print_trail(int n) {
    int i = 0, l = 0;
    while(i<=n) l += printf("%s/", trail[i++]);
    return  l;
}

void print_key_table(cx9r_kt_group *g, int level) {
    trail[level] = cx9r_kt_group_get_name(g);
    cx9r_kt_entry *e = cx9r_kt_group_get_entries(g);
    while (e != NULL) {
        int l = print_trail(level);
        l += printf("%s", cx9r_kt_entry_get_name(e));
        while(l++<50)putchar(' ');
        printf("\t%s\n", getfield(e, "Password"));
        e = cx9r_kt_entry_get_next(e);
    }
    cx9r_kt_group *c = cx9r_kt_group_get_children(g);
    while(c != NULL) {
        print_key_table(c, level + 1);
        
        c =  cx9r_kt_group_get_next(c);
    }
    
    
}

char* empty = "";

char* getfield(cx9r_kt_entry* e, char* name) {
    cx9r_kt_field *f = cx9r_kt_entry_get_fields(e);
    while(f != NULL) {
        if (strcmp(cx9r_kt_field_get_name(f), name) == 0) return cx9r_kt_field_get_value(f);
        f = cx9r_kt_field_get_next(f);
    }
    return empty;
}


void print_help(char* arg0) {
    puts("KDBX Viewer 0.0.1\n");
    puts("dumps KeePass2 Database files in various formats\n");
    puts("\n");
    printf("  %s [-v] [-x|-t|-b] [-p PASSPHRASE] [-f FILTER] FILENAME\n", arg0);
    puts("   -x    XML dump\n");
    puts("   -t    Tree\n");
    puts("   -c    CSV format\n");
    puts("\n");
    
    
    
}


