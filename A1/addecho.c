#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>

void addecho(char *source_name, char *dest_name, int delay, int volume_scale){
    

/**
     * first obtain header information and change it for the destwav
     *  then update the destwav
     * create echo buffer, consider delay
     * apply echo to file and save to buffer
     * if delay has been met/passed then start writing to new file 
     * free buffer and close files
     * 
    */

    char header[44];
   
    // Open the source file and terminate if unable to
    FILE *sourcewav = fopen(source_name, "rb");
    if (sourcewav == NULL) {
        fprintf(stderr, "Failed to open %s\n", source_name);
        exit(-1);
    }

    // Open the destination file and terminate if unable to
    FILE *destwav = fopen(dest_name, "wb");
    if (destwav == NULL) {
        fprintf(stderr, "Failed to open %s\n", dest_name);
        exit(-1);
    }

    //copy the header
    int bytes_read = fread(header, sizeof(unsigned char), 44, sourcewav);
    if(bytes_read != 44){
        fprintf(stderr, "fread fail at %s reading header\n", source_name);
        exit(-1);
    }
    int bytes_read2 = fwrite(header, sizeof(unsigned char), 44, destwav);
    if(bytes_read2 != 44){
        fprintf(stderr, "fwrite fail at %s cpoying header\n", dest_name);
        exit(-1);
    }

    int temp;

    int *echo_buffer = (int *)malloc(delay * sizeof(int));
    if(echo_buffer == NULL){
        printf("echo buffer malloc failed, delay is %d\n",delay);
        exit(-1);
    }
    memset(echo_buffer, 0, delay * sizeof(int));

    int bytes = header[34] / 8; //also equals 2

    //check if the file is atleast 44bytes
    int a =fseek(sourcewav, 0, SEEK_END);
    if(a!=0){
        fprintf(stderr, "fseek fail at %s end\n", source_name);
        exit(-1);
    }
    int size = ftell(sourcewav);
    if(size < 44){
        fprintf(stderr,"empty file\n");
        exit(-1);
    }


    a = fseek(sourcewav, 0, SEEK_END);
    if(a!=0){
        fprintf(stderr, "fseek fail at %s end\n", source_name);
        exit(-1);
    }
    int b = fseek(sourcewav, 44, SEEK_SET);
    if(b!=0){
        fprintf(stderr, "fseek fail at %s seek set\n", source_name);
        exit(-1);
    }
    int c = fseek(destwav, 44, SEEK_SET);
    if(c!=0){
        fprintf(stderr, "fseek fail at %s seek set\n", dest_name);
        exit(-1);
    }


    int read = 0;

    int echoed = 0;

    //while not done reading, apply echo
    while(fread(&temp, 2, 1, sourcewav) == 1) {
        temp = (short)temp;
        // Apply echo effect
        int echo = (int)temp / volume_scale;
        int echo_index = read % delay; //wrapping 
       
       //if we have read more than delay
        if (read >= delay) {

            int num = temp + echo_buffer[echoed % delay]; // we get the original and added old echo
            if(num > 32767){
                num = 32767;
            }
            if(num < -32768){
                num = -32768;
            }
            //echo_buffer[read % delay] = echo;
            bytes_read=fwrite(&num, 2, 1, destwav); //write to dest
                if(bytes_read!=1){
                fprintf(stderr, "fwrite fail at %s writing %d\n", dest_name, num);
                exit(-1);
                }
            echoed++; // increment the amount added

            // we also need to add the new echo in echo buffer
            echo_buffer[echo_index] = echo;
        }

        //if haven't reached delay
        if(read<delay){
            echo_buffer[echo_index] = echo; //add to echo buffer
            bytes_read=fwrite(&temp, 2, 1, destwav); //write original to 
                if(bytes_read!=1){
                fprintf(stderr, "fwrite fail at %s writing %d\n", dest_name, temp);
                exit(-1);
                }
        }

        // Increment total samples written
        read++;
    }

    
    
    int x = delay - read;
    int zero = 0;
    
    if (x > 0){
        for(int j = 0;j<x;j++){
            bytes_read=fwrite(&zero, bytes, 1, destwav);
                if(bytes_read!=1){
                fprintf(stderr, "fwrite fail at %s adding zeros\n", dest_name);
                exit(-1);
                }
        }
    }
    
    for(int j = 0;j<delay;j++){ //increase file size for echo
        bytes_read=fwrite(&zero, bytes, 1, destwav);
        if(bytes_read!=1){
            fprintf(stderr, "fwrite fail at %s adding zeros\n", dest_name);
            exit(-1);
        }
    }

    a = fseek(destwav, -delay*2, SEEK_CUR);
    if(a!=0){
    fprintf(stderr, "fseek fail at %s seek cur\n", dest_name);
    exit(-1);
    }

        //remaining buffer
    for (int i = echoed; i < read; i++) {
        int num = echo_buffer[i % delay];
        bytes_read=fwrite(&num, bytes, 1, destwav);
            if(bytes_read!=1){
            fprintf(stderr, "fwrite fail at %s writing %d\n", dest_name,num);
            exit(-1);
            }
    }

    //update header
    int data_chunk_size = (ftell(destwav) - 44); 

    // Update the data chunk size in the header
    a=fseek(destwav, 40, SEEK_SET);
    if(a!=0){
    fprintf(stderr, "fseek fail at %s seek set\n", dest_name);
    exit(-1);
    }
    bytes_read=fwrite(&data_chunk_size, sizeof(int), 1, destwav);
    if(bytes_read!=1){
    fprintf(stderr, "fwrite fail at %s writing %d\n", dest_name, data_chunk_size);
    exit(-1);
    }

    // Update the file size in the header
    int file_size = data_chunk_size + 36; // 36 bytes for the headers
    a = fseek(destwav, 4, SEEK_SET);
    if(a!=0){
    fprintf(stderr, "fseek fail at %s seek set\n", dest_name);
    exit(-1);
    }
    bytes_read=fwrite(&file_size, sizeof(int), 1, destwav);
    if(bytes_read!=1){
    fprintf(stderr, "fwrite fail at %s writing %d\n", dest_name, file_size);
    exit(-1);
    }

    // free memory
    free(echo_buffer);
    fclose(sourcewav);
    fclose(destwav);
}


int main(int argc, char **argv) {
    int delay = 8000;
    int volume_scale = 4;
    char *source_name;
    char *dest_name;

    /**
     * parse commandline using getopt()
     * since -d and -v and optional, this uses cases
     * returns -1 if successful
    */

    int opt;
     while((opt = getopt(argc, argv, "d:v:")) != -1){
        switch(opt){
            case 'd':
                delay = strtol(optarg, NULL, 10);
                break;
            case 'v':
                volume_scale = strtol(optarg, NULL, 10);
                break;
            default:
                printf("underfined command\n");
        }

     }

    //check number of args
    if (optind +2 != argc)
    {
        fprintf(stderr,"wrong format\n");
        exit(-1);
    }
    

    //todo implement error check
    source_name = argv[optind];
    dest_name = argv[optind+1];

    if (delay == 0 || delay < 0)
    {
    fprintf(stderr, "delay should not be 0 or negative\n");
    exit(-1);
    }
    if (volume_scale == 0 || volume_scale < 0)
    {
    fprintf(stderr, "volume scale should not be 0 or negative\n");
    exit(-1);
    }
    
    addecho(source_name, dest_name, delay, volume_scale);

    
    return 0;
}
