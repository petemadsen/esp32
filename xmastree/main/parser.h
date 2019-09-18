#ifndef AW_POWER_PARSER_H
#define AW_POWER_PARSER_H


void parser_init();

/**
 * Commands:
 * status
 * quit
 * exit
 * ota
 * reboot
 * read [0-9]
 * scan
 * ident
 */
char* parse_input(char* data, int data_len);


#endif
