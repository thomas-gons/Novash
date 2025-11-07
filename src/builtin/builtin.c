#include "builtin.h"

static builtin_entry_t *builtins = NULL;

void builtin_init() {
  builtins = NULL;

  shput(builtins, "cd", builtin_cd);
  shput(builtins, "echo", builtin_echo);
  shput(builtins, "exit", builtin_exit);
  shput(builtins, "pwd", builtin_pwd);
  shput(builtins, "jobs", builtin_jobs);
  shput(builtins, "fg", builtin_fg);
  shput(builtins, "bg", builtin_bg);
  shput(builtins, "history", builtin_history);
  shput(builtins, "type", builtin_fn_type);
}

/* --- BUILTIN LOOKUP --- */

bool builtin_is_builtin(char *name) { return shget(builtins, name) != NULL; }

builtin_fn_t builtin_get_function(char *name) { return shget(builtins, name); }

/* --- CLASSIC BUILTINS --- */

int builtin_cd(int argc, char *argv[]) {
  const char *home = shell_state_getenv("HOME");
  char *_path = argv[1];
  int r = chdir(argc == 1 || *_path == '~' ? home : _path);
  
  shell_state_t *sh_state = shell_state_get();
  getcwd(sh_state->identity.cwd, PATH_MAX);
  return r;
}

int builtin_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++)
    printf("%s ", argv[i]);

  printf("\n");
  return 0;
}

int builtin_exit(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  sh_state->should_exit = true;
  return 0;
}

int builtin_pwd(int argc, char *argv[]) {
  shell_state_t *sh_state = shell_state_get();
  printf("%s\n", sh_state->identity.cwd);
  return 0;
}

int builtin_fn_type(int argc, char *argv[]) {
  if (argc < 2) {
    printf("type: missing argument\n");
    return 1;
  }

  char *cmd = argv[1];

  if (builtin_is_builtin(cmd)) {
    printf("%s is a shell builtin\n", cmd);
  } else {
    char *cmd_path = is_in_path(cmd);
    if (cmd_path) {
      printf("%s is %s\n", cmd, cmd_path);
      free(cmd_path);
    } else {
      printf("%s: not found\n", cmd);
    }
  }
  return 0;
}