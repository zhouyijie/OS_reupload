#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"




int main(){

  mksfs(1);
  //free(rbuf);
  //free(wbuf);
  char name3[20] = "file3.txt";
  char name1[100] = "1234567890ubiub1.789";
  char name2[20] = "file2.txt";
  char wbuf[5] = "abcde";
  char rbuf[20];
  
  
  int fd = sfs_fopen(&name3); 
  
  for (int i =0;i<2662;i++){
    sfs_fwrite(fd,wbuf,5);
  }
  //13310 bytes, 1022 in indir, 12288 for 12 dir, 12285, 
  
  
  sfs_fseek(fd,12285);
  int read = sfs_fread(fd,rbuf,20);
  printf("%s\n",wbuf);
  printf("%s\n",rbuf);
  //int len = strlen(rbuf);
  printf("read is %d\n",read);
  //free(*rbuf);
  //free(*wbuf);
 

  

  return 1;
}
