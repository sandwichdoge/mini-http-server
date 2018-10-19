#include <time.h>

/*system_output()
 *execute external program and return its output.
 *args          : array of strings of program to call and arguments to pass, must be null terminated
                      args[0] is the program to call, the rest are arguments.
 *output_sz : size of output buffer.
 *time_out  : time out in miliseconds. 
 *CALLER IS RESPONSIBLE FOR FREEING RETURNED MEMORY AFTER USE
 */
char* system_output(char **args, long *output_sz, int *retcode, int timeout_ms)
{
    int fds[2];
    int buf[4096];
    pipe(fds);  //fd0<->fd1

    pid_t pid;
    pid = fork();
    if (pid == 0) {  //is child
        dup2(fds[1], STDOUT_FILENO);  //point stdout to file table entry pointed to by fd1, cause all data written in stdout to be-
                                    //written to file table entry pointed to by fd1-
                                    //stdout is alias of fd1
        dup2(fds[1], STDERR_FILENO);  //point stderr to fd1 as well
        close(fds[1]);  //pipe is now fd0<->stdout, we can now close fd1
        execvp(args[0], args);  //child process will now be cannibalized
    }
    else {  //is parent
        clock_t timer_init = clock();
        clock_t timer_diff;
        int ret_code = -1;  //exit code of child process
        long bytes_read_total = 0;
        long bytes_read_last = 0;
        int brkflag = 0; //if flag is 1 when in loop, read buffer 1 last time then break
        pid_t pid_s;
        char *output = malloc(1);
        fcntl(fds[0], F_SETFL, O_NONBLOCK); /*non-blocking IO, https://stackoverflow.com/questions/8130922/processes-hang-on-read
                                                                since read() will hang when reading from empty pipe if this option is not set*/
        while (1) {
            pid_s = waitpid(pid, &ret_code, WNOHANG);
            if (pid_s > 0) brkflag = 1; //child exited, read from buf 1 last time then break
            bytes_read_last = read(fds[0], buf, sizeof(buf));
            if (bytes_read_last > 0) {
                output = realloc(output, bytes_read_total + bytes_read_last + 1);
                if (output == NULL) break; //out of memory
                memcpy(output + bytes_read_total, buf, bytes_read_last); //concatenate new data
                bytes_read_total += bytes_read_last;
                memset(buf, 0, sizeof(buf));
            }
            if (brkflag) {
                break;
            }
            timer_diff = (clock() - timer_init) * 1000 / CLOCKS_PER_SEC;
            if (timer_diff > timeout_ms) break;
        }
        close(fds[0]);
        close(fds[1]);

        *(output + bytes_read_total) = '\0';
        *retcode = ret_code;
        *output_sz = bytes_read_total;
        return output; //caller needs to free() this
    }
}