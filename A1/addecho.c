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
    fread(header, sizeof(unsigned char), 44, sourcewav);
    fwrite(header, sizeof(unsigned char), 44, destwav);

    int temp;

    int *echo_buffer = (int *)malloc(delay * sizeof(int));
    if(echo_buffer == NULL){
        printf("malloc failed\n");
        exit(-1);
    }
    memset(echo_buffer, 0, delay * sizeof(int));

    int bytes = header[34] / 8; //also equals 2
    fseek(sourcewav, 0, SEEK_END);


    fseek(sourcewav, 0, SEEK_END);
    fseek(sourcewav, 44, SEEK_SET);
    fseek(destwav, 44, SEEK_SET);

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
            fwrite(&num, 2, 1, destwav); //write to dest
            echoed++; // increment the amount added

            // we also need to add the new echo in echo buffer
            echo_buffer[echo_index] = echo;
        }

        //if haven't reached delay
        if(read<delay){
            echo_buffer[echo_index] = echo; //add to echo buffer
            fwrite(&temp, 2, 1, destwav); //write original to 
        }

        // Increment total samples written
        read++;
    }

    
    
    int x = delay - read;
    int zero = 0;
    
    if (x > 0){
        for(int j = 0;j<x;j++){
            fwrite(&zero, bytes, 1, destwav);
        }
    }
    
    for(int j = 0;j<delay;j++){ //increase file size for echo
        fwrite(&zero, bytes, 1, destwav);
    }

    fseek(destwav, -delay*2, SEEK_CUR);

        //remaining buffer
    for (int i = echoed; i < read; i++) {
        int num = echo_buffer[i % delay];
        fwrite(&num, bytes, 1, destwav);
    }

    //update header
    int data_chunk_size = (ftell(destwav) - 44); 

    // Update the data chunk size in the header
    fseek(destwav, 40, SEEK_SET);
    fwrite(&data_chunk_size, sizeof(int), 1, destwav);

    // Update the file size in the header
    int file_size = data_chunk_size + 36; // 36 bytes for the headers
    fseek(destwav, 4, SEEK_SET);
    fwrite(&file_size, sizeof(int), 1, destwav);

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
                printf("underfined command");
        }

     }


    //todo implement error check
    source_name = argv[optind];
    dest_name = argv[optind+1];

    if (delay == 0)
    {
    fprintf(stderr, "delay should not be 0\n");
    exit(-1);
    }
    if (volume_scale == 0)
    {
    fprintf(stderr, "volume scale should not be 0\n");
    exit(-1);
    }
    
    addecho(source_name, dest_name, delay, volume_scale);

    
    return 0;
}

