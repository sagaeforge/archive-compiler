
#include <stdio.h>
#include <string.h>

#define cast(data, type) ((type) data)

/*
    ifstream으로 파일을 읽을 경우 
    다른 프로그램에는 마지막 줄이 안 씹히는 데
    이 프로그램에서 마지막 줄이 씹히는 버그가 발생해서 이 문제를 해결하는 프로그램을 작성했음
*/

int main(int argc, char **argv)
{
    // main 루틴에서 파일의 여부를 점검하기에 여기서는 점검하지 않음

    FILE* Target;
    Target = fopen(argv[1], "a+");
    fputs("\r\n", Target);

    fclose(Target);
}
