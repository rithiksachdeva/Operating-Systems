#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX 512

int main(void)
{
    char cmd[CMDLINE_MAX];
    char* dirs[CMDLINE_MAX];
    int dirs_tracker = 0;
    restart:
    while (1)
    {
        char *nl;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line and check for bool conditions*/
        fgets(cmd, CMDLINE_MAX, stdin);
        int cmd_iter = 0;
        /* Instead of using a struct for parsing we decided to just use boolean variables that check what special symbols are
        present in the command and we have conditional statements that will run the command based on what symbol is found
        the default is a command with no symbols */
        int phase12 = 1;
        int phase4 = 0;
        int phase5c = 0;
        int phase5_1 = 0;
        int phase5_2 = 0;
        int phase5_3 = 0;
        int phase6_1 = 0;
        while (cmd[cmd_iter] != '\0')
        {
            if (cmd[cmd_iter] == '>')
            {
                phase4 = 1;
                phase12 = 0;
            }
            if (cmd[cmd_iter] == '<')
            {
                phase6_1 = 1;
                phase12 = 0;
            }
            if (cmd[cmd_iter] == '|')
            {
                if (phase4 == 1)
                {
                    fprintf(stderr, "Error: mislocated output redirection\n");
                    goto restart;
                }
                if (phase6_1 == 1)
                {
                    fprintf(stderr, "Error: mislocated input redirection\n");
                    goto restart;
                }
                phase5c++;
                if (phase5c == 1)
                    phase5_1 = 1;
                if (phase5c == 2)
                {
                    phase5_1 = 0;
                    phase5_2 = 1;
                }
                if (phase5c == 3)
                {
                    phase5_1 = 0;
                    phase5_2 = 0;
                    phase5_3 = 1;
                }
                phase12 = 0;
            }
            cmd_iter++;
        }
        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* Builtin command: exit, pwd, and cd */
        if (!strcmp(cmd, "exit"))
        {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed '%s' [0]\n", cmd);
            break;
        }

        if (!strcmp(cmd, "pwd"))
        {
            char wd[CMDLINE_MAX];
            fprintf(stdout, "%s\n", getcwd(wd, CMDLINE_MAX));
            fprintf(stderr, "+ completed '%s' [0]\n", cmd);
            fflush(stdout);
            continue;
        } // checks if pwd is the command and uses getcwd() to print wd to stdout

        if (cmd[0] == 'c' && cmd[1] == 'd')
        {
            char cmd_cpy[CMDLINE_MAX];
            memcpy(cmd_cpy, cmd, strlen(cmd) + 1);
            char *argument = strtok(cmd, " ");
            argument = strtok(NULL, " ");
            int retval = chdir(argument);
            if (retval == -1)
            {
                fprintf(stderr, "Error: cannot cd into directory\n");
                fprintf(stderr, "+ completed '%s' [1]\n", cmd_cpy);
                continue;
            }
            fprintf(stderr, "+ completed '%s' [0]\n", cmd_cpy);
            continue;
        } // checks if cd is the command and then makes a copy of the cmd input to print to stderr and executes cd with chdir()

        /* Phase 6.2: */

        if (cmd[0] == 'p' && cmd[1] == 'u' && cmd[2] == 's' && cmd[3] == 'h' && cmd[4] == 'd')
        {
            char wd[CMDLINE_MAX];
            char cmd_cpy[CMDLINE_MAX];
            memcpy(cmd_cpy, cmd, strlen(cmd) + 1);
            char *argument = strtok(cmd, " ");
            argument = strtok(NULL, " ");
            dirs_tracker++;
            dirs[dirs_tracker] = getcwd(wd, CMDLINE_MAX);
            int retval = chdir(argument);
            if (retval == -1)
            {
                dirs[dirs_tracker] = NULL;
                dirs_tracker--;
                fprintf(stderr, "Error: no such directory\n");
                fprintf(stderr, "+ completed '%s' [1]\n", cmd_cpy);
                continue;
            }
            fprintf(stderr, "+ completed '%s' [0]\n", cmd_cpy);
            continue;
        }
        if (!strcmp(cmd, "popd"))
        {
            if (dirs_tracker > 0)
            {
                chdir("..");
                dirs_tracker--;
                fprintf(stderr, "+ completed '%s' [0]\n", cmd);
                continue;
            }
            else
            {
                fprintf(stderr, "Error: directory stack empty\n");
                fprintf(stderr, "+ completed '%s' [1]\n", cmd);
                continue;
            }
        }
        if (!strcmp(cmd, "dirs"))
        {
            char wd[CMDLINE_MAX];
            fprintf(stdout, "%s\n", getcwd(wd, CMDLINE_MAX));
            if (dirs_tracker > 0)
            {
                for (int i = dirs_tracker; i != 0; i--)
                {
                    fprintf(stdout, "%s\n", dirs[i]);
                }
            }
            fprintf(stderr, "+ completed '%s' [0]\n", cmd);
            continue;
        }

        /* Phase 1 and 2: Running simple commands the hard way and handling arguments */
        if (phase12)
        {
            int errorfd[2];
            pipe(errorfd);
            /* Decided to use a pipe to communicate between parent and process child to know whether an error was encountered
            while parsing in the child process and tell parent not to print to stderr */
            int id = fork();
            int retval;
            if (id == -1)
            {
                return 1;
            }
            if (id == 0)
            {                                      // child process runs the command for phase 1
                char *args[CMDLINE_MAX];           // initialize an array of arguments that will be passed to execvp()
                char *argument = strtok(cmd, " "); // parse individual arguments from cmd using strtok

                int count = 0;
                int arg_max = 16;
                args[count] = argument;
                count++;
                while (argument != NULL)
                { // the while loop that parses through the string and places argument into args whilst incrementing counter
                    argument = strtok(NULL, " ");
                    args[count] = argument;
                    count++;
                }
                if (count > arg_max)
                {
                    fprintf(stderr, "Error: too many process arguments\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                retval = execvp(cmd, args);
                if (retval == -1)
                {
                    fprintf(stderr, "Error: command not found\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                close(errorfd[0]);
                int error = 0;
                write(errorfd[1], &error, sizeof(int));
                close(errorfd[1]);
            }
            else
            { // parent process waits for the child to finish executing
                int status;
                wait(&status);
                close(errorfd[1]);
                int errorcheck;
                read(errorfd[0], &errorcheck, sizeof(int));
                close(errorfd[0]);
                if (errorcheck)
                {
                    fprintf(stderr, "+ completed '%s' [1]\n", cmd);
                }
                else
                {
                    fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
                }
            }
            continue;
        }

        /* Phase 4:  Output Redirection */
        if (phase4)
        {
            int errorfd[2];
            pipe(errorfd);
            /* Decided to use a pipe to communicate between parent and process child to know whether an error was encountered
            while parsing in the child process and tell parent not to print to stderr */
            int id = fork();
            if (id == -1)
            {
                return 1;
            }
            if (id == 0)
            { // child process runs the command for phase 3
                int buffxreset = 0;
                /* After some debugging was done it seeemed a buffer reset variable was necessary to undo the pointer arithmetic
                done in order to check for spacing around the "</>" symbols */
                char ORcmd[CMDLINE_MAX]; // Output Redirection cmd, we store whatever is located on the rhs of cmd here
                char *args[CMDLINE_MAX];
                int iterx = 0;
                while (cmd[iterx] != '>')
                {
                    ORcmd[iterx] = cmd[iterx];
                    iterx++;
                }

                // We need to remove the newline character again or else it prints weird stuff to Output file
                nl = strchr(ORcmd, '\n');
                if (nl)
                    *nl = '\0';

                /*Thought Process here is that if we know that output redirection needs to be performed then we must first take the command input
                and then seperate it based on the ">" character since we know on the LHS we have the actual command and arguments, and then on the RHS
                what we have is the file that we will redirect output to.
                */
                char *bufferx = strtok(cmd, ">"); // buffer to store the command that needs to be run before ">" symbol
                bufferx = strtok(NULL, ">");      // Basically just call buffer twice so it only contains the RHS of ">"
                if (bufferx == NULL)
                { // Handles error by printing to stderr and then communicating via the pipe that the child encountered and error
                    fprintf(stderr, "Error: no output file\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                while (*bufferx == ' ')
                { // the buffer arithmetic performed to ignore ' ' spaces between files
                    bufferx++;
                    buffxreset = 1;
                }
                char *argument = strtok(ORcmd, " ");
                int count = 0;
                args[count] = argument;
                count++;
                while (argument != NULL)
                {
                    argument = strtok(NULL, " ");
                    args[count] = argument;
                    count++;
                }
                // Main Output Redirection Logic
                int OR_FD = open(bufferx, O_WRONLY | O_CREAT, 0777); // Followed CodeVault video on YT where it explained the 0777 parameter allows access by anyone
                if (OR_FD == -1)
                { // Handles error by printing to stderr and then communicating via the pipe that the child encountered and error
                    fprintf(stderr, "Error: cannot open output file\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                if (OR_FD == -1)
                {
                    fprintf(stderr, "Error: cannot open output file\n");
                    break;
                }
                fflush(stdout);
                dup2(OR_FD, STDOUT_FILENO);
                close(OR_FD);
                close(errorfd[0]);
                int error = 0;
                write(errorfd[1], &error, sizeof(int));
                close(errorfd[1]);
                execvp(ORcmd, args);
                if (buffxreset == 1)
                {
                    bufferx--;
                    buffxreset = 0;
                }
            }
            else
            { // parent process waits for the child to finish executing
                int status;
                wait(&status);
                close(errorfd[1]);
                int errorcheck;
                read(errorfd[0], &errorcheck, sizeof(int));
                close(errorfd[0]);
                if (errorcheck)
                {
                    continue;
                }
                else
                {
                    fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
                }
            }
            continue;
        }

        /* Phase 6.1:  Input Redirection */
        if (phase6_1)
        {
            /* This phase was almost identical to output redirection, the only differences were variable names and the pipe */
            int errorfd[2];
            pipe(errorfd);
            int id = fork();
            if (id == -1)
            {
                return 1;
            }
            if (id == 0)
            {
                int buffreset = 0;
                char IRcmd[CMDLINE_MAX];
                char *args[CMDLINE_MAX];
                int iter = 0;
                while (cmd[iter] != '<')
                {
                    IRcmd[iter] = cmd[iter];
                    iter++;
                }
                nl = strchr(IRcmd, '\n');
                if (nl)
                    *nl = '\0';

                char *buffer = strtok(cmd, "<"); // buffer to store the command that needs to be run before "<" symbol
                buffer = strtok(NULL, "<");      // Basically just call buffer twice so it only contains the RHS of "<"
                if (buffer == NULL)
                {
                    fprintf(stderr, "Error: no output file\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                while (*buffer == ' ')
                {
                    buffer++;
                    buffreset = 1;
                }
                char *argument = strtok(IRcmd, " ");

                int count = 0;
                args[count] = argument;
                count++;
                while (argument != NULL)
                {
                    argument = strtok(NULL, " ");
                    args[count] = argument;
                    count++;
                }

                // Main Input Redirection Logic
                int IR_FD = open(buffer, O_RDONLY, 0777); // Followed CodeVault video on YT where it explained the 0777 parameter allows access by anyone
                if (IR_FD == -1)
                {
                    fprintf(stderr, "Error: cannot open output file\n");
                    close(errorfd[0]);
                    int error = 1;
                    write(errorfd[1], &error, sizeof(int));
                    close(errorfd[1]);
                    break;
                }
                dup2(IR_FD, STDIN_FILENO);
                close(IR_FD);
                close(errorfd[0]);
                int error = 0;
                write(errorfd[1], &error, sizeof(int));
                close(errorfd[1]);
                execvp(IRcmd, args);
                fflush(stdout);
                if (buffreset == 1)
                {
                    buffer--;
                    buffreset = 0;
                }
            }
            else
            { // parent process waits for the child to finish executing
                int status;
                wait(&status);
                close(errorfd[1]);
                int errorcheck;
                read(errorfd[0], &errorcheck, sizeof(int));
                close(errorfd[0]);
                if (errorcheck)
                {
                    continue;
                }
                else
                {
                    fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
                }
            }
            continue;
        }

        /* Phase 5.1: Piping with 1 symbol */
        if (phase5_1)
        {
            char cmdlist[2][CMDLINE_MAX];
            int cmdnum = 0;
            int iter = 0;
            int itercmdlist = 0;
            while (cmd[iter] != '\0')
            {
                while (cmd[iter] != '|' && cmd[iter] != '\0')
                {
                    cmdlist[cmdnum][itercmdlist] = cmd[iter];
                    iter++;
                    itercmdlist++;
                }
                iter++;
                if (cmd[iter] == ' ')
                {
                    iter++;
                }
                itercmdlist = 0;
                cmdnum++;
            }

            char *args1[CMDLINE_MAX]; // initialize an array of arguments
            char *args2[CMDLINE_MAX];

            char *argument1 = strtok(cmdlist[0], " "); // parse arguments from cmd using strtok

            int count1 = 0;
            args1[count1] = argument1;
            count1++;
            while (argument1 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument1 = strtok(NULL, " ");
                args1[count1] = argument1;
                count1++;
            }

            char *argument2 = strtok(cmdlist[1], " "); // parse arguments from cmd using strtok
            int count2 = 0;
            args2[count2] = argument2;
            count2++;
            while (argument2 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument2 = strtok(NULL, " ");
                args2[count2] = argument2;
                count2++;
            }

            int fd[2]; // pipe for first 2 processes
            pipe(fd);
            int p1 = fork();
            if (p1 == 0)
            {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(cmdlist[0], args1);
            }
            int p2 = fork();
            if (p2 == 0)
            {
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(cmdlist[1], args2);
            }
            close(fd[0]);
            close(fd[1]);
            int status1;
            int status2;
            waitpid(p1, &status1, 0);
            waitpid(p2, &status2, 0);
            fprintf(stderr, "+ completed '%s' [%d][%d]\n", cmd, WEXITSTATUS(status1), WEXITSTATUS(status2));
            continue;
        }

        /* Phase 5.2: Piping with 2 symbol */
        if (phase5_2)
        {
            char cmdlist[3][CMDLINE_MAX];
            int cmdnum = 0;
            int iter = 0;
            int itercmdlist = 0;
            while (cmd[iter] != '\0')
            {
                while (cmd[iter] != '|' && cmd[iter] != '\0')
                {
                    cmdlist[cmdnum][itercmdlist] = cmd[iter];
                    iter++;
                    itercmdlist++;
                }
                iter++;
                if (cmd[iter] == ' ')
                {
                    iter++;
                }
                itercmdlist = 0;
                cmdnum++;
            }

            char *args1[CMDLINE_MAX];
            char *args2[CMDLINE_MAX];
            char *args3[CMDLINE_MAX];

            char *argument1 = strtok(cmdlist[0], " "); // parse arguments from cmd using strtok

            int count1 = 0;
            args1[count1] = argument1;
            count1++;
            while (argument1 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument1 = strtok(NULL, " ");
                args1[count1] = argument1;
                count1++;
            }

            char *argument2 = strtok(cmdlist[1], " "); // parse arguments from cmd using strtok
            int count2 = 0;
            args2[count2] = argument2;
            count2++;
            while (argument2 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument2 = strtok(NULL, " ");
                args2[count2] = argument2;
                count2++;
            }

            char *argument3 = strtok(cmdlist[2], " "); // parse arguments from cmd using strtok
            int count3 = 0;
            args3[count3] = argument3;
            count3++;
            while (argument3 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument3 = strtok(NULL, " ");
                args3[count3] = argument3;
                count3++;
            }

            int fd0[2];
            pipe(fd0);
            int fd1[2];
            pipe(fd1);
            int p1 = fork();
            if (p1 == 0)
            {
                dup2(fd0[1], STDOUT_FILENO);
                close(fd0[0]);
                close(fd0[1]);
                execvp(cmdlist[0], args1);
            }
            int p2 = fork();
            if (p2 == 0)
            {
                dup2(fd0[0], STDIN_FILENO);
                close(fd0[0]);
                close(fd0[1]);
                dup2(fd1[1], STDOUT_FILENO);
                close(fd1[0]);
                close(fd1[1]);
                execvp(cmdlist[1], args2);
            }
            int p3 = fork();
            if (p3 == 0)
            {
                close(fd0[0]);
                close(fd0[1]);
                dup2(fd1[0], STDIN_FILENO);
                close(fd1[0]);
                close(fd1[1]);
                execvp(cmdlist[2], args3);
            }

            close(fd0[0]);
            close(fd0[1]);
            close(fd1[0]);
            close(fd1[1]);
            int status1;
            int status2;
            int status3;
            waitpid(p1, &status1, 0);
            waitpid(p2, &status2, 0);
            waitpid(p3, &status3, 0);
            fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n", cmd, WEXITSTATUS(status1), WEXITSTATUS(status2), WEXITSTATUS(status3));
            continue;
        }

        /* Phase 5.3: Piping with 3 symbol */
        if (phase5_3)
        {
            char cmdlist[4][CMDLINE_MAX];
            int cmdnum = 0;
            int iter = 0;
            int itercmdlist = 0;
            while (cmd[iter] != '\0')
            {
                while (cmd[iter] != '|' && cmd[iter] != '\0')
                {
                    cmdlist[cmdnum][itercmdlist] = cmd[iter];
                    iter++;
                    itercmdlist++;
                }
                iter++;
                if (cmd[iter] == ' ')
                {
                    iter++;
                }
                itercmdlist = 0;
                cmdnum++;
            }

            char *args1[CMDLINE_MAX];
            char *args2[CMDLINE_MAX];
            char *args3[CMDLINE_MAX];
            char *args4[CMDLINE_MAX];

            char *argument1 = strtok(cmdlist[0], " "); // parse arguments from cmd using strtok

            int count1 = 0;
            args1[count1] = argument1;
            count1++;
            while (argument1 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument1 = strtok(NULL, " ");
                args1[count1] = argument1;
                count1++;
            }

            char *argument2 = strtok(cmdlist[1], " "); // parse arguments from cmd using strtok
            int count2 = 0;
            args2[count2] = argument2;
            count2++;
            while (argument2 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument2 = strtok(NULL, " ");
                args2[count2] = argument2;
                count2++;
            }

            char *argument3 = strtok(cmdlist[2], " "); // parse arguments from cmd using strtok
            int count3 = 0;
            args3[count3] = argument3;
            count3++;
            while (argument3 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument3 = strtok(NULL, " ");
                args3[count3] = argument3;
                count3++;
            }

            char *argument4 = strtok(cmdlist[3], " "); // parse arguments from cmd using strtok
            int count4 = 0;
            args4[count4] = argument4;
            count4++;
            while (argument4 != NULL)
            { // the while loop that parses through the string and places argument into args whilst incrementing counter
                argument4 = strtok(NULL, " ");
                args4[count4] = argument4;
                count4++;
            }

            int fd0[2];
            pipe(fd0);

            int fd1[2];
            pipe(fd1);

            int fd2[2];
            pipe(fd2);

            int p1 = fork();
            if (p1 == 0)
            {
                dup2(fd0[1], STDOUT_FILENO);
                close(fd0[0]);
                close(fd0[1]);
                execvp(cmdlist[0], args1);
            }
            int p2 = fork();
            if (p2 == 0)
            {
                dup2(fd0[0], STDIN_FILENO);
                close(fd0[0]);
                close(fd0[1]);
                dup2(fd1[1], STDOUT_FILENO);
                close(fd1[0]);
                close(fd1[1]);
                execvp(cmdlist[1], args2);
            }
            int p3 = fork();
            if (p3 == 0)
            {
                close(fd0[0]);
                close(fd0[1]);
                dup2(fd1[0], STDIN_FILENO);
                close(fd1[0]);
                close(fd1[1]);
                dup2(fd2[1], STDOUT_FILENO);
                close(fd2[0]);
                close(fd2[1]);
                execvp(cmdlist[2], args3);
            }
            int p4 = fork();
            if (p4 == 0)
            {
                close(fd0[0]);
                close(fd0[1]);
                close(fd1[0]);
                close(fd1[1]);
                dup2(fd2[0], STDIN_FILENO);
                close(fd2[0]);
                close(fd2[1]);
                execvp(cmdlist[3], args4);
            }

            close(fd0[0]);
            close(fd0[1]);
            close(fd1[0]);
            close(fd1[1]);
            close(fd2[0]);
            close(fd2[1]);
            int status1;
            int status2;
            int status3;
            int status4;
            waitpid(p1, &status1, 0);
            waitpid(p2, &status2, 0);
            waitpid(p3, &status3, 0);
            waitpid(p4, &status4, 0);
            fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n", cmd, WEXITSTATUS(status1), WEXITSTATUS(status2), WEXITSTATUS(status3), WEXITSTATUS(status4));
            continue;
        }
    }

    return EXIT_SUCCESS;
}
