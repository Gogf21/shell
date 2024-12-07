#include<stdio.h>
#include <string.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<unistd.h>
#include<sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>


void save_history(char *command)
{
    FILE *file = fopen("history.txt", "a");
    if (file == NULL)
    {
        perror("Failed to open");
        return;
    }
    fprintf(file, "%s\n", command);
    fclose(file);
}
void handle_sighup(int sig){
    printf("Configuration reloaded\n");
    printf("my_shell> ");

}

void is_bootable_device(char* device_name) {
	while(*device_name  == ' ') device_name++;

	const char* root = "/dev/";
	char full_path[128];
	sprintf(full_path, "%s%s", root, device_name);

	FILE* device_file = fopen(full_path, "rb");

	if(device_file == NULL) {
		printf("There is no such disk!\n");
		return;
	}

	int position = 510;
	if(fseek(device_file, position, SEEK_SET) != 0) {
		printf("Error while SEEK operation!\n");
		fclose(device_file);
		return;
	}

	uint8_t data[2];
	if(fread(data, 1, 2, device_file) != 2) {
		printf("Error while fread operation\n");
		fclose(device_file);
		return;
	}
	fclose(device_file);

	if(data[1]==0xaa && data[0]==0x55) {
		printf("Disk %s is bootable\n", device_name);
	} else {
		printf("Disk %s isn't bootable\n", device_name);
	}
}

void dump_process_memory(const char *pid) {
    char dump_file_name[256];
    snprintf(dump_file_name, sizeof(dump_file_name), "memory_dump_%s.txt", pid);
    
    FILE *dump_file = fopen(dump_file_name, "w");
    if (!dump_file) {
        perror("Failed to open output file");
        return;
    }

    char maps_directory_path[256];
    snprintf(maps_directory_path, sizeof(maps_directory_path), "/proc/%s/map_files", pid);

    struct stat stat_buffer;
    if (stat(maps_directory_path, &stat_buffer) != 0 || !S_ISDIR(stat_buffer.st_mode)) {
        printf("Process %s does not exist or has no memory maps.\n", pid);
        fclose(dump_file);
        return;
    }

    DIR *maps_directory = opendir(maps_directory_path);
    if (!maps_directory) {
        perror("Failed to open map_files directory");
        fclose(dump_file);
        return;
    }

    struct dirent *directory_entry;
    while ((directory_entry = readdir(maps_directory)) != NULL) {
        if (strcmp(directory_entry->d_name, ".") == 0 || strcmp(directory_entry->d_name, "..") == 0) {
            continue;
        }

        char memory_file_path[512];
        if (snprintf(memory_file_path, sizeof(memory_file_path), "%s/%s", maps_directory_path, directory_entry->d_name) >= sizeof(memory_file_path)) {
            fprintf(stderr, "Warning: file path truncated: %s/%s\n", maps_directory_path, directory_entry->d_name);
            continue; 
        }

        FILE *memory_file = fopen(memory_file_path, "r");
        if (memory_file) {
            char line_buffer[1024];
            while (fgets(line_buffer, sizeof(line_buffer), memory_file) != NULL) {
                fputs(line_buffer, dump_file);
            }
            fclose(memory_file);
        } else {
            perror("Failed to open memory file");
        }
    }

    closedir(maps_directory);
    fclose(dump_file);
    printf("Memory dump for process %s saved to %s\n", pid, dump_file_name);
}




int main()
{
    char input[999];

    printf("my_shell> ");
    fgets(input,sizeof(input),stdin);

    input[strcspn(input,"\n")] = 0;

    printf("You entered: %s\n", input);
    
    while(1)
    {

        save_history(input);
        printf("my_shell> ");
        signal(SIGHUP, handle_sighup);//number9;

        if(fgets(input, sizeof(input),stdin) == NULL)
        {
            printf("Exit\n");
            break;
        }
        input[strcspn(input, "\n")] = 0;

        
        if(strcmp(input, "exit") == 0 || strcmp(input,"\\q") == 0)
        {
            printf("Exit\n");
            break;
        }

        if(strncmp(input,"echo ",5)==0)
        {
            printf("%s\n", input + 5);
            continue;   
        }
        if(strcmp(input,"\\e #PATH") == 0)
        {
            const char * path = getenv("PATH");
            printf("%s\n",path);
            continue;   
        }
        
        if(strncmp(input, "\\l ", 3)==0){
			is_bootable_device(input + 2);
            continue;
		}
        
        if(strncmp(input,"./",2) == 0){//NUMBER 8
            pid_t pid = fork();

            if (pid == 0) {
                char path[1024];
                snprintf(path, sizeof(path), "/bin/%s", input);
                execl(path, input, NULL);
                perror("execl");
                exit(1);
            } else if (pid > 0){
                int status;
                waitpid(pid,&status, 0);
            }
            else {
                perror("fork");
            }
            continue;
            
        }
        

        printf("You entered: %s\n",input);
        
       
    if (strncmp(input, "\\mem ", 5) == 0) {
            char* proc_id = input + 5;
            proc_id[strcspn(proc_id, "\n")] = 0;
            dump_process_memory(proc_id);
            continue;
    }

    printf("Комманда не найденна\n");
    }
    return 0;
}