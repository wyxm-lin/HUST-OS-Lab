// #include "user_lib.h"
// #include "util/types.h"

// int main(int argc, char *argv[]) {
//   printu("\n======== exec /bin/app_ls in app_exec ========\n");
//   int ret = exec("/bin/app_ls");
//   if (ret == -1)
//     printu("exec failed!\n");

//   exit(0);
//   return 0;
// }
#include "user_lib.h"
#include "util/types.h"

int main(int argc, char* argv[]) {
  int ret = exec("/bin/app_ls");
  if (ret == -1) {
    printu("fail\n");
  }
  exit(0);
  return 0;
}

// ÈÚºÏlab3_chanllenge1
// #include "user_lib.h"
// #include "util/types.h"

// int main(int argc, char *argv[]) {
//   int pid = fork();
//   if (pid == 0) {
//     printu("\n======== exec /bin/app_ls in app_exec ========\n");
//     int ret = exec("/bin/app_ls");
//     if (ret == -1)
//     printu("exec failed!\n");
//     exit(0);
//   }
//   else {
//     wait(pid);
//     printu("parent process exit\n");
//     exit(0);
//   }
//   return 0;
// }
