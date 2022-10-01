#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <map>
#include <string>

using namespace std;

//---------spliter manual
// line = input char array
// splitby = char used to split input
// b (begin) = result start from the b-st fragment
// e (end)   = result end before the e-st fragment (use -1 when you want get all fragment before the end of input char array)
// the b, e usage is similar to c++ containers : [begin,end)
string spliter(char * line, char splitby, int b, int e) {
    string result = "";
    int col = 0;
    for (char *c = line; *c != '\0' && col != e; c++) {
        if (*c == '\n')
            break;
        if (*c == splitby)
            col++;
        if (*c == splitby && col == b)
            continue;
        if (col >= b && col != e)
            result += *c;
    }
    return result;
}

int main(void) {
    char recbuf[1024] = "";
    char ch;
    while ( (ch = getchar()) != '\n') {
        strncat(recbuf, &ch, 1);
    }
    strncat(recbuf, &ch, 1);
    printf("%s\n", recbuf);

    cout << "===============================\n";
    cout << "all\t: "<< spliter(recbuf, ' ', 0, -1) << "-\n";
    cout << "0\t: "<< spliter(recbuf, ' ', 0, 1) << "-\n";
    cout << "1\t: "<< spliter(recbuf, ' ', 1, 2) << "-\n";
    cout << "2-end\t: "<< spliter(recbuf, ' ', 2, -1) << "-\n";
    cout << "2-4\t: "<< spliter(recbuf, ' ', 2, 5) << "-\n";
    return 0;
}