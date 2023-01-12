#include <stdio.h>
#include <stdlib.h>

void swap (int *sp1, int *sp2) {
    int* temp = sp1;
    sp1 = sp2;
    sp2 = sp1;
}

int main() {

    FILE *fp;
    char* str;
    fp = fopen("test.txt", "r");
    printf("Everything inside of test.txt: ");
    while(1) {
        str = fgets(str, 15, fp);

        if (str == EOF) {
            break;
        }
        printf("%s", str);
    }

return 0;

}