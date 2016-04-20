#include <fcntl.h>
#include <stdio.h>

#define DEV1     "/dev/ht2030_100"
#define DEV2     "/dev/ht2030_101"
#define DEV3     "/dev/ht2030_102"

int main (int argc, char *argv[])
{
  int           fd1, fd2, fd3;
  unsigned char val;
  int ret = 0;

  fd1 = open (DEV1, O_RDONLY);
  if (fd1 < 0) {
    fprintf (stderr, "%s open error.\n", DEV1);
    return -1;
  }
  fd2 = open (DEV2, O_RDONLY);
  if (!fd2 < 0) {
    fprintf (stderr, "%s open error.\n", DEV2);
    return -1;
  }
  fd3 = open (DEV3, O_WRONLY);
  if (!fd3 < 0) {
    fprintf (stderr, "%s open error.\n", DEV3);
    return -1;
  }

  read (fd1, &val, 1);
  printf ("Read HT2030/0x100: 0x%02x\n", val);
  read (fd2, &val, 1);
  printf ("Read HT2030/0x101: 0x%02x\n", val);

  val = 0x55;
  printf ("Write HT2030/0x102 = 0x%02x\n", val);
  write (fd3, &val, 1);

  close (fd1);
  close (fd2);
  close (fd3);
  
  return 0;
}
