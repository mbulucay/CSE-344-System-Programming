#include "structs.h"


void print_date(date d){
    fprintf(stdout, "%d-%d-%d\n", d.day, d.month, d.year);
}


void print_request(server_request req){
    fprintf(stdout, "%s %s\n", req.real_estate, req.city_name);
    print_date(req.start_date);
    print_date(req.end_date);
    fprintf(stdout, "\n======================\n");
    fflush(stdout);
}


void get_date(char str[DATE_SIZE], date* d){

    char day[3], month[3], year[5];

    day[0] = str[0];
    day[1] = str[1];
    day[2] = '\0';

    month[0] = str[3];
    month[1] = str[4];
    month[2] = '\0';

    year[0] = str[6];
    year[1] = str[7];
    year[2] = str[8];
    year[3] = str[9];
    year[4] = '\0';

    d->day = atoi(day);
    d->month = atoi(month);
    d->year = atoi(year);

}


void print_transaction_arg(transaction_arg t){

    printf("#%d ", t.transaction_id);
    printf("%s ", t.real_estate_type);
    printf("%s ", t.city_name);
    printf("%ld ", t.price);
    printf("%d\n", t.surface_area);

}


void print_date_file(date_file df){

    printf("date: %d-%d-%d  > ", df.date.day, df.date.month, df.date.year);
    printf("transaction_count: %d\n", df.transaction_count);

    for(int i = 0; i < df.transaction_count; i++){
        print_transaction_arg(df.transaction[i]);
    }

    fprintf(stdout, "========================================================\n");

}


void print_city(City c){

    printf("city_name: %s  > ", c.city_name);
    printf("date_file_count: %d\n", c.date_file_count);

    for(int i = 0; i < c.date_file_count; i++){
        print_date_file(c.date_file[i]);
    }

}


void print_cities(City cities[], int num_cities){

    for(int i = 0; i < num_cities; i++){
        print_city(cities[i]);
    }

}


void swap_file(char file_1[FOLDER_NAME_LEN], char file_2[FOLDER_NAME_LEN]){
    
    char temp[FOLDER_NAME_LEN];
    strcpy(temp, file_1);
    strcpy(file_1, file_2);
    strcpy(file_2, temp);

}


void sort_folders(char directory[MAX_FOLDER][FOLDER_NAME_LEN], int num_folders){

    int i, j;
    for(i = 0; i < num_folders; i++){
        for(j = i + 1; j < num_folders; j++){
            if(strcmp(directory[i], directory[j]) > 0){
                swap_file(directory[i], directory[j]);
            }
        }
    }

}


void print_folders(folder resp_dir[], int num_folders){

    int i;
    for(i = 0; i < num_folders; i++){
        printf("[%d] > %s\n",i , resp_dir[i].name);
    }

}

