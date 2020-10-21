#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<dirent.h>
#include<netinet/in.h>
#include<fcntl.h>

char* read_cmd();
char** parse(char* cmd_line);
int execute(char** cmd_table, char* cmd_line);

int cmd_lls(char** argv);
int cmd_lcd(char** argv);
int cmd_help(char** argv);
int cmd_lchmod(char** argv);

int cmd_ls(char* arg, char** args);
int cmd_get(char* arg, char** args);
int cmd_put(char* arg, char** args);
int cmd_cd(char* arg, char** args);
int cmd_chmod(char* arg, char** args);
int cmd_close(char* argv, char** args);


struct sockaddr_in server_socket_ds;
struct stat obj;
int client_socket;
char hom[100];


int main(int argc, char** argv)
{
	  char* cmd_line;
	  char* cp_cmd_line;
	  char** cmd_table;
	  int status = 1, count = 0;
	  getcwd(hom, 100);

    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
    	  printf("Error in socket creation...\n");
    	  exit(1);
    }
  
    server_socket_ds.sin_family = AF_INET;
    server_socket_ds.sin_port = atoi(argv[1]);
    server_socket_ds.sin_addr.s_addr = 0;

    if((connect(client_socket, (struct sockaddr *)&server_socket_ds, sizeof(server_socket_ds))) == -1)
    {
    	  printf("Error in connection...\n");
    	  exit(1);
    }


	do
	{
	    cmd_line = read_cmd();
	    if(strcmp(cmd_line, "\n")==0)
	        continue;  
	       
	    count = strlen(cmd_line);
	      
	    cp_cmd_line = (char *)malloc(sizeof(char)*count);
	    strcpy(cp_cmd_line, cmd_line);
	      	     
      cmd_table = parse(cmd_line);
         
      status = execute(cmd_table, cp_cmd_line);
     
  }while(status);

	return 0;
}

char* read_cmd()
{
	  char* rd_line = NULL;
	  unsigned long bufsize = 0;
	  getline(&rd_line, &bufsize, stdin);
	  return rd_line;
}

char** parse(char* cmd_line)
{
	  char** cmd_table_tokens;
	  char* token;
	  int i = 0;
	  int max_tokens = 10;
    cmd_table_tokens = (char **)malloc(sizeof(char *)*max_tokens);

    token = strtok(cmd_line, " \t\r\n\a");

    while(token != NULL)
    {
    	  cmd_table_tokens[i] = token;
    	  i++;
    	  if(i >= max_tokens-1)
    	  {
    	    	printf("\nNo. of tokens has exceeded the maximum limit\n");
            
            char **old_a = cmd_table_tokens;
            cmd_table_tokens = (char** )malloc(sizeof(char *)*(2*max_tokens));
            for(int i = 0 ; i<max_tokens ; i++)
	              cmd_table_tokens[i] = old_a[i]; 
            free(old_a);	
            max_tokens = max_tokens*2;
            printf("max_tokens doubled\n");
    	  }
    	  token = strtok(NULL, " \t\r\n\a");
    }
    cmd_table_tokens[i] = NULL;
	  return cmd_table_tokens;
}

char* array_of_local_cmds[] = { "lls", "help", "lcd", "lchmod"};
char* array_of_server_cmds[] = { "ls", "get", "put", "cd", "chmod", "close"};
int (*array_of_server_functions[])(char* arg, char** args) = { &cmd_ls, &cmd_get, &cmd_put, &cmd_cd, &cmd_chmod, &cmd_close};
int (*array_of_local_functions[])(char** args) = { &cmd_lls, &cmd_help, &cmd_lcd, &cmd_lchmod};
int no_local_cmds = 4;
int no_server_cmds = 6;

int cmd_lls(char** argv)
{
    int pid = fork();
    argv[0] = "ls";
    if(pid == 0)
    {
        if(execvp(argv[0], argv) == -1)
        {
            printf("Not a valid command\n");
        }
        exit(0);
    }
    else if(pid<0)
        printf("Error in fork\n");
    else
    {
        wait(NULL);
    }
    return 1;
}

int cmd_help(char** argv)
{
	  printf("Following are the local commands\n");
    for(int i = 0 ; i<no_local_cmds ; i++)
    	  printf("%d. %s\n", i+1, array_of_local_cmds[i]);

    printf("Following are the server commands\n");
    for(int i = 0 ; i<no_server_cmds ; i++)
        printf("%d. %s\n", i+1, array_of_server_cmds[i]);  
    return 1;
}

int cmd_lcd(char** argv)
{
    if(argv[1] == NULL)
    {
        chdir(hom);
    }
    else
    {   
        int temp = chdir(argv[1]);
        if(temp != 0)
        {
            printf("Not a valid path to change directory\n");
        }
    }          
	return 1;
}

int cmd_lchmod(char** argv)
{
    int i = strtol(argv[1], 0, 8);
    if(chmod(argv[2], i) < 0)
        printf("Error in chmod\n");

	  return 1;
}

int cmd_ls(char* arg, char** args)
{
    int cmd_line_len = strlen(arg);
    unsigned int rf_size = 0;
    char *buf;
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);

    recv(client_socket, &rf_size, sizeof(unsigned int), 0);
    //printf("**cli %d **\n", rf_size);
    buf = (char *)malloc(rf_size);
    recv(client_socket, buf, rf_size, 0);
    printf("%s",buf);
    free(buf);
	  return 1;
}

int cmd_cd(char* arg, char** args)
{
    int cmd_line_len = strlen(arg);
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);
    return 1;
}

int cmd_chmod(char* arg, char** args)
{
    int cmd_line_len = strlen(arg);
    int status = 0;
 
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);
    recv(client_socket, &status, sizeof(int), 0);
    if(status > 0)
        printf("Permissions changed"); 
    return 1;
}

int cmd_get(char* arg, char** args)
{
    int cmd_line_len = strlen(arg);
    int s = 0, status = 0;
    unsigned int file_size = 0;
    char *buf;
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);

    recv(client_socket, &status, sizeof(int), 0);
    if(status == -1)
    {
        printf("File not found\n");
        return 1;
    }
    recv(client_socket, &file_size, sizeof(unsigned int), 0);
    //printf("**file size to recv %d **\n", file_size);
    buf = (char *)malloc(file_size);
    recv(client_socket, buf, file_size, 0);
    int fd = open(args[1], O_CREAT | O_RDONLY | O_EXCL | O_WRONLY, 0666);
    s = write(fd, buf, file_size);
    close(fd);
    return 1;
}

int cmd_put(char* arg, char** args)
{
    int fd;
    if((fd = open(args[1], O_RDONLY)) == -1)
    {
        printf("file doesn't exist\n");
        return 1;
    }
    int cmd_line_len = strlen(arg);
    int status = 0;
    unsigned int file_size = 0;
    off_t offset = 0;
    char *f;
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);

    stat(args[1], &obj);
    file_size = obj.st_size;
    //printf("size of file to send is %d\n", file_size);
    send(client_socket, &file_size, sizeof(unsigned int), 0);
    sendfile(fd, client_socket, offset, &file_size, NULL, 0);
    recv(client_socket, &status, sizeof(int), 0);
    if(status)
        printf("File uploaded successfully\n");
    else
        printf("File uploading failed\n");
    return 1;
}

int cmd_close(char* arg, char** args)
{
    int cmd_line_len = strlen(arg);
    int status = 0;
    send(client_socket, &cmd_line_len, sizeof(int), 0);
    send(client_socket, arg, cmd_line_len, 0);
    recv(client_socket, &status, sizeof(int), 0);
    if(status == 1)
    {
        printf("Connection closed successfully\n");
        return 0;
    }
    else
    {
        printf("Problem in closing connection\n");
        return 1; 
    }
}


int execute(char** cmd_table, char* cmd_line)
{
	  int i;
	  if(cmd_table == NULL)
		    return 1;

	  for(i = 0 ; i<no_local_cmds ; i++)
	  {
		    if(strcmp(cmd_table[0], array_of_local_cmds[i])==0)
		    {
			      int temp;
			      temp = (*array_of_local_functions[i])(cmd_table);
			      return temp;			
		    }
	  }

    for(i = 0 ; i<no_server_cmds ; i++)
    {
        if(strcmp(cmd_table[0], array_of_server_cmds[i])==0)
        {
            int temp;
            temp = (*array_of_server_functions[i])(cmd_line, cmd_table);
            return temp;      
        }
    }
}

