
#include <nString.h>
#include <stdio.h>

int
main(int argc, char const* argv[])
{

  char str1[20];
  char str2[20];

  scanf("%s\n", str1);
  fgets(str2, 12, stdin);

  printf("%s\n", str1);
  printf("%s\n", str2);

  return 0;
}
