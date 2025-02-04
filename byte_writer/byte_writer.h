#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint16_t uWord;
typedef int16_t  Word;
typedef uWord    Instruction;

typedef struct {
    size_t       capacity; 
    size_t       count; 
    Instruction* bytes;
} Byte_Data;

Byte_Data* init_byte_data() {
    Byte_Data* byte_data = (Byte_Data*)malloc(sizeof(Byte_Data));
    byte_data->count = 0;
    byte_data->capacity = 10;
    byte_data->bytes = (Instruction*)malloc(sizeof(uWord)*byte_data->capacity);
    return byte_data;
}

void push_data(Byte_Data* byte_data, uWord data) {
    if (byte_data->count += 1 > byte_data->capacity) {
        byte_data = (Byte_Data*)realloc(byte_data, byte_data->capacity + 5); 
    } 
    byte_data->bytes[byte_data->count] = data;
    byte_data->count++;
}

Byte_Data* read_bin_from_file(char* file_name) {
    FILE* fh;
    fh = fopen(file_name, "rb");
    if (!fh) { printf("ERROR: could not open specified file `%s`", file_name); return NULL; }

    fseek(fh, 0, SEEK_END);
    size_t length = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    Byte_Data* byte_data = init_byte_data();

    uWord word = 0;
    for (int i = 0; i < length/2; i++) {
        int ok = fread(&word, 2, 1, fh);
        if (!ok) { printf("ERROR: error reading binary data for file `%s`", file_name); return NULL; }
        push_data(byte_data, word);
    }

    return byte_data;
}

bool write_bin_to_file(const Byte_Data* byte_data, char* file_name) {
    FILE* file;
    file = fopen(file_name, "wb");
    if (file == NULL) return false;
    for (int i = 0; i < byte_data->count; i++) {
        fwrite(&byte_data->bytes[i], sizeof(uWord), 1, file);
    }
    fclose(file);
    return true;
}

