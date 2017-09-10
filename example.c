// Test with: gcc example.c -g && valgrind --track-fds=yes ./a.out
// Will show one child process with leaks - that's fine, it's what happens if you try to run an executable
// that doesn't exist - it returns an error on a special error pipe and immediately exits.

#define SIMPLE_EXEC_IMPLEMENTATION
#include "simple_exec.h"


int main(int argc, char** argv)
{
    // get stdout, and return code
    char* stdOut = NULL;
    int byteCount = 0;
    int exitCode = 0;
    int err = runCommand(&stdOut, &byteCount, &exitCode, 0, "ls", "-l", (char*)NULL);
    assert(err == COMMAND_RAN_OK); // doesn't mean the command succeeded, just that invoking it didn't fail
    printf("EXIT CODE: %d, STDOUT SIZE: %d bytes, STDOUT: %s", exitCode, byteCount, stdOut);
    free(stdOut);

    // or don't
    runCommand(NULL, NULL, NULL, 0, "touch", "blah", (char*)NULL); 
    
    // This will show leaks in valgrind - see comment at top of file
    err = runCommand(NULL, NULL, NULL, 0, "thisCommandDoesntExist", (char*)NULL);
    assert(err == COMMAND_NOT_FOUND);

    return 0;
}
