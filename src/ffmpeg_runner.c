#include "ffmpeg_runner.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#include "log.h"
#include "project_config.h"

/*
 * Maximum number of argv slots for the rclone command we build.
 * Generous upper bound: base args + optional flags, all statically bounded.
 */

#define MAX_FFMPEG_ARGS 40

int run_ffmpeg_IPC(/* const Config_entity *entity, */ bool dry_run)
{
	return 0;
}


int get_path_via_IPC(const char *command, char *output_buffer, size_t buffer_len)
{
    char shell_cmd[PATH_MAX];

    // Format the shell execution string securely
    snprintf(shell_cmd, sizeof(shell_cmd), "which %s", command);

    // Create the IPC channel (pipe) to read ("r") the shell output
    FILE *pipe_fp = popen(shell_cmd, "r");
    if (pipe_fp == NULL) {
    	LOG_ERROR("IPC shell cmd failed: '%s'", shell_cmd);
        return 0; // IPC initialization failed
    }

    int found = 0;
    // Read the result from the pipe
    if (fgets(output_buffer, buffer_len, pipe_fp) != NULL) {
        // Strip the trailing newline character from the pipe output
        output_buffer[strcspn(output_buffer, "\n")] = '\0';

        // Ensure the output isn't empty
        if (strlen(output_buffer) > 0) {
        	// ADD LOGGINC HERE
            found = 1;
        }
    }

    LOG_DEBUG("IPC: shell [%s] = (%s)", shell_cmd, output_buffer);

    // Close the IPC channel and reap the child process
    pclose(pipe_fp);
    return found;
}
