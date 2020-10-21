#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<dirent.h>
#include<netinet/in.h>
#include<fcntl.h>


char** parse(char* cmd_line);
void* runner(int* param);

int main(int argc, char** argv)
{
	int server_socket, client_socket;
	struct sockaddr_in server_socket_ds, client_socket_ds;
  

	if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error in socket creation...\n");
		exit(1);
	}

	server_socket_ds.sin_port = atoi(argv[1]);
	server_socket_ds.sin_addr.s_addr = 0;

	if((bind(server_socket, (struct sockaddr *)&server_socket_ds, sizeof(server_socket_ds))) == -1)
    {
    	printf("Error in binding...\n");
		exit(1);
    }

    if((listen(server_socket, 1)) == -1)
    {
    	printf("Error in listening...\n");
		exit(1);
    }
    int i = 0;

        unsigned int len = sizeof(client_socket_ds);
    l:  client_socket = accept(server_socket, (struct sockaddr *)&client_socket_ds, &len);
        
        printf("%d connected \n", client_socket);
        int p_arg = client_socket;
        int pid = fork();
        if(pid == 0)
        {
            runner(&p_arg);
        }
        else if(pid< 0)
        {
            printf("Fork error");
        }
        else
        {
            goto l;
        }

	return 0;
}

void* runner(int* param)
{
    int client_socket = *param;
    char* cmd_line;
    int size_of_cmd_line;
    char** args;
    struct stat obj;
    char key;
    char hom[100];
    getcwd(hom, 100);
   
    while(1)
    {
        recv(client_socket, &size_of_cmd_line, sizeof(int), 0);
        cmd_line = (char *)malloc(sizeof(char)*size_of_cmd_line);
        char* cp_cmd_line = (char *)malloc(sizeof(char)*100);
        recv(client_socket, cmd_line, size_of_cmd_line, 0);
        strcpy(cp_cmd_line, cmd_line);
        args = parse(cmd_line);
        off_t offset = 0;
        unsigned int size = 0;
        

        if((strcmp(args[0], "close")) == 0)
        {
        	int status = 1;
        	send(client_socket, &status, sizeof(int), 0);
        	printf("Connection to %d closed\n", client_socket);
            exit(0);
        }

        else if((strcmp(args[0], "get")) == 0)
        {
        	int fd;
        	int temp = 1;
        	
        	if((fd = open(args[1], O_RDONLY))==-1)
        	{
        		temp = -1;
        		send(client_socket, &temp, sizeof(int), 0);
        	}
        	else
        	{
                stat(args[1], &obj);
                size = obj.st_size;
                send(client_socket, &temp, sizeof(int), 0);
                send(client_socket, &size, sizeof(unsigned int), 0);
                sendfile(fd, client_socket, offset, &size, NULL, 0);
            }
        }

        else if((strcmp(args[0], "put")) == 0)
        {
        	char* buf;
        	int s;
        	unsigned int file_size = 0;
        	recv(client_socket, &file_size, sizeof(unsigned int), 0);
            //printf("**file size to recv %d **\n", file_size);
            buf = (char *)malloc(file_size);
            recv(client_socket, buf, file_size, 0);
            int fd = open(args[1], O_CREAT | O_RDONLY | O_EXCL | O_WRONLY, 0666);
            s = write(fd, buf, file_size);
            close(fd);
            send(client_socket, &s, sizeof(int), 0);

        }

        else if((strcmp(args[0], "cd")) == 0)
        {
        	if(args[1] == NULL)
            {
                chdir(hom);
            }
            else
            {   
                int temp = chdir(args[1]);
                if(temp != 0)
                {
                    printf("Not a valid path to change directory\n");
                }
                
            }         
        }

        else if((strcmp(args[0], "chmod")) == 0)
        {
        	int i = strtol(args[1], 0, 8);
        	int status = 1;
            if((status = chmod(args[2], i)) < 0)
                printf("Error in chmod\n");
            send(client_socket, &status, sizeof(int), 0);
        }

        else
        { 
            int pid_n = fork();
            if(pid_n == 0)
            {
        	    int mystdout=open("temp.txt", O_CREAT | O_WRONLY | O_RDONLY, 0777);
                close(1); 
                dup2(mystdout, 1);
                execvp(args[0], args);  
                exit(0);  
            }
            else if(pid_n<0)
                printf("Error in fork\n");
            else
            { 
                wait(NULL);   
                int fd = open("temp.txt", O_RDONLY);
                if(fd == -1)
                {
        	        printf("File doesn't exist\n");
                }
                else
                {
                    stat("temp.txt", &obj);
                    size = obj.st_size;
                    send(client_socket, &size, sizeof(unsigned int), 0);
                sendfile(fd, client_socket, offset, &size, NULL, 0);
                unlink("temp.txt");
            }
        }
        }

        free(cmd_line);
        free(cp_cmd_line);
        free(args);
    }
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


