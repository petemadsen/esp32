/**
 * This code is public domain.
 */
#ifndef PROJECT_WAV_H
#define PROJECT_WAV_H

#include <stdio.h>


/**
 * @brief Read file into a byte array. Byte array is allocated with malloc().
 * @param file
 * @return
 */
unsigned char* read_wav(FILE* file, size_t* len);


#endif
