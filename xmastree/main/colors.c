#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "ws2812b.h"
#include "colors.h"
#include "config.h"
#include "msgeq7.h"


static void run_mode(int m, struct leds_t* leds, bool single_run);
static const char* MY_TAG = "xmastree/colors";


#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))



typedef uint32_t COLOR;
#define MAKE_COLOR(r, g, b) (((r&0xff)<<16) | ((g&0xff)<<8) | (b&0xff))


#define RED		0x3f0000
#define GREEN	0x003f00
#define BLUE	0x00003f
#define BLACK	0x000000

#define RED_M	0x1f0000
#define GREEN_M	0x001f00
#define BLUE_M	0x00001f
#define WHITE_M	0x1f1f1f



static struct leds_t leds;


// XXX: assume that read/write to bool is atomic.
static bool m_do_stop = false;
#define CHECK_DO_STOP(x) if (m_do_stop) x;



static int m_mode = COLORS_BLACK;


static inline uint32_t rainbow(float height)
{
	float max_val = 64.0;
    height = max(0.f, min(1.f, height));

    if (height < 0.25f)
        return (0x00 << 16) | ((int)(4. * height * max_val) << 8) | (int)max_val;
    else if (height < 0.5)
        return (0x00 << 16) | ((int)max_val << 8) | ((int)((2.0f - 4 * height) * max_val));
    else if (height < 0.75)
        return ((int)((-2. + 4. * height) * max_val) << 16) | ((int)max_val << 8);
    else
        return ((int)max_val << 16) | (((int)((4. - 4. * height) * max_val)) << 8);
}


static COLOR random_middle()
{
	COLOR c = esp_random();
	uint16_t r = (c >> 16) & 0xff * 32 / 255;
	uint16_t g = (c >> 8) & 0xff * 32 / 255;
	uint16_t b = (c >> 0) & 0xff * 32 / 255;
	return MAKE_COLOR(r, g, b);
}


/*
static void fill_with_rainbow(struct leds_t* leds)
{
	for (int i=0; i<leds->num_leds; ++i)
		leds->leds[i] = rainbow((float)i/(float)leds->num_leds);
}
*/


static void fill_with_color(struct leds_t* leds, int from, int cnt, uint32_t color)
{
	for (int i=from; i<from+cnt; ++i)
		leds->leds[i] = color;
}

static void fill_with_colors(struct leds_t* leds, int from, int cnt, uint32_t colors[])
{
	uint32_t* color = colors;
	for (int i=from; i<from+cnt; ++i)
	{
		leds->leds[i] = *color;
		if (*++color == 0xffffffff)
			color = colors;
	}
}


static void pulsing_red(struct leds_t* leds)
{
	const TickType_t delay = 35 / portTICK_PERIOD_MS;

	// fade in
	for (int i=4; i<=64; ++i)
	{
		CHECK_DO_STOP(return);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, leds->num_leds, (i<<16));
		xSemaphoreGive(leds->sem);

		vTaskDelay(delay);
	}
	// fade out
	for (int i=64; i>=4; --i)
	{
		CHECK_DO_STOP(return);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, leds->num_leds, (i<<16));
		xSemaphoreGive(leds->sem);

		vTaskDelay(delay);
	}
}


static void pulsing_rgb(struct leds_t* leds, int delay_ms)
{
	int count = leds->num_leds;
	uint32_t rgb[] = { RED_M, GREEN_M, BLUE_M, 0xffffffff };
	uint32_t brg[] = { BLUE_M, RED_M, GREEN_M, 0xffffffff };
	uint32_t gbr[] = { GREEN_M, BLUE_M, RED_M, 0xffffffff };

	xSemaphoreTake(leds->sem, portMAX_DELAY);
	fill_with_colors(leds, 0, count, rgb);
	xSemaphoreGive(leds->sem);

	CHECK_DO_STOP(return);
	vTaskDelay(delay_ms / portTICK_PERIOD_MS);

	xSemaphoreTake(leds->sem, portMAX_DELAY);
	fill_with_colors(leds, 0, count, brg);
	xSemaphoreGive(leds->sem);

	CHECK_DO_STOP(return);
	vTaskDelay(delay_ms / portTICK_PERIOD_MS);

	xSemaphoreTake(leds->sem, portMAX_DELAY);
	fill_with_colors(leds, 0, count, gbr);
	xSemaphoreGive(leds->sem);

	CHECK_DO_STOP(return);
	vTaskDelay(delay_ms / portTICK_PERIOD_MS);
}


static void run_demo(struct leds_t* leds)
{
	int m = 0;

	for (;;)
	{
		CHECK_DO_STOP(return);

		if (m != COLORS_DEMO)
			run_mode(m, leds, true);
		m = (m + 1) % COLORS_NUM_MODES;

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


static void run_snake(struct leds_t* leds, COLOR color, int delay_ms)
{
	uint32_t c_n_b[] = { color, BLACK, 0xffffffff };
	uint32_t b_n_c[] = { BLACK, color, 0xffffffff };

	xSemaphoreTake(leds->sem, portMAX_DELAY);
	fill_with_colors(leds, 0, leds->num_leds, c_n_b);
	xSemaphoreGive(leds->sem);

	CHECK_DO_STOP(return);
	vTaskDelay(delay_ms / portTICK_PERIOD_MS);

	xSemaphoreTake(leds->sem, portMAX_DELAY);
	fill_with_colors(leds, 0, leds->num_leds, b_n_c);
	xSemaphoreGive(leds->sem);

	CHECK_DO_STOP(return);
	vTaskDelay(delay_ms / portTICK_PERIOD_MS);
}



void colors_set_mode(int mode)
{
	ESP_LOGI(MY_TAG, "new mode: %d", mode);

	xSemaphoreTake(leds.sem, portMAX_DELAY);
	m_mode = mode;
	xSemaphoreGive(leds.sem);

	m_do_stop = true;
}


void colors_next_mode()
{
	xSemaphoreTake(leds.sem, portMAX_DELAY);
	m_mode = (m_mode + 1) % COLORS_NUM_MODES;
	xSemaphoreGive(leds.sem);

	m_do_stop = true;
}


const char* colors_mode_desc(int mode)
{
	switch (mode)
	{
	case COLORS_BLACK: return "Black\n";
	case COLORS_RED: return "Red\n";
	case COLORS_GREEN: return "Green\n";
	case COLORS_BLUE: return "Blue\n";
	case COLORS_DEMO: return "Demo\n";
	case COLORS_WHITE: return "White\n";
	case COLORS_SNAKE_BLUE: return "SnakeBlue\n";
	case COLORS_SNAKE_WHITE: return "SnakeWhite\n";
	case COLORS_FILL_BLUE: return "FillBlue\n";
	case COLORS_FILL_BLUE_R: return "FillBlueR\n";
	case COLORS_RAINBOW	: return "Rainbow\n";
	case COLORS_PULSING_RED: return "PulsingRed\n";
	case COLORS_PULSING_RGB: return "PulsingRGB\n";
	case COLORS_WHITE_NOISE: return "WhiteNoise\n";
	case COLORS_XXX: return "XXX\n";
	case COLORS_MSGEQ7: return "Disco\n";
	}

	return "UNKNOWN\n";
}


static void colors_task(void* arg)
{
    struct leds_t* leds = (struct leds_t*)arg;

	int last_mode = COLORS_NUM_MODES; // forces a first run
	for (;;)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		if (last_mode == m_mode)
			continue;
		last_mode = m_mode;
		m_do_stop = false;

		if (m_mode == COLORS_DEMO)
			run_demo(leds);
		else
			run_mode(m_mode, leds, false);

	}
}


static void run_fill(struct leds_t* leds, COLOR c, int delay_ms)
{
	for (int i=0; i<leds->num_leds; ++i)
	{
		CHECK_DO_STOP(return);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		leds->leds[i] = c;
		xSemaphoreGive(leds->sem);

		vTaskDelay(delay_ms / portTICK_PERIOD_MS);
	}
}


static void run_xxx(struct leds_t* leds)
{
	// -- 1 -- fill with white
	fill_with_color(leds, 0, leds->num_leds, BLACK);
	run_fill(leds, WHITE_M, 50);

	CHECK_DO_STOP(return);
	vTaskDelay(200 / portTICK_PERIOD_MS);

	// -- 2 -- faid out
	for (int i=0x1f; i>=0; --i)
	{
		CHECK_DO_STOP(return);

		COLOR c = MAKE_COLOR(i, i, i);

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, leds->num_leds, c);
		xSemaphoreGive(leds->sem);

		vTaskDelay(30 / portTICK_PERIOD_MS);
	}
}


static void run_disco(struct leds_t* leds)
{
	int leds_per_band = leds->num_leds / MSGEQ_NUM_BANDS;

	uint32_t bands[MSGEQ_NUM_BANDS];
	uint32_t prev_bands[MSGEQ_NUM_BANDS];

	unsigned int nr_runs = 0;
	for (;;)
	{
		msgeq7_get(bands);

		int color = BLACK;
		int led = 0;

		xSemaphoreTake(leds->sem, portMAX_DELAY);
		for (int i=0; i<MSGEQ_NUM_BANDS; ++i)
		{
			int level = bands[i];
			prev_bands[i] = level;

			if (level < 1000)
				color = BLACK;
			else if (level < 2000)
				color = BLUE_M;
			else if (level < 3000)
				color = GREEN_M;
			else if (level < 4000)
				color = RED_M;
			else
				color = WHITE_M;

			for (int k=0; k<leds_per_band; ++k)
				leds->leds[led++] = color;
		}
		xSemaphoreGive(leds->sem);

		vTaskDelay(50 / portTICK_PERIOD_MS);
		CHECK_DO_STOP(return);

		// display
		if ((++nr_runs % 2) == 0)
		{
			printf(">>");
			for (int i=0; i<MSGEQ_NUM_BANDS; ++i)
				printf("\t%u", bands[i]);
			printf("\n");
		}
	}
}


static void run_mode(int m, struct leds_t* leds, bool single_run)
{
	int count = leds->num_leds;

	switch (m)
	{
	case COLORS_BLACK:
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, BLACK);
		xSemaphoreGive(leds->sem);
		break;
	case COLORS_RED:
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, RED_M);
		xSemaphoreGive(leds->sem);
		break;
	case COLORS_GREEN:
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, GREEN_M);
		xSemaphoreGive(leds->sem);
		break;
	case COLORS_BLUE:
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, BLUE_M);
		xSemaphoreGive(leds->sem);
		break;
	case COLORS_WHITE:
		xSemaphoreTake(leds->sem, portMAX_DELAY);
		fill_with_color(leds, 0, count, WHITE_M);
		xSemaphoreGive(leds->sem);
		break;
	case COLORS_SNAKE_BLUE:
		while (!m_do_stop)
		{
			run_snake(leds, BLUE_M, 1000);
			if (single_run)
				break;
		}
		break;
	case COLORS_SNAKE_WHITE:
		while (!m_do_stop)
		{
			run_snake(leds, WHITE_M, 1000);
			if (single_run)
				break;
		}
		break;
	case COLORS_FILL_BLUE:
		while (!m_do_stop)
		{
			fill_with_color(leds, 0, count, BLACK);
			run_fill(leds, BLUE_M, 30);
			vTaskDelay(200 / portTICK_PERIOD_MS);

			if (single_run)
				break;
		}
		break;
	case COLORS_FILL_BLUE_R:
		while (!m_do_stop)
		{
			fill_with_color(leds, 0, count, BLACK);
			for (int i=0; i<count; ++i)
			{
				CHECK_DO_STOP(return);

				xSemaphoreTake(leds->sem, portMAX_DELAY);
				leds->leds[count-i-1] = BLUE_M;
				xSemaphoreGive(leds->sem);

				vTaskDelay(30 / portTICK_PERIOD_MS);
			}
			vTaskDelay(200 / portTICK_PERIOD_MS);

			if (single_run)
				break;
		}
		break;
	case COLORS_RAINBOW:
		fill_with_color(leds, 0, count, BLACK);
		while (!m_do_stop)
		{
			float h = 1.0;
			int steps = 100;
			float dh = h / (float)steps;
			for (int i=0; i<steps; ++i)
			{
				CHECK_DO_STOP(return);

				xSemaphoreTake(leds->sem, portMAX_DELAY);
				fill_with_color(leds, 0, count, rainbow(h));
				xSemaphoreGive(leds->sem);

				h -= dh;
				vTaskDelay(30 / portTICK_PERIOD_MS);
			}

			if (single_run)
				break;
		}
		break;
	case COLORS_PULSING_RED:
		fill_with_color(leds, 0, count, BLACK);
		while (!m_do_stop)
		{
			pulsing_red(leds);

			if (single_run)
				break;
		}
		break;
	case COLORS_PULSING_RGB:
		fill_with_color(leds, 0, count, BLACK);
		while (!m_do_stop)
		{
			pulsing_rgb(leds, 1000);

			if (single_run)
				break;
		}
		break;
	case COLORS_WHITE_NOISE:
		fill_with_color(leds, 0, count, BLACK);
		while (!m_do_stop)
		{
			xSemaphoreTake(leds->sem, portMAX_DELAY);
			for (int i=0; i<count; ++i)
				leds->leds[i] = random_middle();
			xSemaphoreGive(leds->sem);

			vTaskDelay(250 / portTICK_PERIOD_MS);

			if (single_run)
				break;
		}
		break;
	case COLORS_XXX:
		while (!m_do_stop)
		{
			run_xxx(leds);

			if (single_run)
				break;
		}
		break;
	case COLORS_MSGEQ7:
		while (!m_do_stop)
		{
			run_disco(leds);
			if (single_run)
				break;
		}
		break;
	}
}




void colors_init()
{
	leds.sem = xSemaphoreCreateBinary();
	leds.num_leds = 60;
	leds.num_leds = 150;
	leds.leds = malloc(sizeof(uint32_t) * leds.num_leds);
	memset(leds.leds, 0, sizeof(uint32_t) * leds.num_leds);

	ws2812b_init(&leds);

    xTaskCreate(colors_task, "colors_task", 2048, &leds, 10, NULL);
}
