#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 1024

/**
 * _close - closes a file descriptor and handles errors
 * @fd: file descriptor
 *
 * Return: 0 on success, exits with code 100 on error
 */
int _close(int fd)
{
	if (close(fd) == -1)
	{
		dprintf(STDERR_FILENO, "Error: Can't close fd %d\n", fd);
		exit(100);
	}
	return (0);
}

/**
 * open_files - opens source and destination file descriptors
 * @file_from: path to source file
 * @file_to: path to destination file
 * @fd_from: pointer to store source fd
 * @fd_to: pointer to store destination fd
 * @buffer: buffer to free on error
 */
void open_files(char *file_from, char *file_to,
	int *fd_from, int *fd_to, char *buffer)
{
	*fd_from = open(file_from, O_RDONLY);
	if (*fd_from == -1)
	{
		dprintf(STDERR_FILENO, "Error: Can't read from file %s\n", file_from);
		free(buffer);
		exit(98);
	}

	*fd_to = open(file_to, O_CREAT | O_WRONLY | O_TRUNC, 0664);
	if (*fd_to == -1)
	{
		dprintf(STDERR_FILENO, "Error: Can't write to %s\n", file_to);
		free(buffer);
		_close(*fd_from);
		exit(99);
	}
}

/**
 * write_chunk - writes a full chunk to fd_to, handles partial writes
 * @fd_to: destination file descriptor
 * @buffer: data buffer
 * @to_write: number of bytes to write
 * @file_to: destination filename for error message
 * @fd_from: source fd to close on error
 */
void write_chunk(int fd_to, char *buffer, ssize_t to_write,
	char *file_to, int fd_from)
{
	ssize_t written = 0, write_ret;

	while (to_write > 0)
	{
		write_ret = write(fd_to, buffer + written, to_write);
		if (write_ret == -1)
		{
			dprintf(STDERR_FILENO, "Error: Can't write to %s\n", file_to);
			free(buffer);
			_close(fd_from);
			_close(fd_to);
			exit(99);
		}
		written += write_ret;
		to_write -= write_ret;
	}
}

/**
 * copy_content - reads from fd_from and writes to fd_to
 * @fd_from: source file descriptor
 * @fd_to: destination file descriptor
 * @buffer: read buffer
 * @file_to: destination filename for error messages
 * @file_from: source filename for error messages
 */
void copy_content(int fd_from, int fd_to, char *buffer,
	char *file_to, char *file_from)
{
	ssize_t r;

	while ((r = read(fd_from, buffer, BUF_SIZE)) > 0)
		write_chunk(fd_to, buffer, r, file_to, fd_from);

	if (r == -1)
	{
		dprintf(STDERR_FILENO, "Error: Can't read from file %s\n", file_from);
		free(buffer);
		_close(fd_from);
		_close(fd_to);
		exit(98);
	}
}

/**
 * main - copies content of one file to another
 * @ac: argument count
 * @av: argument vector
 *
 * Return: 0 on success, exits with error codes otherwise
 */
int main(int ac, char **av)
{
	int fd_from, fd_to;
	char *buffer;

	if (ac != 3)
	{
		dprintf(STDERR_FILENO, "Usage: cp file_from file_to\n");
		exit(97);
	}

	buffer = malloc(BUF_SIZE);
	if (buffer == NULL)
	{
		dprintf(STDERR_FILENO, "Error: Can't write to %s\n", av[2]);
		exit(99);
	}

	open_files(av[1], av[2], &fd_from, &fd_to, buffer);
	copy_content(fd_from, fd_to, buffer, av[2], av[1]);

	free(buffer);
	_close(fd_from);
	_close(fd_to);

	return (0);
}
