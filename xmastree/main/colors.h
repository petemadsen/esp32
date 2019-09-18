#ifndef XMASTREE_COLORS_H
#define XMASTREE_COLORS_H


void colors_init();


#define COLORS_BLACK		0
#define COLORS_DEMO			1
#define COLORS_RED			2
#define COLORS_GREEN		3
#define COLORS_BLUE			4
#define COLORS_WHITE		5
#define COLORS_SNAKE_BLUE	6
#define COLORS_SNAKE_WHITE	7
#define COLORS_FILL_BLUE	8
#define COLORS_FILL_BLUE_R	9
#define COLORS_RAINBOW		10
#define COLORS_PULSING_RED	11
#define COLORS_PULSING_RGB	12
#define COLORS_WHITE_NOISE	13
#define COLORS_XXX			14
#define COLORS_MSGEQ7		15
#define COLORS_NUM_MODES	16


void colors_set_mode(int mode);


void colors_next_mode();


const char* colors_mode_desc(int mode);


#endif
