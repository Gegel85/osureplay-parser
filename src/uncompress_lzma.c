//
// Created by Gegel85 on 29/01/2019.
//

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <common.h>
#include <decompress.h>
#include <stdio.h>
#include "decompressor.h"

/* an input callback that will be passed to elzma_decompress_run(),
* it reads from a memory buffer */
int inputCallback(void *_ctx, void *_buf, size_t *_size)
{
	size_t rd = 0;
	struct DataStream *stream = (struct DataStream*)_ctx;
	assert(stream != NULL);

	rd = (stream->inLen < *_size) ? stream->inLen : *_size;
	if (rd > 0)
	{
		memcpy(_buf, (void *)stream->inData, rd);
		stream->inData += rd;
		stream->inLen -= rd;
	}

	*_size = rd;

	return 0;
}

/* an ouput callback that will be passed to elzma_decompress_run(),
* it reallocs and writes to a memory buffer */
size_t outputCallback(void *_ctx, const void *_buf, size_t _size)
{
	struct DataStream *stream = _ctx;
	assert(stream != NULL);

	if (_size > 0)
	{
		stream->outData = realloc(stream->outData, stream->outLen + _size);
		memcpy(stream->outData + stream->outLen, _buf, _size);
		stream->outLen += _size;
	}

	return _size;
}


int	decompressStreamData(struct DataStream *_stream)
{
	elzma_decompress_handle handle = elzma_decompress_alloc();

	if (handle == NULL) {
		printf("Memory allocation error");
		abort();
	}
	int ret = elzma_decompress_run(handle, inputCallback, _stream, outputCallback, _stream, ELZMA_lzma);

	if (ret != ELZMA_E_OK)
	{
		if (_stream->outData != NULL)
			free(_stream->outData);
		elzma_decompress_free(&handle);
	}

	return ret;
}
