//
// Created by shane on 2/19/17.
//

#ifndef COMP8005_ASSN2_LOGFILE_H
#define COMP8005_ASSN2_LOGFILE_H

/**
 * Opens a file for logging. If the file exists, it will be overwritten.
 * This also sets signal handlers for every non-fatal exception except for
 * SIGINT, SIGQUIT and SIGTERM and flushes the log on these signals.
 *
 * @param name The name of the file to open for logging.
 * @return 0 on success, -1 on failure with errno set appropriately.
 */
int log_open(char const* name);

/**
 * Logs the given message to the log file. Blocks until the message is logged or an error occurs.
 *
 * @param message The message to log.
 * @return 0 on success, -1 on failure with errno set appropriately.
 */
int log_msg(char const *message);

/**
 * Tries to flush the current log contents immediately.
 *
 * @return 0 on success, -1 on failure with errno set appropriately.
 */
int log_flush();

/**
 * Closes the log file.
 * @return 0 on success, -1 on failure. Not that you'll be checking it.
 */
int log_close();

#endif //COMP8005_ASSN2_LOGFILE_H
