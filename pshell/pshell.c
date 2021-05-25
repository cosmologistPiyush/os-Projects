#include"helper.h"
#include"fcntl.h" 
#include<sys/types.h>
#include<sys/wait.h>
#include<assert.h>

static char** cmd;
char error_msg[30] = "An error has occured\n";

void defaultCommands(char** path) { //creates an absolute path for shell commands

    for(char **it = path; (it) && (*it); ++it) { //pointer variable iterates over path array
        size_t size = (size_t)(strlen(*cmd) + strlen(*it) + 2); // size of the full absolute path
        char check[size];
        snprintf(check, size, "%s/%s", *it, *cmd); // writes the new absolute path in check variable
        if(!access(check, X_OK)) { //checks if the file is executable
            free(*cmd);
            *cmd = strdup(check); // replaces file name with absolute path
            break;
        }
    }
}

int trimSpaces(char** input, size_t end) {
    size_t j, count = 1;
    while(j < end) {
        if(strcmp(input[j], "") == 0)
            j++;
        else {
            count++;
            if((cmd = realloc(cmd, count*sizeof(char*))) == NULL) {
                printf("e: %i\n", errno);
                perror("realloc failed while trimming spaces");
                exit(1);
            }
            cmd[count-2] = strdup(input[j++]);
        }
    }
    cmd[count-1] = NULL;
    for(j=0; j<end; j++)
        free(input[j]);
    return count;
}

int main(int argc, char *argv[]) {
    char *line=NULL;
    size_t linecap=0, count, i;
    ssize_t linelen=0;
    int status;
    pid_t pid;
    cmd = (char**) malloc(sizeof(char*)); 
    char** input = malloc(sizeof(char*));
    char **path = (char**) malloc(sizeof(char*));
    if((!input) || (!cmd) || (!path)) {
        printf("e: %i\n", errno);
        perror("malloc in the beginning of main failed");
        exit(1);
    }

    if(argc == 1) {
        printf("wish> ");
        while((linelen = getline(&line, &linecap, stdin)) != EOF) {
            i=count=1;
            if(*line == '\n') { //provides default shell like enter key functionality
                printf("wish> ");
                continue;
            }
            if(line[linelen-1] == '\n')
                line[linelen-1] = '\0';
            if((linelen == 5) && (strcmp(line, "exit") == 0)) //exit command
                exit(0);
            //*path = strdup(getcwd(NULL, 0)); // adds current working directory to path

            do {
                count++; // counter for number of char pointers
                if((input = realloc(input, count*sizeof(char*))) == NULL) {
                    printf("e: %i\n", errno);
                    perror("realloc failed to seperate input");
                    write(STDERR_FILENO, error_msg, 30);
                    exit(1);
                }
                input[count-2] = strdup(strsep(&line, " ")); // seperate contents of line by space pointed to by an array of char pointers
            } while(line !=  NULL);

            count = trimSpaces(input, count-1);

            if(!((cmd[0][0] == '/') && (cmd[0][0] == '.'))) { // if not a full mentioned path for executable
                if(strcmp(*cmd, "cd") == 0) {
                    if(cd(cmd, count-1) < 0)
                        perror("cd failed");
                    printf("wish> ");
                    continue;
                }
                else if(strcmp(*cmd, "path") == 0) {
                    if(makePath(cmd, &path, count-1) < 0)
                        perror("setting path failed");
                    printf("wish> ");
                    continue;
                }
                else
                    defaultCommands(path); // i.e a shell command
            }

            i=1;
            for(;i<count-1; i++) {
                if(strcmp(cmd[i], ">") == 0) {
                    int newOp;
                    if((newOp = open(cmd[i+1], O_CREAT|O_TRUNC|O_WRONLY, S_IWUSR)) != -1) {
                        dup2(STDOUT_FILENO, STDERR_FILENO);
                        dup2(newOp, STDOUT_FILENO);
                        assert(newOp = 1);
                        break;
                    }
                    else
                        perror("File opening error");

                }
            }

            pid = fork();
            if(pid < 0) {
                puts("wish: Fork error");
                exit(1);
            } else if(pid == 0) {
                execv(*cmd, cmd);
                printf("e: %i\n", errno);
                perror("exec error");
                exit(127);
            } else {
                pid = waitpid(pid, &status, 0);
                if(pid > 0) {
                    printf("wish> ");
                    for(size_t i=0; i<count-1; i++)
                        free(cmd[i]);
                } else
                    perror("Error");

                //if(i != 0) {
                int stout = fileno(freopen("/dev/null", "w", stdout));
                dup2(stout, 1);
                assert(stout == 1);
                int sterr = fileno(freopen("/dev/null", "w", stderr));
                dup2(sterr, 2);
                assert(sterr == 2);
                //}

            }
        }
    } else if(argc == 2) {
        FILE* cmd = fopen(argv[1], "r");
        if(cmd == NULL) {
            write(STDERR_FILENO, error_msg, 30);
            exit(1);
        }

        return 0;
    } else {
        puts("errro");
        exit(1);
    }

    return 0;
}

