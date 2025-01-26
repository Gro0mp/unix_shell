#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

//accept up to 16 command-line arguments
#define MAXARG 16

//allow up to 64 environment variables
#define MAXENV 64

//keep the last 500 commands in history
#define HISTSIZE 500

//accept up to 1024 bytes in one command
#define MAXLINE 1024

// Global array to store enviroment variables
char *environmentVariables[MAXENV];

// Global integer k to keep track of how many environment variables are present
int k = 0;

// Global array to store previous commands
char history[HISTSIZE][MAXLINE];

// Counter to keep track of valid slots in history
int histSlots = 0;

// In and Out Files for redirection.
char *inFile;
char *outFile;

static char **parseCmd(char cmdLine[]) {
  char **cmdArg, *ptr;
  int i;

  //(MAXARG + 1) because the list must be terminated by a NULL ptr
  cmdArg = (char **) malloc(sizeof(char *) * (MAXARG + 1));
  if (cmdArg == NULL) {
    perror("parseCmd: cmdArg is NULL");
    exit(1);
  }
  for (i = 0; i <= MAXARG; i++) //note the equality
    cmdArg[i] = NULL;
  i = 0;
  ptr = strsep(&cmdLine, " ");
  while (ptr != NULL) {
    // (strlen(ptr) + 1)
    cmdArg[i] = (char *) malloc(sizeof(char) * (strlen(ptr) + 1));

    if (strcmp(ptr, "<") == 0) {
      inFile = strsep(&cmdLine, " ");
    } else if (strcmp(ptr, ">") == 0) {
      outFile = strsep(&cmdLine, " ");
    }

    if (cmdArg[i] == NULL) {
      perror("parseCmd: cmdArg[i] is NULL");
      exit(1);
    }
	    strcpy(cmdArg[ i++ ], ptr);
    if (i == MAXARG)
      break;
    ptr = strsep(&cmdLine, " ");
  }
  return(cmdArg);
}

void implementSetEnv(char *variable, char *path) {
  char tmpStr[MAXLINE], *myPath, *justPath;
  // Ensure enough space for new variable and path, including the
  // "=" and null terminater.
  int newEnvLength = (strlen(variable) + strlen(path) + 2);
  for (int i = 0; i < k; i++) {
    strcpy(tmpStr, environmentVariables[i]);
    myPath = tmpStr;
    justPath = strsep(&myPath, "=");
    if (strcmp(justPath, variable) == 0) {
      // Memory allocate enough space at the correct environment variable
      // and free the current contents. Then, print new contents into the
      // freed slot.
      free(environmentVariables[i]);
      environmentVariables[i] = malloc(newEnvLength);
      snprintf(environmentVariables[i], newEnvLength, "%s=%s", variable, path);
      return;
    }
  }

  // If it is a new environment variable, malloc the necessary space in
  // environmentVariables and add it. Then, increment the number of ev's
  // and set the last element to the null terminater.
  if (k < MAXENV) {
    environmentVariables[k] = malloc(newEnvLength);
    snprintf(environmentVariables[k++], newEnvLength, "%s=%s", variable, path);
    environmentVariables[k] = '\0';
  }
}

void implementUnsetEnv(char *variable) {
  char tmpStr[MAXLINE], *myPath, *justPath;
  for (int i = 0; i < k; i++) {
    strcpy(tmpStr, environmentVariables[i]);
    myPath = tmpStr;
    justPath = strsep(&myPath, "=");
    // If the variable that the user wants to delete matches
    // with an environment variable present, free that variable
    // and shift all variables after upwards.
    if (strcmp(justPath, variable) == 0) {
      free(environmentVariables[i]);
      for (int j = i; j < k - 1; j++) {
	environmentVariables[j] = environmentVariables[j + 1];
      }
    }
  }
  // Set null terminater at the end.
  environmentVariables[k--] = NULL;
  return;
}


void implementChangeDirectory(char **cmdArg) {
  // Variables to store current and old directories,
  // and the home directory.
  char cwd[MAXLINE];
  char oldcwd[MAXLINE];
  char *homeDir = getenv("HOME");
  char *oldPath = getcwd(oldcwd, sizeof(oldcwd));
  // If the second parameter from the user use NULL or ~, change
  // to home directory and update PWD and OLDPWD.
  if (cmdArg[1] == NULL || strcmp(cmdArg[1], "~") ==  0) {
    if (chdir(homeDir) == 0) {
      implementSetEnv("PWD", homeDir);
      implementSetEnv("OLDPWD", oldPath);
    }
  } else {
    // Change the directory and get the value of the current working directory.
    // Then change PWD and OLDPWD accordingly.
    if (chdir(cmdArg[1]) == 0) {
      char *newDirectory = getcwd(cwd, sizeof(cwd));
      if (newDirectory != NULL) {
	implementSetEnv("PWD", newDirectory);
	implementSetEnv("OLDPWD", oldPath);
      }
    }
  }
}


void addHistory(char *command) {
  // If the max size has been reached in the history array (500), then
  // make space in history.
  if (histSlots == HISTSIZE) {
    memmove(history, history + 1, (HISTSIZE - 1) * sizeof(char *));
    histSlots--;
  }
  // Copy the command to history and increment slot.
  strncpy(history[histSlots], command, MAXLINE - 1);
  history[histSlots][MAXLINE - 1] = '\0';
  histSlots++;
}

void implementHistory() {
  for (int i = 0; i < histSlots; i++) {
    printf("%d %s\n", i + 1, history[i]);
  }
}

void implementOtherCommands(char **cmdArg, pid_t pid) {
  char *pathEnv = getenv("PATH");
  char *pathCopy = strdup(pathEnv);
  char *dir;
  char absolutePath[MAXLINE];
  int found = 0;

  // While the PATH can be seperated, check
  while ((dir = strsep(&pathCopy, ":")) != NULL) {
    // Redirect printf of directort and cmdArg to absolute path.
    // And check if it is a valid command.
    snprintf(absolutePath, sizeof(absolutePath), "%s/%s", dir, cmdArg[0]);
    //printf("%s\n", absolutePath);
    if (access(absolutePath, X_OK) == 0) {
      found = 1;
      pid = fork();
      if (pid == 0) {
	if (inFile) {
	  int fdIn = open(inFile, O_RDONLY);
	  dup2(fdIn, STDIN_FILENO);
	  close(fdIn);
	}
	if (outFile) {
	  int fdOut = open(outFile, O_WRONLY | O_CREAT);
	  dup2(fdOut, STDOUT_FILENO);
	  close(fdOut);
	}
        //printf("%s\n", cmdArg[1]);
	execv(absolutePath, cmdArg);
	perror("Error executing command\n");
	exit(1);
      } else if (pid > 0) {
	int status;
	waitpid(pid, &status, 0);
      } else {
	perror("Error forking process\n");
      }
      break;
    }
  }
  if (!found) {
    printf("Command not found\n");
  }
}


int main(int argc, char *argv[], char *envp[]) {
  char cmdLine[MAXLINE], **cmdArg;
  int status, i, debug;
  pid_t pid;

  // Store environment varaibles into global character array.
  while (envp[k] != NULL) {
    environmentVariables[k] = strdup(envp[k]);
    k++;
  }
  environmentVariables[k] = NULL;

  debug = 0;
  i = 1;
  while (i < argc) {
    if (! strcmp(argv[i], "-d") )
      debug = 1;
    i++;
  }
  while (( 1 )) {
    printf("bsh> ");                      //prompt
    fgets(cmdLine, MAXLINE, stdin);       //get a line from keyboard
    cmdLine[strlen(cmdLine) - 1] = '\0';  //strip '\n'
    addHistory(cmdLine);		  // Add command to history

    inFile = NULL;
    outFile = NULL;

    cmdArg = parseCmd(cmdLine);

    if (debug) {
      i = 0;
      while (cmdArg[i] != NULL) {
	printf("\t%d (%s)\n", i, cmdArg[i]);
	i++;
      }
    }

    //built-in command exit
    if (strcmp(cmdArg[0], "exit") == 0) {
      if (debug)
	printf("exiting\n");
      break;
    }
    //built-in command env
    else if (strcmp(cmdArg[0], "env") == 0) {
      for (int i = 0; i < k; i++) {
        printf("%s\n", environmentVariables[i]);
      }
      printf("\n%d env variables\n", k);
    }
    //built-in command setenv
    else if (strcmp(cmdArg[0], "setenv") == 0) {
      implementSetEnv(cmdArg[1], cmdArg[2]);
    }
    //built-in command unsetenv
    else if (strcmp(cmdArg[0], "unsetenv") == 0) {
      implementUnsetEnv(cmdArg[1]);
    }
    //built-in command cd
    else if (strcmp(cmdArg[0], "cd") == 0) {
      implementChangeDirectory(cmdArg);
    }
    //built-in command history
    else if (strcmp(cmdArg[0], "history") == 0) {
      implementHistory();
    }

    //implement how to execute Minix commands here

    //the following is a template for using fork() and execv()
    //***remove this else case from your final code for bsh.c
    else {
      implementOtherCommands(cmdArg, pid);
      //printf("%s %s\n", inFile, outFile);
    }
    //***remove up to this point

    //clean up before running the next command
    i = 0;
    while (cmdArg[i] != NULL)
      free( cmdArg[i++] );
    free(cmdArg);
  }

  return 0;
}
