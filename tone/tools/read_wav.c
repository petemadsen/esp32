#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef unsigned int uint32_t;
typedef unsigned short uint16_t;


struct wav_header
{
	char riff[4];
	uint32_t filelen;
	char wav[4];
};


struct wav_fmt
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

struct wav_data
{
	char data[4];
	uint32_t len;
};


int main()
{
	FILE* file = fopen("belldoor.wav", "r");
	if (!file)
		return 1;

	fseek(file, 0, SEEK_END);
	long file_len = ftell(file);
	rewind(file);
	printf("--file size: %d\n", file_len);

	// -- header
	struct wav_header hdr;
	if (fread(&hdr, sizeof(char), 12, file) != 12)
		return 2;
	if (strncmp(hdr.riff, "RIFF", 4) != 0)
		return 3;
	if (file_len != hdr.filelen + 8)
		return 4;
	if (strncmp(hdr.wav, "WAVE", 4) != 0)
		return 5;
	printf("--%c%c%c%c\n", hdr.riff[0], hdr.riff[1], hdr.riff[2], hdr.riff[3]);
	printf("--%u\n", hdr.filelen);
	printf("--%c%c%c%c\n", hdr.wav[0], hdr.wav[1], hdr.wav[2], hdr.wav[3]);

	// -- fmt
	struct wav_fmt fmt;
	if (fread(&fmt, sizeof(char), 24, file) != 24)
		return 10;
	if (strncmp(fmt.fmt, "fmt ", 4) != 0)
		return 11;
	printf("\n");
	printf("--%c%c%c%c\n", fmt.fmt[0], fmt.fmt[1], fmt.fmt[2], fmt.fmt[3]);
	printf("--fmt len......: %u\n", fmt.len); // 16 bytes
	printf("--fmt tag......: 0x%x (pcm: %d)\n", fmt.fmttag, fmt.fmttag==1);
	printf("--nr channels..: %u\n", fmt.channels);
	printf("--sample rate..: %u\n", fmt.samplerate);
	printf("--bytes/sec....: %u\n", fmt.bytes_per_second);
	printf("--block align..: %u\n", fmt.block_align);
	printf("--bits/sample..: %u\n", fmt.bits_per_sample);
	if (fmt.bits_per_sample != 16)
		return 12;

	// -- data
	struct wav_data data;
	if (fread(&data, sizeof(char), 8, file) != 8)
		return 20;
	if (strncmp(data.data, "data", 4) != 0)
		return 21;
	printf("\n");
	printf("--%c%c%c%c\n", data.data[0], data.data[1], data.data[2], data.data[3]);
	printf("--data len.....: %u\n", data.len);

	const int target_bits = 8;
	const int sampwidth = fmt.bits_per_sample;
	for (size_t i = 0; i < data.len / fmt.bits_per_sample / 8; ++i)
	{
		uint16_t sample;
		if (fread(&sample, sizeof(uint16_t), 1, file) != 1)
			return 22;

        int scale_val = (1 << target_bits) - 1;
        int cur_lim   = (1 << sampwidth) - 1;
        //#scale current data to 8-bit data
        int val       = sample * scale_val / cur_lim;
        val = (int)(val + ((scale_val + 1) / 2)) & scale_val;

		if (i < 10)
			printf(">>%u --> %u\n", sample, val);
	}

	fclose(file);
	return 0;
}
