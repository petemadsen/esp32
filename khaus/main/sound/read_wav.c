/**
 * This code is public domain.
 */
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "read_wav.h"
#include "common.h"


// 12 bytes
struct __attribute__((__packed__)) wav_header
{
	char riff[4];
	uint32_t filelen;
	char wav[4];
};


// 24 bytes
struct __attribute__((__packed__)) wav_fmt
{
	char fmt[4];
	uint32_t len;
	uint16_t fmttag;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t bytes_per_second;
	uint16_t block_align;
	uint16_t bits_per_sample;
};

// 8 bytes
struct __attribute__((__packed__)) wav_data
{
	char data[4];
	uint32_t len;
};

// 44 bytes
struct __attribute__((__packed__)) wav_all_headers
{
	struct wav_header hdr;
	struct wav_fmt fmt;
	struct wav_data data;
};


static int read_from_file(FILE* file, unsigned char** buf, size_t* buf_len);


unsigned char* read_wav(FILE* file, size_t* len)
{
	unsigned char* buf = NULL;
	int ret = read_from_file(file, &buf, len);
	if (ret != 0)
	{
		ESP_LOGE(PROJECT_TAG("read_wav"), "Error: %d", ret);
		if (buf)
			free(buf);
		buf = NULL;
	}
	return buf;
}


static int read_from_file(FILE* file, unsigned char** buf, size_t* len)
{
	fseek(file, 0, SEEK_END);
	long file_len = ftell(file);
	rewind(file);
	printf("--file size: %ld\n", file_len);
	if (file_len < 44)
		return 30;

	struct wav_all_headers hdr;
	if (fread(&hdr, sizeof(char), 44, file) != 44)
		return 31;
	int wav = read_wav_check(&hdr, file_len);
	if (wav != 0)
		return wav;

	// header seams to be okay. go on.
	const int target_bits = 8;
	const int sampwidth = hdr.fmt.bits_per_sample;
	int scale_val = (1 << target_bits) - 1;
	int cur_lim   = (1 << sampwidth) - 1;
	size_t num_samples = hdr.data.len / (hdr.fmt.bits_per_sample / 8);

	*len = num_samples;
	*buf = malloc(num_samples);
	unsigned char* p = *buf;

	printf("\n");
	printf("--malloc %zu -> %d\n", num_samples, (*buf == NULL));

	for (size_t i = 0; i < num_samples; ++i)
	{
		unsigned char val;

		if (hdr.fmt.bits_per_sample == 16)
		{
			uint16_t sample;
			if (fread(&sample, hdr.fmt.bits_per_sample / 8, 1, file) != 1)
				return 32;

			// scale current data to 8-bit data
			unsigned char val = sample * scale_val / cur_lim;
			val = (unsigned char)(val + ((scale_val + 1) / 2)) & scale_val;

			if (i < 25)
				printf(">>%d --> %u\n", sample, val);
		}
		else
		{
			if (fread(&val, hdr.fmt.bits_per_sample / 8, 1, file) != 1)
				return 33;
			if (i < 25)
				printf(">>%d\n", val);
		}

		*p++ = val;
	}

	fclose(file);
	return 0;
}


int read_wav_check(const char* buf, size_t buflen)
{
	if (buflen < 44)
		return 10;

	struct wav_all_headers* hdr = (struct wav_all_headers*)buf;

	// -- header
	if (strncmp(hdr->hdr.riff, "RIFF", 4) != 0)
		return 11;
	if (buflen != hdr->hdr.filelen + 8)
		return 12;
	if (strncmp(hdr->hdr.wav, "WAVE", 4) != 0)
		return 13;
	printf("--%c%c%c%c\n", hdr->hdr.riff[0], hdr->hdr.riff[1], hdr->hdr.riff[2], hdr->hdr.riff[3]);
	printf("--%u\n", hdr->hdr.filelen);
	printf("--%c%c%c%c\n", hdr->hdr.wav[0], hdr->hdr.wav[1], hdr->hdr.wav[2], hdr->hdr.wav[3]);

	// -- fmt
	if (strncmp(hdr->fmt.fmt, "fmt ", 4) != 0)
		return 21;
	printf("\n");
	printf("--%c%c%c%c\n", hdr->fmt.fmt[0], hdr->fmt.fmt[1], hdr->fmt.fmt[2], hdr->fmt.fmt[3]);
	printf("--fmt len......: %u\n", hdr->fmt.len); // 16 bytes
	printf("--fmt tag......: 0x%x (pcm: %d)\n", hdr->fmt.fmttag, hdr->fmt.fmttag==1);
	printf("--nr channels..: %u\n", hdr->fmt.channels);
	printf("--sample rate..: %u\n", hdr->fmt.samplerate);
	printf("--bytes/sec....: %u\n", hdr->fmt.bytes_per_second);
	printf("--block align..: %u\n", hdr->fmt.block_align);
	printf("--bits/sample..: %u\n", hdr->fmt.bits_per_sample);
	if (hdr->fmt.bits_per_sample != 16 && hdr->fmt.bits_per_sample != 8)
		return 22;

	// -- data
	if (strncmp(hdr->data.data, "data", 4) != 0)
		return 31;
	printf("\n");
	printf("--%c%c%c%c\n", hdr->data.data[0], hdr->data.data[1], hdr->data.data[2], hdr->data.data[3]);
	printf("--data len.....: %u\n", hdr->data.len);

	return 0;
}
