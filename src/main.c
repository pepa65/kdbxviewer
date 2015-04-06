


#include <cx9r.h>
int main(int argc, char** argv) {
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) { perror("Failed to open file"); return 1; }
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
    char* pass;
    pass = getpass("Passphrase: ");
    
    int res = cx9r_kdbx_read(file, pass);
    printf("Result: %d\n", res);
    return 42;
}
