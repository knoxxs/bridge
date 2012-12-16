#include <stdio.h>
#include <string.h>

int main(){
char a[strlen("password")+1];

scanf("%s",a);


printf("SIze of a %d, Scanned value : %s\n", sizeof(a),a);

if(strcmp(a,"password") == 0){
printf("Yeah\n");
}
else{
printf("OH no!!\n");
}

return 0;
}