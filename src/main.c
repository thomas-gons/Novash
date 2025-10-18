#include "main.h"

bool is_metachar(char c) {
    return (c == '|' || c == '&' || c == ';' || c == '<' || c == '>');
}

void *realloc_s(void *ptr, size_t s) {
    void *tmp_ptr = realloc(ptr, s);
    if (!tmp_ptr) {perror("realloc failed"); exit(EXIT_FAILURE);}
    return tmp_ptr;
}


char peek(tokenizer_t *tz) {
    if (tz->pos >= tz->length) return '\0';
    return tz->input[tz->pos];
}

char advance(tokenizer_t *tz) {
    if (tz->pos >= tz->length) return '\0';
    return tz->input[tz->pos++];
}

void skip_whitespaces(tokenizer_t *tz) {
    char c;
    while ((c = peek(tz)) == ' ' || c == '\t') {
        advance(tz);
    }
}

token_t get_next_token(tokenizer_t *tz) {
    skip_whitespaces(tz);
    char c = peek(tz);
    switch (c) {
        case '|': {
            advance(tz);
            if (peek(tz) == '|') { advance(tz); return (token_t){TOK_OR, "||"}; }
            else return (token_t){TOK_PIPE, "|"};
        }
        case '&': {
            advance(tz);
            if (peek(tz) == '&') { advance(tz); return (token_t){TOK_AND, "&&"}; }
            else return (token_t){TOK_BG, "&"};
        }
        case '>': {
            advance(tz);
            if (peek(tz) == '>') { advance(tz); return (token_t){TOK_REDIR_APPEND, ">>"}; }
            else return (token_t){TOK_REDIR_OUT, ">"};
        }
        case '<': {
            advance(tz);
            if (peek(tz) == '<') { advance(tz); return (token_t){TOK_HEREDOC, "<<"}; }
            else return (token_t){TOK_REDIR_IN, "<"};
        }
        case ';': { advance(tz); return (token_t) {TOK_SEMI, ";"}; }
        case '\0': { advance(tz); return (token_t) {TOK_EOF, NULL}; }
        default: {
            bool in_sq = false, in_dq = false;
            
            size_t buf_cap = 64;
            char *buf = (char *) malloc(buf_cap);
            unsigned length = 0;
            
            while ((c = peek(tz)) != '\0') {
                if (c == '\'' && !in_dq) { in_sq = !in_sq; advance(tz); continue; }
                if (c == '\"' && !in_sq) { in_dq = !in_dq; advance(tz); continue; }
                if (!in_sq && !in_dq && ((c == ' ' || c == '\t') || is_metachar(c))) break;
                
                if (length == buf_cap) { buf_cap *= 2; buf = (char*) realloc_s(buf, buf_cap);}
                buf[length++] = advance(tz);
            }
            
            
            char *end;
            strtol(buf, &end, 10);
            if (*end == '\0' && (c == '>' || c == '<')) return (token_t) {TOK_FD, buf}; 

            if (length == buf_cap) { buf_cap *= 2; buf = (char*) realloc_s(buf, buf_cap);}
            buf[length] = '\0';
            
            return (token_t) {TOK_WORD, buf}; 
        }
    }
}


void print_token(token_t tok) {
    switch (tok.type) {
        case TOK_AND:          printf("[TOK_AND]: %s\n", tok.value); break;
        case TOK_BG:           printf("[TOK_BG]: %s\n", tok.value); break;
        case TOK_OR:           printf("[TOK_OR]: %s\n", tok.value); break;
        case TOK_PIPE:         printf("[TOK_PIPE]: %s\n", tok.value); break;
        case TOK_REDIR_APPEND: printf("[TOK_REDIR_APPEND]: %s\n", tok.value); break;
        case TOK_REDIR_IN:     printf("[TOK_REDIR_IN]: %s\n", tok.value); break;
        case TOK_REDIR_OUT:    printf("[TOK_REDIR_OUT]: %s\n", tok.value); break;
        case TOK_SEMI:         printf("[TOK_SEMI]: %s\n", tok.value); break;
        case TOK_WORD:         printf("[TOK_WORD]: %s\n", tok.value); break;
        case TOK_EOF:          printf("[TOK_EOF]\n"); break;
        default:               printf("[UNKNOWN]: %s\n", tok.value); break;
    }
}

void next_token(tokenizer_t *tz) {
    g_tok = get_next_token(tz);
}

ast_node_t *parse_command(tokenizer_t *tz) {
    if (g_tok.type != TOK_WORD) return NULL;

    size_t argv_buf_cap = 64;
    char **argv_buf = (char **) malloc(argv_buf_cap * sizeof(char*));
    unsigned argc = 0;
    while (g_tok.type == TOK_WORD) {
        argv_buf[argc++] = g_tok.value;
        next_token(tz);

        if (argc == argv_buf_cap) {
            argv_buf_cap *= 2;
            argv_buf = (char**) realloc_s(argv_buf, argv_buf_cap * sizeof(char*));
        }
    }
    
    argv_buf[argc] = NULL;

    size_t redir_buf_cap = 16;
    redirection_t *redir_buf = (redirection_t*) malloc(redir_buf_cap * sizeof(redirection_t));
    size_t redir_count = 0;
    
    while (g_tok.type == TOK_FD || g_tok.type == TOK_REDIR_IN || 
       g_tok.type == TOK_REDIR_OUT || g_tok.type == TOK_REDIR_APPEND) {
        redirection_t redir = {0};

        if (g_tok.type == TOK_FD) {
            redir.fd = strtol(g_tok.value, NULL, 10);
            next_token(tz);
        }

        switch (g_tok.type) {
            case TOK_REDIR_IN: 
                redir.type = REDIR_IN;
                break;
            case TOK_REDIR_OUT: {
                redir.type = REDIR_OUT;
                if (redir.fd == 0) redir.fd = 1;
                break;
            }
            case TOK_REDIR_APPEND: {
                redir.type = REDIR_APPEND;
                if (redir.fd == 0) redir.fd = 1;
                break;
            }
            default:
                fprintf(stderr, "Unexpected token type %d in redirection\n", g_tok.type);
                exit(EXIT_FAILURE);
        }

        next_token(tz);

        if (g_tok.type != TOK_WORD) {
            fprintf(stderr, "Expected filename after redirection\n");
            exit(EXIT_FAILURE);
        }

        redir.target = strdup(g_tok.value);
        redir_buf[redir_count++] = redir;

        next_token(tz);

        if (redir_count == redir_buf_cap) {
            redir_buf_cap *= 2;
            redir_buf = (redirection_t*) realloc_s(redir_buf, redir_buf_cap * sizeof(redirection_t));
        }
    }

    bool is_bg = g_tok.type == TOK_BG;

    ast_node_t *ast_node = (ast_node_t*) malloc(sizeof(ast_node_t));
    ast_node->cmd = (cmd_node_t) {
        .argc=argc,
        .argv=argv_buf,
        .redir=redir_buf,
        .redir_count=redir_count,
        .is_bg=is_bg
    };

    ast_node->type = NODE_CMD;
    return ast_node;
}

ast_node_t *parse_pipeline(tokenizer_t *tz) {
    ast_node_t *left = parse_command(tz);
    while (g_tok.type == TOK_PIPE) {
        next_token(tz);
        ast_node_t *right = parse_command(tz);
        ast_node_t *node = malloc(sizeof(ast_node_t));
        node->type = NODE_PIPELINE;
        node->pipe.left = left;
        node->pipe.right = right;
        left = node;
    }
    return left;
}

ast_node_t *parse_conditional(tokenizer_t *tz) {
    ast_node_t *left = parse_pipeline(tz);
    while (g_tok.type == TOK_AND || g_tok.type == TOK_OR) {
        cond_op_e op = g_tok.type == TOK_AND ? COND_AND: COND_OR;
        next_token(tz);
        ast_node_t *right = parse_pipeline(tz);
        ast_node_t *node = malloc(sizeof(ast_node_t));
        node->type = NODE_CONDITIONAL;
        node->cond.left = left;
        node->cond.right = right;
        node->cond.op = op; 
        left = node;
    }
    return left;
}

ast_node_t *parse_sequence(tokenizer_t *tz) {
    ast_node_t *node = malloc(sizeof(ast_node_t));
    size_t nodes_cap = 16;
    node->type = NODE_SEQUENCE;
    node->seq.nodes_count = 0;
    node->seq.nodes = malloc(nodes_cap * sizeof(ast_node_t*));

    ast_node_t *next = parse_conditional(tz);
    do {
        if (node->seq.nodes_count == nodes_cap) {
            nodes_cap *= 2;
            node->seq.nodes = realloc(node->seq.nodes, nodes_cap * sizeof(ast_node_t*));
        }
        node->seq.nodes[node->seq.nodes_count++] = next;

        next_token(tz);
        next = parse_conditional(tz);
    } while (g_tok.type == TOK_SEMI || g_tok.type == TOK_BG);

    return node;
}

void print_ast(ast_node_t *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_CMD: {
            printf("CMD: %s", node->cmd.argv[0]);
            for (int i = 1; i < node->cmd.argc; i++)
                printf(" %s", node->cmd.argv[i]);
            if (node->cmd.redir_count > 0) {
                printf(" [");
                for (size_t i = 0; i < node->cmd.redir_count; i++) {
                    redirection_t *r = &node->cmd.redir[i];
                    printf("%d", r->fd);
                    switch (r->type) {
                        case REDIR_IN: printf("<"); break;
                        case REDIR_OUT: printf(">"); break;
                        case REDIR_APPEND: printf(">>"); break;
                    }
                    printf("%s", r->target);
                    if (i < node->cmd.redir_count - 1) printf(", ");
                }
                printf("]");
            }
            if (node->cmd.is_bg) printf(" &");
            printf("\n");
            break;
        }
        case NODE_PIPELINE: {
            printf("PIPELINE\n");
            print_ast(node->pipe.left, indent + 1);
            print_ast(node->pipe.right, indent + 1);
            break;
        }
        case NODE_CONDITIONAL: {
            printf("CONDITIONAL (%s)\n", node->cond.op == COND_AND ? "&&" : "||");
            print_ast(node->cond.left, indent + 1);
            print_ast(node->cond.right, indent + 1);
            break;
        }
        case NODE_SEQUENCE: {
            printf("SEQUENCE\n");
            for (size_t i = 0; i < node->seq.nodes_count; i++) {
                print_ast(node->seq.nodes[i], indent + 1);
            }
            break;
        }
    }
}

int exec_pipeline(ast_node_t *ast_node) {
    pipe_node_t pipe_node = ast_node->pipe;

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_node(pipe_node.left);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_node(pipe_node.right);
        _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return 0;
}

int handle_redirection(cmd_node_t cmd_node) {
    int *fd = (int*) malloc(cmd_node.redir_count * sizeof(int));
    int oflag;
    for (unsigned i = 0; i < cmd_node.redir_count; i++) {
        redirection_t redir = cmd_node.redir[i];
        oflag = (redir.type == REDIR_IN) ?  O_RDONLY:
                    (redir.type == REDIR_OUT) ? O_WRONLY | O_CREAT | O_TRUNC:
                                                O_WRONLY | O_CREAT | O_APPEND;
        
        fd[i] = open(redir.target, oflag, 0644);
        if (fd[i] == -1) { perror("open failed"); return 1; }
        if (dup2(fd[i], redir.fd) == -1) { perror("fd duplication failed"); return 1;}

        close(fd[i]);
    }
    return 0;
}

void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n$ ", 3);
    fflush(stdout);
}

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    char buf[128];

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int n = snprintf(buf, sizeof(buf), "\n[%d] finished with status %d\n$ ", pid, WEXITSTATUS(status));
        if (n > 0) {
            write(STDOUT_FILENO, buf, n);
        }
    }
}

int run_child(cmd_node_t cmd_node, runner_f_t runner_f) {
    pid_t pid = fork();

    if (pid == 0) {
        setpgid(0, 0);
        if (!cmd_node.is_bg) {
            tcsetpgrp(STDIN_FILENO, getpid());
        } else {
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        if (handle_redirection(cmd_node) != 0) _exit(1);
        runner_f(cmd_node);
        _exit(0);
    } else if (pid > 0) {
        setpgid(pid, pid);
        if (cmd_node.is_bg) {
            if (shell_state.bg_tasks_count < MAX_BG_TASKS) {
                shell_state.bg_tasks[shell_state.bg_tasks_count++] = pid;
                printf("[%d] %s running in background\n", pid, cmd_node.argv[0]);
            } else {
                fprintf(stderr, "Too many background tasks\n");
            }
            return 0;
        }
        int status;
        tcsetpgrp(STDIN_FILENO, pid);
        waitpid(pid, &status, 0);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    } else {
        perror("fork failed");
        return -1;
    }
}

void builtin_runner(cmd_node_t cmd_node) {
    builtin_f_t f = get_builtin(cmd_node.argv[0]);
    int r = f(cmd_node.argc, cmd_node.argv);
    _exit(r);
}

void external_cmd_runner(cmd_node_t cmd_node) {
    char *cmd_path = is_in_path(cmd_node.argv[0]);
    if (!cmd_path) {
        fprintf(stderr, "%s: command not found\n", cmd_node.argv[0]);
        _exit(127);
    }
    execv(cmd_path, cmd_node.argv);
    fprintf(stderr, "%s: %s\n", cmd_node.argv[0], strerror(errno));
    free(cmd_path);
    _exit(1);
}

int exec_node(ast_node_t *ast_node) {
    if (!ast_node)
        return -1;

    int r = 0;
    switch (ast_node->type) {
        case NODE_SEQUENCE:
            seq_node_t seq_node = ast_node->seq;
            for (unsigned i = 0; i < seq_node.nodes_count; i++) {
                r = exec_node(seq_node.nodes[i]);
            }
            return r;

        case NODE_CONDITIONAL:
            cond_node_t cond_node = ast_node->cond;
            if (cond_node.op == COND_AND) {
                r = exec_node(cond_node.left);
                if (r == 0) r = exec_node(cond_node.right);
            } else if (cond_node.op == COND_OR) {
                r = exec_node(cond_node.left);
                if (r != 0) r = exec_node(cond_node.right);
            }
            return r;

        case NODE_PIPELINE:
            return exec_pipeline(ast_node);

        case NODE_CMD:
            cmd_node_t cmd_node = ast_node->cmd;
            char *cmd = ast_node->cmd.argv[0];

            if (!cmd) return 0;

            if (is_builtin(cmd)) {
                if (cmd_node.redir_count == 0) {
                    builtin_f_t f = get_builtin(cmd);
                    return f(cmd_node.argc, cmd_node.argv);
                }
                return run_child(cmd_node, builtin_runner);
            }
            
            if ((is_in_path(cmd)) != NULL) {
                return run_child(cmd_node, external_cmd_runner);
            } else {
                printf("%s: command not found\n", cmd);
                r = 127;
            }

            return r;

        default:
            fprintf(stderr, "Unknown AST node type: %d\n", ast_node->type);
            return -1;
    }
}


char *is_in_path(char *cmd) {
    char *dir;
    char cmd_path[1024];
    struct stat buf;

    char *_p = strdup(shell_state.path);

    for (char *p = _p; (dir = strsep(&p, ":")) != NULL;) {
        snprintf(cmd_path, sizeof(cmd_path), "%s/%s", dir, cmd);
        if (stat(cmd_path, &buf) == 0 && buf.st_mode & S_IXUSR) {
            free(_p);
            return strdup(cmd_path);
        }
    }

    free(_p);
    return NULL;
}


buitlin_t builtins[] = {
    {"cd", cd_builtin},
    {"echo", echo_builtin},
    {"exit", exit_builtin},
    {"pwd", pwd_builtin},
    {"type", type_builtin},
};

#define BUILTINS_N (int) (sizeof(builtins) / sizeof(buitlin_t))

int cd_builtin(int argc, char *argv[]) {
    const char *home = getenv("HOME");
    char *_path = argv[1];
    int r = chdir(argc == 1 || *_path == '~' ? home: _path);
    return r;
}

int echo_builtin(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++)
        printf("%s ", argv[i]);
    
    printf("\n");
    return 0;
}

int exit_builtin(int argc, char *argv[]) {
    int code = atoi(argv[1]);
    exit(code);
    return 0;
}

int pwd_builtin(int argc, char *argv[]) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    return 0;
}

int type_builtin(int argc, char *argv[]) {
    if (argc < 2) {
        printf("type: missing argument\n");
        return 1;
    }

    char *cmd = argv[1];

    if (is_builtin(cmd)) {
        printf("%s is a shell_state builtin\n", cmd);
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


bool is_builtin(char *name) {
    for (int i = 0; i < BUILTINS_N; i++) {
        if (strcmp(name, builtins[i].name) == 0)
            return true;
    }
    return false;
}

builtin_f_t get_builtin(char *name) {
    for (int i = 0; i < BUILTINS_N; i++) {
        if (strcmp(name, builtins[i].name) == 0)
            return builtins[i].f;
    }
    return NULL;
}


int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    shell_state.path = getenv("PATH");
    setbuf(stdout, NULL);

    tokenizer_t tz = (tokenizer_t) {0};

    do {
        tz.pos = 0;
        tz.input = readline("$ ");
        tz.length = strlen(tz.input);

        g_tok = (token_t){0};
        next_token(&tz);
        
        ast_node_t *ast_node = parse_sequence(&tz);
        exec_node(ast_node);
    } while (*tz.input != '\0');
    return 0;
}