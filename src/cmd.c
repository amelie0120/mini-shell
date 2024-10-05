// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "utils.h"

#define READ 0
#define WRITE 1

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO: Execute cd. */

	char *param = get_word(dir);

	if (param == NULL) {
		free(param);
		return chdir("~");
	}

	free(param);
	return chdir(dir->string);

	return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO: Execute exit/quit. */
	_exit(0);
	return 0; /* TODO: Replace with actual exit code. */
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	/* TODO: Sanity checks. */

	/* TODO: If builtin command, execute the command. */
	int fd;
	char *command = get_word(s->verb);

	if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
		free(command);
		return shell_exit();
	}


	if (strcmp(command, "cd") == 0) {
		if (s->out != NULL)
			fd = open(s->out->string, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

		if (s->err != NULL)
			fd = open(s->err->string, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

		free(command);

		int ret = shell_cd(s->params);

		if (s->out != NULL)
			close(fd);

		if (s->err != NULL)
			close(fd);

		return ret;
	}

	/* TODO: If variable assignment, execute the assignment and return
	 * the exit status.
	 */

	if (strchr(command, '=')) {
		char *value = get_word(s->verb->next_part->next_part);

		if (setenv(s->verb->string, value, 1) != 0) {
			free(value);
			return -1;
		}

		char *varValue = getenv(s->verb->string);

		if (varValue == NULL)
			varValue = "";
		else
			varValue = value;

		free(command);
		free(value);

		return 0;
	}

	/* TODO: If external command:
	 *   1. Fork new process
	 *     2c. Perform redirections in child
	 *     3c. Load executable in child
	 *   2. Wait for child
	 *   3. Return exit status
	 */

	int fdout, fderr, fdin;
	int status;

	__pid_t pid = fork();

	if (pid == -1)
		exit(-1);

	if (pid == 0) {
		if (get_word(s->in) != NULL) {
			fdin = open(get_word(s->in), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(fdin, 0);
		}

		if (get_word(s->out) != NULL) {
			if (s->io_flags != IO_OUT_APPEND)
				unlink(s->out->string);

			fdout = open(get_word(s->out), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

			if (s->io_flags == IO_OUT_APPEND)
				lseek(fdout, 0, SEEK_END);

			dup2(fdout, 1);
		}

		if (get_word(s->err) != NULL) {
			if (s->err == s->out) {
				dup2(fdout, 2);
			} else {
				if (s->io_flags != IO_ERR_APPEND)
					unlink(get_word(s->err));

				fderr = open(get_word(s->err), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
				if (s->io_flags == IO_ERR_APPEND)
					lseek(fderr, 0, SEEK_END);
				dup2(fderr, 2);
			}
		}

		int size = 0;
		char **argument_list = get_argv(s, &size);

		execvp(command, argument_list);

		if (get_word(s->in) != NULL)
			close(fdin);

		if (get_word(s->out) != NULL)
			close(fdout);

		if (get_word(s->err) != NULL && s->err != s->out)
			close(fderr);

		fprintf(stderr, "%s '%s'\n", "Execution failed for", get_word(s->verb));
		_exit(-1);
	} else {
		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			int exitStatus = WEXITSTATUS(status);

			free(command);
			return exitStatus;
		}

		fprintf(stderr, "%s '%s'\n", "Execution failed for", get_word(s->verb));
		return 1;
	}


	return 0; /* TODO: Replace with actual exit status. */
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
							command_t *father)
{
	/* TODO: Execute cmd1 and cmd2 simultaneously. */
	int status1 = 0;
	int status2 = 0;
	int ret;

	pid_t pid = fork();

	if (pid == -1)
		_exit(-1);

	if (pid == 0) {
		ret = parse_command(cmd1, level + 1, NULL);
		_exit(ret);
	} else {
		pid_t pid2 = fork();

		if (pid2 == -1)
			_exit(-1);
		if (pid2 == 0) {
			ret = parse_command(cmd2, level + 1, NULL);
			_exit(ret);
		} else {
			waitpid(pid, &status1, 0);
			waitpid(pid2, &status2, 0);

			if (WIFEXITED(status1) && WIFEXITED(status2))
				return WEXITSTATUS(status1) | WEXITSTATUS(status2);
		}
	}
	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
						command_t *father)
{
	/* TODO: Redirect the output of cmd1 to the input of cmd2. */
	int fd_pipe[2];
	int status1 = 0;
	int status2 = 0;
	int ret;

	if (pipe(fd_pipe) == -1)
		_exit(-1);

	pid_t pid = fork();

	if (pid == -1)
		_exit(-1);

	if (pid == 0) {
		close(fd_pipe[0]);
		dup2(fd_pipe[1], 1);
		ret = parse_command(cmd1, level + 1, NULL);
		close(fd_pipe[1]);
		_exit(ret);
	} else {
		pid_t pid2 = fork();

		if (pid2 == -1) {
			close(fd_pipe[1]);
			close(fd_pipe[0]);
			_exit(-1);
		}

		if (pid2 == 0) {
			close(fd_pipe[1]);
			dup2(fd_pipe[0], 0);

			ret = parse_command(cmd2, level + 1, NULL);
			close(fd_pipe[0]);
			_exit(ret);
		} else {
			close(fd_pipe[1]);
			close(fd_pipe[0]);

			waitpid(pid, &status1, 0);
			waitpid(pid2, &status2, 0);

			if (WIFEXITED(status1) && WIFEXITED(status2))
				return WEXITSTATUS(status2);
		}
	}

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO: sanity checks */

	int succes = -1;

	if (c->op == OP_NONE) {
		/* TODO: Execute a simple command. */

		return parse_simple(c->scmd, 0, NULL);
		/* TODO: Replace with actual exit code of command. */
	}

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* TODO: Execute the commands one after the other. */

		parse_command(c->cmd1, level + 1, NULL);
		parse_command(c->cmd2, level + 1, NULL);
		break;

	case OP_PARALLEL:
		/* TODO: Execute the commands simultaneously. */
		return run_in_parallel(c->cmd1, c->cmd2, level, NULL);

	case OP_CONDITIONAL_NZERO:
		/* TODO: Execute the second command only if the first one
		 * returns non zero.
		 */

		succes = parse_command(c->cmd1, level + 1, NULL);

		if (succes != 0)
			return parse_command(c->cmd2, level + 1, NULL);

		break;

	case OP_CONDITIONAL_ZERO:
		/* TODO: Execute the second command only if the first one
		 * returns zero.
		 */

		succes = parse_command(c->cmd1, level + 1, NULL);

		if (succes == 0)
			return parse_command(c->cmd2, level + 1, NULL);

		break;

	case OP_PIPE:
		/* TODO: Redirect the output of the first command to the
		 * input of the second.
		 */

		return run_on_pipe(c->cmd1, c->cmd2, level, NULL);

		break;

	default:
		return SHELL_EXIT;
	}

	return succes; /* TODO: Replace with actual exit code of command. */
}
