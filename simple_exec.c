// test with: gcc simple_exec.c -DTEST_EXEC_MAIN -g && valgrind --track-fds=yes ./a.out
// adapted from: https://stackoverflow.com/a/479103

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <stdarg.h>

enum PIPE_FILE_DESCRIPTORS
{
  READ_FD  = 0,
  WRITE_FD = 1
};

int runCommandArray(char** stdOut, int* stdOutByteCount, char* const* allArgs)
{
    int bufferSize = 256;
    char buffer[bufferSize + 1];

    int dataReadFromChildDefaultSize = bufferSize * 5;
    int dataReadFromChildSize = dataReadFromChildDefaultSize;
    int dataReadFromChildUsed = 0;
    char* dataReadFromChild = (char*)malloc(dataReadFromChildSize);


    int parentToChild[2];
    assert(pipe(parentToChild) == 0);

    int childToParent[2];
    assert(pipe(childToParent) == 0);

    pid_t pid;
    switch( pid = fork() )
    {
        case -1:
        {
            assert(0 && "Fork failed");
        }

        case 0: // child
        {
            assert(dup2(parentToChild[READ_FD ], STDIN_FILENO ) != -1);
            assert(dup2(childToParent[WRITE_FD], STDOUT_FILENO) != -1);
            assert(dup2(childToParent[WRITE_FD], STDERR_FILENO) != -1);

            // unused
            assert(close(parentToChild[WRITE_FD]) == 0);
            assert(close(childToParent[READ_FD ]) == 0);
            
            const char* command = allArgs[0];
            execvp(command, allArgs);

            // We should finish closing parentToChild and childToParent here, 
            // but execvp will never return anyway.

            assert(0 && "This line should never be reached!!!");
        }


        default: // parent
        {
            // unused
            assert(close(parentToChild[READ_FD]) == 0);
            assert(close(childToParent[WRITE_FD]) == 0);

            while(1)
            {
                ssize_t bytesRead = 0;
                switch(bytesRead = read(childToParent[READ_FD], buffer, bufferSize))
                {
                    case 0: // End-of-File, or non-blocking read.
                    {
                        int status = 0;
                        assert(waitpid(pid, &status, 0) == pid);
                        
                        // done with these now
                        assert(close(parentToChild[WRITE_FD]) == 0);
                        assert(close(childToParent[READ_FD]) == 0);
                       
                        // free any un-needed memory with realloc + add a null terminator for convenience
                        dataReadFromChild = (char*)realloc(dataReadFromChild, dataReadFromChildUsed + 1);
                        dataReadFromChild[dataReadFromChildUsed] = '\0';
                        
                        if(stdOut != NULL)
                            *stdOut = dataReadFromChild;
                        if(stdOutByteCount != NULL)
                            *stdOutByteCount = dataReadFromChildUsed;
                        return WEXITSTATUS(status);
                    }
                    case -1:
                    {
                        assert(0 && "read() failed");
                    }

                    default:
                    {
                        if(dataReadFromChildUsed + bytesRead + 1 >= dataReadFromChildSize)
                        {
                            dataReadFromChildSize += dataReadFromChildDefaultSize;
                            dataReadFromChild = (char*)realloc(dataReadFromChild, dataReadFromChildSize);
                        }

                        memcpy(dataReadFromChild + dataReadFromChildUsed, buffer, bytesRead);
                        dataReadFromChildUsed += bytesRead;
                        break;
                    }
                }
            }
        }
    }
}

int runCommand(char** stdOut, int* stdOutByteCount, char* command, ...)
{
    va_list vl;
    va_start(vl, command);
      
    char* currArg = NULL;
      
    int allArgsInitialSize = 16;
    int allArgsSize = allArgsInitialSize;
    char** allArgs = (char**)malloc(sizeof(char*) * allArgsSize);
    allArgs[0] = command;
        
    int i = 1;
    do
    {
        currArg = va_arg(vl, char*);
        allArgs[i] = currArg;

        i++;

        if(i >= allArgsSize)
        {
            allArgsSize += allArgsInitialSize;
            allArgs = (char**)realloc(allArgs, sizeof(char*) * allArgsSize);
        }

    } while(currArg != NULL);

    va_end(vl);

    int retval = runCommandArray(stdOut, stdOutByteCount, allArgs);
    free(allArgs);
    return retval;
}

#ifdef TEST_EXEC_MAIN
int main(int argc, char** argv)
{
    // get stdout, and return code
    char* stdOut = NULL;
    int byteCount = 0;
    int exitCode = runCommand(&stdOut, &byteCount, "ls", "-l", (char*)NULL);
    printf("EXIT CODE: %d, STDOUT SIZE: %d bytes, STDOUT: %s", exitCode, byteCount, stdOut);
    free(stdOut);

    // or don't
    runCommand(NULL, NULL, "touch", "blah", (char*)NULL); 

    return 0;
}
#endif
