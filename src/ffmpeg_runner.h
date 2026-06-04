#ifndef _FFMPEG_RUNNER_
#define _FFMPEG_RUNNER_


#include <stddef.h>

/**
 * Gets the absolute path of a command using 'which' via IPC.
 *
 * @param command     The name of the executable (e.g., "ls").
 * @param output_path Buffer where the resulting absolute path will be stored.
 * @param max_len     The maximum size of the output_path buffer.
 * @return            1 if successful and found, 0 on failure or not found.
 */
int get_path_via_IPC(const char *command, char *output_buffer, size_t buffer_len);


#endif  // _FFMPEG_RUNNER_
