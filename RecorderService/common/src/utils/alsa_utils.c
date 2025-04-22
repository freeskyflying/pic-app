#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>

#include "pnt_log.h"
#include "alsa_utils.h"

#define DEFAULT_FORMAT		SND_PCM_FORMAT_U8
#define DEFAULT_SPEED		8000
#define FORMAT_DEFAULT		-1
#define FORMAT_RAW		0
#define FORMAT_VOC		1
#define FORMAT_WAVE		2
#define FORMAT_AU		3

/*
 * make sure we write all bytes or return an error
 */
static ssize_t xwrite(int fd, const void *buf, size_t count)
{
	ssize_t written;
	size_t offset = 0;

	while (offset < count) {
		written = write(fd, buf + offset, count - offset);
		if (written <= 0)
			return written;

		offset += written;
	};

	return offset;
}

static ssize_t safe_read(AlsaPlayHandle_t* player, int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0)
	{
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	
	return result;
}

static ssize_t pcm_write(AlsaPlayHandle_t* player, u_char *data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;

	if (count < player->chunk_size) {
		snd_pcm_format_set_silence(player->format, data + count * player->bits_per_frame / 8, (player->chunk_size - count) * player->channels);
		count = player->chunk_size;
	}

	while (count > 0) {
		r = snd_pcm_writei(player->handle, data, count);

		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
		} else if (r == -EPIPE) {
		} else if (r == -ESTRPIPE) {
		} else if (r < 0) {
			break;
		}
		if (r > 0) {
			result += r;
			count -= r;
			data += r * player->bits_per_frame / 8;
		}
	}
	return result;
}

static ssize_t voc_pcm_write(AlsaPlayHandle_t* player, int buffer_pos, u_char *data, size_t count)
{
	ssize_t result = count, r;
	size_t size;

	while (count > 0) {
		size = count;
		if (size > player->chunk_bytes - buffer_pos)
			size = player->chunk_bytes - buffer_pos;
		memcpy(player->audiobuf + buffer_pos, data, size);
		data += size;
		count -= size;
		buffer_pos += size;
		if ((size_t)buffer_pos == player->chunk_bytes) {
			if ((size_t)(r = pcm_write(player, player->audiobuf, player->chunk_size)) != player->chunk_size)
				return r;
			buffer_pos = 0;
		}
	}
	return result;
}

static void voc_write_silence(AlsaPlayHandle_t* player, int buffer_pos, unsigned x)
{
	unsigned l;
	u_char *buf;

	buf = (u_char *) malloc(player->chunk_bytes);
	if (buf == NULL) {
		return;		/* not fatal error */
	}
	snd_pcm_format_set_silence(player->format, buf, player->chunk_size * player->channels);
	while (x > 0) {
		l = x;
		if (l > player->chunk_size)
			l = player->chunk_size;
		if (voc_pcm_write(player, buffer_pos, buf, l) != (ssize_t)l) {
			break;
		}
		x -= l;
	}
	free(buf);
}

/*
 * Test, if it is a .VOC file and return >=0 if ok (this is the length of rest)
 *                                       < 0 if not 
 */
static int test_vocfile(AlsaPlayHandle_t* player, void *buffer)
{
	VocHeader *vp = buffer;

	if (!memcmp(vp->magic, VOC_MAGIC_STRING, 20)) {
		player->vocminor = LE_SHORT(vp->version) & 0xFF;
		player->vocmajor = LE_SHORT(vp->version) / 256;
		if (LE_SHORT(vp->version) != (0x1233 - LE_SHORT(vp->coded_ver)))
			return -2;	/* coded version mismatch */
		return LE_SHORT(vp->headerlen) - sizeof(VocHeader);	/* 0 mostly */
	}
	return -1;		/* magic string fail */
}

/*
 * helper for test_wavefile
 */

static size_t test_wavefile_read(AlsaPlayHandle_t* player, int fd, u_char *buffer, size_t *size, size_t reqsize, int line)
{
	if (*size >= reqsize)
		return *size;
	if ((size_t)safe_read(player, fd, buffer + *size, reqsize - *size) != reqsize - *size) {
		return 0;
	}
	return *size = reqsize;
}

#define check_wavefile_space(buffer, len, blimit) \
	if (len > blimit) { \
		blimit = len; \
		if ((buffer = realloc(buffer, blimit)) == NULL) { \
			PNT_LOGE("not enough memory");		  \
		} \
	}

/*
 * test, if it's a .WAV file, > 0 if ok (and set the speed, stereo etc.)
 *                            == 0 if not
 * Value returned is bytes to be discarded.
 */
static ssize_t test_wavefile(AlsaPlayHandle_t* player, int fd, u_char *_buffer, size_t size)
{
	WaveHeader *h = (WaveHeader *)_buffer;
	u_char *buffer = NULL;
	size_t blimit = 0;
	WaveFmtBody *f;
	WaveChunkHeader *c;
	u_int type, len;
	unsigned short format, channels;
	int big_endian, native_format;

	if (size < sizeof(WaveHeader))
		return -1;
	if (h->magic == WAV_RIFF)
		big_endian = 0;
	else if (h->magic == WAV_RIFX)
		big_endian = 1;
	else
		return -1;
	if (h->type != WAV_WAVE)
		return -1;

	if (size > sizeof(WaveHeader)) {
		check_wavefile_space(buffer, size - sizeof(WaveHeader), blimit);
		memcpy(buffer, _buffer + sizeof(WaveHeader), size - sizeof(WaveHeader));
	}
	size -= sizeof(WaveHeader);
	while (1) {
		check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
		test_wavefile_read(player, fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__);
		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = TO_CPU_INT(c->length, big_endian);
		len += len % 2;
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_FMT)
			break;
		check_wavefile_space(buffer, len, blimit);
		test_wavefile_read(player, fd, buffer, &size, len, __LINE__);
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

	if (len < sizeof(WaveFmtBody)) {
		return -1;
	}
	check_wavefile_space(buffer, len, blimit);
	test_wavefile_read(player, fd, buffer, &size, len, __LINE__);
	f = (WaveFmtBody*) buffer;
	format = TO_CPU_SHORT(f->format, big_endian);
	if (format == WAV_FMT_EXTENSIBLE) {
		WaveFmtExtensibleBody *fe = (WaveFmtExtensibleBody*)buffer;
		if (len < sizeof(WaveFmtExtensibleBody)) {
			return -1;
		}
		if (memcmp(fe->guid_tag, WAV_GUID_TAG, 14) != 0) {
			return -1;
		}
		format = TO_CPU_SHORT(fe->guid_format, big_endian);
	}
	if (format != WAV_FMT_PCM &&
	    format != WAV_FMT_IEEE_FLOAT) {
		return -1;
	}
	channels = TO_CPU_SHORT(f->channels, big_endian);
	if (channels < 1) {
		return -1;
	}
	player->channels = channels;
	switch (TO_CPU_SHORT(f->bit_p_spl, big_endian)) {
	case 8:
		if (player->format != DEFAULT_FORMAT &&
		    player->format != SND_PCM_FORMAT_U8)
			PNT_LOGI("Warning: format is changed to U8");
		player->format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		if (big_endian)
			native_format = SND_PCM_FORMAT_S16_BE;
		else
			native_format = SND_PCM_FORMAT_S16_LE;
		if (player->format != DEFAULT_FORMAT &&
		    player->format != native_format)
			PNT_LOGI("Warning: format is changed to %s",
				snd_pcm_format_name(native_format));
		player->format = native_format;
		break;
	case 24:
		switch (TO_CPU_SHORT(f->byte_p_spl, big_endian) / player->channels) {
		case 3:
			if (big_endian)
				native_format = SND_PCM_FORMAT_S24_3BE;
			else
				native_format = SND_PCM_FORMAT_S24_3LE;
			if (player->format != DEFAULT_FORMAT &&
			    player->format != native_format)
				PNT_LOGI("Warning: format is changed to %s",
					snd_pcm_format_name(native_format));
			player->format = native_format;
			break;
		case 4:
			if (big_endian)
				native_format = SND_PCM_FORMAT_S24_BE;
			else
				native_format = SND_PCM_FORMAT_S24_LE;
			if (player->format != DEFAULT_FORMAT &&
			    player->format != native_format)
				PNT_LOGI("Warning: format is changed to %s",
					snd_pcm_format_name(native_format));
			player->format = native_format;
			break;
		default:
			return -1;
		}
		break;
	case 32:
		if (format == WAV_FMT_PCM) {
			if (big_endian)
				native_format = SND_PCM_FORMAT_S32_BE;
			else
				native_format = SND_PCM_FORMAT_S32_LE;
                        player->format = native_format;
		} else if (format == WAV_FMT_IEEE_FLOAT) {
			if (big_endian)
				native_format = SND_PCM_FORMAT_FLOAT_BE;
			else
				native_format = SND_PCM_FORMAT_FLOAT_LE;
			player->format = native_format;
		}
		break;
	default:
		return -1;
	}
	player->rate = TO_CPU_INT(f->sample_fq, big_endian);
	
	if (size > len)
		memmove(buffer, buffer + len, size - len);
	size -= len;
	
	while (1) {
		u_int type, len;

		check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
		test_wavefile_read(player, fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__);
		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = TO_CPU_INT(c->length, big_endian);
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_DATA) {
			if (len < player->pbrec_count && len < 0x7ffffffe)
				player->pbrec_count = len;
			if (size > 0)
				memcpy(_buffer, buffer, size);
			free(buffer);
			return size;
		}
		len += len % 2;
		check_wavefile_space(buffer, len, blimit);
		test_wavefile_read(player, fd, buffer, &size, len, __LINE__);
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

	/* shouldn't be reached */
	return -1;
}

/*

 */

static int test_au(AlsaPlayHandle_t* player, int fd, void *buffer)
{
	AuHeader *ap = buffer;

	if (ap->magic != AU_MAGIC)
		return -1;
	if (BE_INT(ap->hdr_size) > 128 || BE_INT(ap->hdr_size) < 24)
		return -1;
	player->pbrec_count = BE_INT(ap->data_size);
	switch (BE_INT(ap->encoding)) {
	case AU_FMT_ULAW:
		if (player->format != DEFAULT_FORMAT &&
		    player->format != SND_PCM_FORMAT_MU_LAW)
			PNT_LOGI("Warning: format is changed to MU_LAW\n");
		player->format = SND_PCM_FORMAT_MU_LAW;
		break;
	case AU_FMT_LIN8:
		if (player->format != DEFAULT_FORMAT &&
		    player->format != SND_PCM_FORMAT_U8)
			PNT_LOGI("Warning: format is changed to U8\n");
		player->format = SND_PCM_FORMAT_U8;
		break;
	case AU_FMT_LIN16:
		if (player->format != DEFAULT_FORMAT &&
		    player->format != SND_PCM_FORMAT_S16_BE)
			PNT_LOGI("Warning: format is changed to S16_BE\n");
		player->format = SND_PCM_FORMAT_S16_BE;
		break;
	default:
		return -1;
	}
	player->rate = BE_INT(ap->sample_rate);
	if (player->rate < 2000 || player->rate > 256000)
		return -1;
	player->channels = BE_INT(ap->channels);
	if (player->channels < 1 || player->channels > 256)
		return -1;
	if ((size_t)safe_read(player, fd, buffer + sizeof(AuHeader), BE_INT(ap->hdr_size) - sizeof(AuHeader)) != BE_INT(ap->hdr_size) - sizeof(AuHeader))
	{
		return -1;
	}
	return 0;
}

static int read_header(AlsaPlayHandle_t* player, int fd, int *loaded, int header_size)
{
	int ret;
	struct stat buf;

	ret = fstat(fd, &buf);
	if (ret < 0)
	{
		return -1;
	}

	/* don't be adventurous, get out if file size is smaller than
	 * requested header size */
	if ((buf.st_mode & S_IFMT) == S_IFREG && buf.st_size < header_size)
		return -1;

	if (*loaded < header_size)
	{
		header_size -= *loaded;
		ret = safe_read(player, fd, player->audiobuf + *loaded, header_size);
		if (ret != header_size)
		{
			return -1;
		}
		*loaded += header_size;
	}
	return 0;
}

static int set_params(AlsaPlayHandle_t* player)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(player->handle, params);
	if (err < 0) {
		PNT_LOGE("Broken configuration for this PCM: no configurations available");
		return -1;
	}

	err = snd_pcm_hw_params_set_access(player->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		PNT_LOGE("Access type not available");
		return -1;
	}
	err = snd_pcm_hw_params_set_format(player->handle, params, player->format);
	if (err < 0) {
		PNT_LOGE("Sample format non available");
		return -1;
	}
	err = snd_pcm_hw_params_set_channels(player->handle, params, player->channels);
	if (err < 0) {
		PNT_LOGE("Channels count non available");
		return -1;
	}

	rate = player->rate;
	err = snd_pcm_hw_params_set_rate_near(player->handle, params, &player->rate, 0);
	if (err < 0) { return -1;}
	if ((float)rate * 1.05 < player->rate || (float)rate * 0.95 > player->rate) {
		if (1/*!quiet_mode*/) {
			char plugex[64];
			const char *pcmname = snd_pcm_name(player->handle);
			if (! pcmname || strchr(snd_pcm_name(player->handle), ':'))
				*plugex = 0;
		}
	}
	rate = player->rate;
	if (player->buffer_time == 0 && player->buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
							    &player->buffer_time, 0);
		if (err < 0) { return -1;}
		if (player->buffer_time > 500000)
			player->buffer_time = 500000;
	}
	if (player->period_time == 0 && player->period_frames == 0) {
		if (player->buffer_time > 0)
			player->period_time = player->buffer_time / 4;
		else
			player->period_frames = player->buffer_frames / 4;
	}
	if (player->period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(player->handle, params,
							     &player->period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(player->handle, params,
							     &player->period_frames, 0);
	if (err < 0) { return -1;}
	if (player->buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(player->handle, params,
							     &player->buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(player->handle, params,
							     &player->buffer_frames);
	}
	if (err < 0) { return -1;}
	player->monotonic = snd_pcm_hw_params_is_monotonic(params);
	player->can_pause = snd_pcm_hw_params_can_pause(params);
	err = snd_pcm_hw_params(player->handle, params);
	if (err < 0) {
		return -1;
	}
	snd_pcm_hw_params_get_period_size(params, &player->chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (player->chunk_size == buffer_size) {
		return -1;
	}
	err = snd_pcm_sw_params_current(player->handle, swparams);
	if (err < 0) {
		return -1;
	}
	if (player->avail_min < 0)
		n = player->chunk_size;
	else
		n = (double) rate * player->avail_min / 1000000;
	err = snd_pcm_sw_params_set_avail_min(player->handle, swparams, n);

	/* round up to closest transfer boundary */
	n = buffer_size;
	if (player->start_delay <= 0) {
		start_threshold = n + (double) rate * player->start_delay / 1000000;
	} else
		start_threshold = (double) rate * player->start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(player->handle, swparams, start_threshold);
	if (err < 0) { return -1;}
	if (player->stop_delay <= 0) 
		stop_threshold = buffer_size + (double) rate * player->stop_delay / 1000000;
	else
		stop_threshold = (double) rate * player->stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(player->handle, swparams, stop_threshold);
	if (err < 0) { return -1;}

	if (snd_pcm_sw_params(player->handle, swparams) < 0) {
		return -1;
	}

	player->bits_per_sample = snd_pcm_format_physical_width(player->format);
	player->significant_bits_per_sample = snd_pcm_format_width(player->format);
	player->bits_per_frame = player->bits_per_sample * player->channels;
	player->chunk_bytes = player->chunk_size * player->bits_per_frame / 8;

	free(player->audiobuf);
	player->audiobuf = NULL;
	player->audiobuf = (u_char*)malloc(player->chunk_bytes);
	if (player->audiobuf == NULL) {
		return -1;
	}

	player->buffer_frames = buffer_size;	/* for position test */

	return 0;
}

static void playback_go(AlsaPlayHandle_t* player, int fd, size_t loaded, int64_t count, int rtype)
{
	int l, r;
	int64_t written = 0;
	int64_t c;

	if (set_params(player))
	{
		PNT_LOGE("set params failed.");
		return;
	}

	while (loaded > player->chunk_bytes && written < count) {
		if (pcm_write(player, player->audiobuf + written, player->chunk_size) <= 0)
			return;
		written += player->chunk_bytes;
		loaded -= player->chunk_bytes;
	}
	if (written > 0 && loaded > 0)
		memmove(player->audiobuf, player->audiobuf + written, loaded);

	l = loaded;
	while (written < count) {
		do {
			c = count - written;
			if (c > player->chunk_bytes)
				c = player->chunk_bytes;

			/* c < l, there is more data loaded
			 * then we actually need to write
			 */
			if (c < l)
				l = c;

			c -= l;

			if (c == 0)
				break;
			r = safe_read(player, fd, player->audiobuf + l, c);
			if (r < 0) {
				break;
			}
			player->fdcount += r;
			if (r == 0)
				break;
			l += r;
		} while ((size_t)l < player->chunk_bytes);
		l = l * 8 / player->bits_per_frame;
		r = pcm_write(player, player->audiobuf, l);
		if (r != l)
			break;
		r = r * player->bits_per_frame / 8;
		written += r;
		l = 0;
	}
	
	snd_pcm_drain(player->handle);
}

/* calculate the data count to read from/to dsp */
static int64_t calc_count(AlsaPlayHandle_t* player)
{
	int64_t count;

	if (player->timelimit == 0)
		if (player->sampleslimit == 0)
			count = player->pbrec_count;
		else
			count = snd_pcm_format_size(player->format, player->sampleslimit * player->channels);
	else {
		count = snd_pcm_format_size(player->format, player->rate * player->channels);
		count *= (int64_t)player->timelimit;
	}
	return count < player->pbrec_count ? count : player->pbrec_count;
}

static int playback_au(AlsaPlayHandle_t* player, int fd, int *loaded)
{
	if (read_header(player, fd, loaded, sizeof(AuHeader)) < 0)
		return -1;

	if (test_au(player, fd, player->audiobuf) < 0)
		return -1;

	player->pbrec_count = calc_count(player);
	
	playback_go(player, fd, *loaded - sizeof(AuHeader), player->pbrec_count, FORMAT_AU);

	return 0;
}

static void voc_pcm_flush(AlsaPlayHandle_t* player, int buffer_pos)
{
	if (buffer_pos > 0) {
		size_t b;
		
		b = player->chunk_size;
		if (pcm_write(player, player->audiobuf, b) != (ssize_t)b)
			PNT_LOGE("voc_pcm_flush error");
	}
	snd_pcm_drain(player->handle);
}

static void voc_play(AlsaPlayHandle_t* player, int fd, int ofs)
{
	int l;
	VocBlockType *bp;
	VocVoiceData *vd;
	VocExtBlock *eb;
	size_t nextblock, in_buffer;
	u_char *data, *buf;
	char was_extended = 0, output = 0;
	u_short *sp, repeat = 0;
	int64_t filepos = 0;

#define COUNT(x)	nextblock -= x; in_buffer -= x; data += x
#define COUNT1(x)	in_buffer -= x; data += x

	data = buf = (u_char *)malloc(64 * 1024);
	int buffer_pos = 0;
	if (data == NULL) {
		return;
	}
	
	/* first we waste the rest of header, ugly but we don't need seek */
	while (ofs > (ssize_t)player->chunk_bytes) {
		if ((size_t)safe_read(player, fd, buf, player->chunk_bytes) != player->chunk_bytes) {
			return;
		}
		ofs -= player->chunk_bytes;
	}
	if (ofs) {
		if (safe_read(player, fd, buf, ofs) != ofs) {
			goto __end;
		}
	}
	player->format = DEFAULT_FORMAT;
	player->channels = 1;
	player->rate = DEFAULT_SPEED;
	set_params(player);

	in_buffer = nextblock = 0;
	while (1) {
Fill_the_buffer:	/* need this for repeat */
		if (in_buffer < 32) {
			/* move the rest of buffer to pos 0 and fill the buf up */
			if (in_buffer)
				memcpy(buf, data, in_buffer);
			data = buf;
			if ((l = safe_read(player, fd, buf + in_buffer, player->chunk_bytes - in_buffer)) > 0)
				in_buffer += l;
			else if (!in_buffer) {
				/* the file is truncated, so simulate 'Terminator' 
				   and reduce the datablock for safe landing */
				nextblock = buf[0] = 0;
				if (l == -1) {
					goto __end;
				}
			}
		}
		while (!nextblock) {	/* this is a new block */
			if (in_buffer < sizeof(VocBlockType))
				goto __end;
			bp = (VocBlockType *) data;
			COUNT1(sizeof(VocBlockType));
			nextblock = VOC_DATALEN(bp);
			
			output = 0;
			switch (bp->type) {
			case 0:
				return;		/* VOC-file stop */
			case 1:
				vd = (VocVoiceData *) data;
				COUNT1(sizeof(VocVoiceData));
				/* we need a SYNC, before we can set new SPEED, STEREO ... */

				if (!was_extended) {
					player->rate = (int) (vd->tc);
					player->rate = 1000000 / (256 - player->rate);
					if (vd->pack) {		/* /dev/dsp can't it */
						goto __end;
					}
					if (player->channels == 2)		/* if we are in Stereo-Mode, switch back */
						player->channels = 1;
				} else {	/* there was extended block */
					player->channels = 2;
					was_extended = 0;
				}
				set_params(player);
				break;
			case 2:	/* nothing to do, pure data */
				break;
			case 3:	/* a silence block, no data, only a count */
				sp = (u_short *) data;
				COUNT1(sizeof(u_short));
				player->rate = (int) (*data);
				COUNT1(1);
				player->rate = 1000000 / (256 - player->rate);
				set_params(player);
				voc_write_silence(player, buffer_pos, *sp);
				break;
			case 4:	/* a marker for syncronisation, no effect */
				sp = (u_short *) data;
				COUNT1(sizeof(u_short));
				break;
			case 5:	/* ASCII text, we copy to stderr */
				output = 1;

				break;
			case 6:	/* repeat marker, says repeatcount */
				/* my specs don't say it: maybe this can be recursive, but
				   I don't think somebody use it */
				repeat = *(u_short *) data;
				COUNT1(sizeof(u_short));

				if (filepos >= 0) {	/* if < 0, one seek fails, why test another */
					if ((filepos = lseek(fd, 0, 1)) < 0) {
						repeat = 0;
					} else {
						filepos -= in_buffer;	/* set filepos after repeat */
					}
				} else {
					repeat = 0;
				}
				break;
			case 7:	/* ok, lets repeat that be rewinding tape */
				if (repeat) {
					if (repeat != 0xFFFF) {
						--repeat;
					}
					lseek(fd, filepos, 0);
					in_buffer = 0;	/* clear the buffer */
					goto Fill_the_buffer;
				}
				break;
			case 8:	/* the extension to play Stereo, I have SB 1.0 :-( */
				was_extended = 1;
				eb = (VocExtBlock *) data;
				COUNT1(sizeof(VocExtBlock));
				player->rate = (int) (eb->tc);
				player->rate = 256000000L / (65536 - player->rate);
				player->channels = eb->mode == VOC_MODE_STEREO ? 2 : 1;
				if (player->channels == 2)
					player->rate = player->rate >> 1;
				if (eb->pack) {		/* /dev/dsp can't it */
					goto __end;
				}
				break;
			default:
				return;
			}	/* switch (bp->type) */
		}		/* while (! nextblock)  */
		/* put nextblock data bytes to dsp */
		l = in_buffer;
		if (nextblock < (size_t)l)
			l = nextblock;
		if (l) {
			if (output) {
				if (xwrite(2, data, l) != l) {	/* to stderr */
					goto __end;
				}
			} else {
				if (voc_pcm_write(player, buffer_pos, data, l) != l) {
					goto __end;
				}
			}
			COUNT(l);
		}
	}			/* while(1) */
__end:
    voc_pcm_flush(player, buffer_pos);
    free(buf);
}

static int playback_voc(AlsaPlayHandle_t* player, int fd, int *loaded)
{
	int ofs;

	if (read_header(player, fd, loaded, sizeof(VocHeader)) < 0)
		return -1;

	if ((ofs = test_vocfile(player, player->audiobuf)) < 0)
		return -1;

	player->pbrec_count = calc_count(player);
	
	voc_play(player, fd, ofs);

	return 0;
}

static int playback_wave(AlsaPlayHandle_t* player, int fd, int *loaded)
{
	ssize_t dtawave;

	if (read_header(player, fd, loaded, sizeof(WaveHeader)) < 0)
		return -1;

	if ((dtawave = test_wavefile(player, fd, player->audiobuf, *loaded)) < 0)
		return -1;

	player->pbrec_count = calc_count(player);
	playback_go(player, fd, dtawave, player->pbrec_count, FORMAT_WAVE);

	return 0;
}

static int playback_raw(AlsaPlayHandle_t* player, int fd, int *loaded)
{
	player->pbrec_count = calc_count(player);
	playback_go(player, fd, *loaded, player->pbrec_count, FORMAT_RAW);

	return 0;
}

static void alsa_playback(AlsaPlayHandle_t* player)
{
	int fd = -1;
	int loaded = 0;
	
	if ((fd = open(player->filename, O_RDONLY, 0)) == -1)
	{
		PNT_LOGE("open %s failed.", player->filename);
		return;
	}

	PNT_LOGI("start to play %s", player->filename);

	switch(player->file_type)
	{
		case FORMAT_AU:
			playback_au(player, fd, &loaded);
			break;
		case FORMAT_VOC:
			playback_voc(player, fd, &loaded);
			break;
		case FORMAT_WAVE:
			playback_wave(player, fd, &loaded);
			break;
		case FORMAT_RAW:
			playback_raw(player, fd, &loaded);
			break;
		default:
			/* parse the file header */
			if (playback_au(player, fd, &loaded) < 0 &&
			    playback_voc(player, fd, &loaded) < 0 &&
			    playback_wave(player, fd, &loaded) < 0)
				playback_raw(player, fd, &loaded); /* should be raw data */
			break;
	}

	close(fd);
}

static void alsa_play_audio(AlsaPlayHandle_t* player)
{
	int err = 0;
	
	err = snd_pcm_open(&player->handle, player->pcm_name, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0)
	{
		PNT_LOGE("audio open error: %s", snd_strerror(err));
		return;
	}

	alsa_playback(player);

	snd_pcm_close(player->handle);
}

void* alsa_play_audio_thread(void* parg)
{
	pthread_detach(pthread_self());
	
	AlsaPlayHandle_t* player = (AlsaPlayHandle_t*)parg;

	alsa_play_audio(player);
	
	free(player->audiobuf);
	free(player);
	
	return NULL;
}

int alsa_playaudio(char* filename, int sample_rate, int channels, char* format_str, int mode/* 1-block 0-nonblock */)
{
	snd_pcm_format_t format = snd_pcm_format_value(format_str);

	if (format == SND_PCM_FORMAT_UNKNOWN)
	{
		PNT_LOGE("error format str [%s]", format_str);
		return -1;
	}
		
	AlsaPlayHandle_t* player = (AlsaPlayHandle_t*)malloc(sizeof(AlsaPlayHandle_t));

	if (NULL == player)
	{
		PNT_LOGE("fatal error: malloc failed.");
		return -1;
	}

	memset(player, 0, sizeof(AlsaPlayHandle_t));

	strcpy(player->filename, filename);
	player->rate = sample_rate;
	player->channels = channels;
	player->format = format;
	player->avail_min = -1;
	player->file_type = FORMAT_RAW;
	player->pbrec_count = LLONG_MAX;

	strcpy(player->pcm_name, "default");

	player->audiobuf = (u_char*)malloc(1024);
	if (NULL == player->audiobuf)
	{
		PNT_LOGE("fatal error: malloc failed.");
		return -1;
	}

	if (mode)
	{
		alsa_play_audio(player);

		free(player->audiobuf);
		free(player);
	}
	else
	{
		pthread_t pid;
		pthread_create(&pid, NULL, alsa_play_audio_thread, player);
	}

	return 0;
}

