#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define FIRST 1
#define ONE 0
#define MIDDLE 2
#define LAST 3

int	ft_strcmp(char *str, char *str1)
{
	if (!str || !str1)
		return 1;
	else
		return (strcmp(str, str1));
}

int	counter(char **av, int i)
{
	int res = 1;
	while (av[i] && ft_strcmp(av[i], ";"))
	{
		i++;
		if (!ft_strcmp(av[i], "|"))
			res++;
	}
	return (res);
}

char **get_cmd(char **av, int i)
{
	int index;
	char **res;
	int size;
	index = 0;

	size = 0;
	if (!ft_strcmp(av[i], "|") || !ft_strcmp(av[i], ";")|| !av[i])
		i--;
	while (ft_strcmp(av[i], "|") && ft_strcmp(av[i], ";") && i >= 0)
	{
		size++;
		i--;
	}
	if (!ft_strcmp(av[i], "|") || !ft_strcmp(av[i], ";") || i < 0)
		i++;
	res = malloc(sizeof(char *) * (size + 1));
	res[size] = NULL;
	while (ft_strcmp(av[i], "|") && ft_strcmp(av[i], ";") && av[i])
	{
		res[index] = av[i];
		i++;
		index++;
	}
	return (res);
}

void	put_error(char *str)
{
	int i;

	if (!str)
		return ;
	i = 0;
	while (str[i])
		write(2, &str[i++], 1);
}

int get_pos(int number, int count)
{
	if (number == 1 && count == 1)
		return ONE;
	if (number == 1)
		return FIRST;
	if (number > 1 && number == count)
		return LAST;
	return MIDDLE;
}

void	set_fds(int pos, int prev_fd, int *fds)
{
	if (pos == ONE)
		return ;
	if (pos == FIRST)
	{
		if (dup2(fds[1], 1) == -1 || close(fds[1]) == -1 || close(fds[0]) == -1)
			put_error("error: fatal\n"), exit(1);
		return ;
	}
	if (pos == MIDDLE)
	{
		if (dup2(fds[1], 1) == -1 || dup2(prev_fd, 0) || close(prev_fd) == -1
			|| close(fds[1]) == -1 || close(fds[0]) == -1)
				put_error("error: fatal\n"), exit(1);

	}
	if (pos == LAST)
	{
		if (dup2(prev_fd, 0) == -1 || close(prev_fd) == -1)
			put_error("error: fatal\n"), exit(1);
		return ;
	}
}

int final_part(int prev_fd, int pos, int *fds)
{
	if (pos == FIRST)
	{
		if (close(fds[1]) == -1)
			exit(1);
		return (fds[0]);
	}
	if (pos == MIDDLE)
	{
		if (close(prev_fd) == -1 || close(fds[1]) == -1)
			put_error("error: fatal\n"), exit(1);
		return (fds[0]);
	}
	if (pos == LAST)
	{
		if (close(prev_fd) == -1)
			put_error("error: fatal\n"), exit(1);
		return (-1);
	}
	return (-1);
}

void	cd_in_child(char **av)
{
	int id;
	id = fork();
	if (id == -1)
		put_error("error: fatal\n");
	if (id == 0)
	{
		if (av[1] && av[2])
			return ((put_error("error: cd: bad arguments\n")));
		if (!av[1])
			return (put_error("error: cd: bad arguments\n"));
		if (chdir(av[1]) == -1)
		{
			put_error("error: cd: cannot change directory to ");
			put_error(av[1]);
			write(2, "\n", 1);
		}
		exit(0);
	}
	waitpid(id, NULL, 0);
	return ;
}

void	ft_cd(char **av, int pos)
{
	if (pos != ONE)
		return (cd_in_child(av));
	if (av[1] && av[2])
		return (put_error("error: cd: bad arguments\n"));
	if (!av[1])
		return (put_error("error: cd: bad arguments\n"));
	if (chdir(av[1]) == -1)
	{
		put_error("error: cd: cannot change directory to ");
		put_error(av[1]);
		write(2, "\n", 1);
	}
}

int	executer(char **av, int i, int number, int count, char **env, int prev_fd)
{
	char **cmd_av;
	int id;
	int fds[2];
	int pos;

	pos = get_pos(number, count);
	cmd_av = get_cmd(av, i);
	if (pos == MIDDLE || pos == FIRST)
		pipe(fds);
	if (!ft_strcmp(cmd_av[0], "cd"))
	{
		ft_cd(cmd_av, pos);
		prev_fd = final_part(prev_fd, pos, fds);
		free(cmd_av);
		return (prev_fd);
	}
	id = fork();
	if (id == -1)
	{
		put_error("error: fatal\n");
		exit(1);
	}
	if (id == 0)
	{
		set_fds(pos, prev_fd, fds);
		execve(cmd_av[0], cmd_av, env);
		put_error("error: cannot execute ");
		put_error(cmd_av[0]);
		write(2, "\n", 1);
		free(cmd_av);
		exit(1);
	}
	waitpid(id, NULL, 0);
	prev_fd = final_part(prev_fd, pos, fds);
	free(cmd_av);
	return(prev_fd);
}

void	ft_pipe_line(char **av, int i, char **env)
{
	int index;
	int prev;
	int count;

	index = 1;
	prev = -1;
	count = counter(av, i);
	while (av[i] && ft_strcmp(av[i], ";"))
	{
		if (!ft_strcmp(av[i], "|"))
		{
			prev = executer(av, i, index, count, env, prev);
			index++;
		}
		i++;
	}
	executer(av, i, index, count, env, prev);
}

int is_empty(char **av, int i)
{
	if (i == 0)
		return (1);
	if (!ft_strcmp(av[i - 1], ";"))
		return (1);
	return (0);
}

void ft_devider(char **av, int i, char **env)
{
	if (is_empty(av, i))
		return ;
	if (!ft_strcmp(av[i], ";") || !av[i])
		i--;
	while (ft_strcmp(av[i], ";") && i >= 0)
		i--;
	i++;
	ft_pipe_line(av, i, env);
	return ;
}

int main(int ac, char **av, char **envp)
{
	int i;

	i = 0;
	av++;
	if (ac == 1)
		return (1);
	while (av[i])
	{
		if (!ft_strcmp(av[i], ";"))
			ft_devider(av, i, envp);
		i++;
	}
	ft_devider(av, i, envp);
	return (0);
}
