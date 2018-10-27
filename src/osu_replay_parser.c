#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <setjmp.h>
#include <limits.h>
#include <unistd.h>
#include <LzmaLib.h>
#include <ctype.h>
#include "osu_replay_parser.h"

char	*OsuReplay_gameModeToString(unsigned char mode)
{
	switch (mode) {
	case 0:
		return "osu! Standard";
	case 1:
		return "Taiko";
	case 2:
		return "Catch the Beat";
	case 3:
		return "osu!mania";
	default:
		return "Unknown mode";
	}
}

unsigned char 	*OsuReplay_getRawData(const unsigned char *buffer, size_t buffSize, size_t size, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned char	*data;

	if (*currentPos + size > buffSize) {
		*currentPos = buffSize;
		sprintf(err_buff, "Unexpected end of file\n");
		longjmp(jump_buffer, true);
	}
	data = malloc(size + 1);
	if (!data) {
		sprintf(err_buff, "Memory allocation error (%luB)\n", (unsigned long)size);
		longjmp(jump_buffer, true);
	}
	memcpy(data, &buffer[*currentPos], size);
	data[size] = 0;
	(*currentPos) += size;
	return data;
}

unsigned long	OsuReplay_getLongInteger(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned long	data = 0;

	for (int i = 0; i < 8; i++) {
		if (buffSize == *currentPos) {
			sprintf(err_buff, "Unexpected end of file\n");
			longjmp(jump_buffer, true);
		}
		data += (unsigned long)buffer[*currentPos] << (i * 8);
		(*currentPos)++;
	}
	return data;
}

unsigned int	OsuReplay_getVarLenInt(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned char	buff;
	unsigned int	data = 0;
	int		count = 0;

	do {
		if (count == 4) {
			sprintf(err_buff, "Variable length int is longer than 4 bytes\n");
			longjmp(jump_buffer, true);
		} else if (*currentPos == buffSize) {
			sprintf(err_buff, "Unexpected end of file\n");
			longjmp(jump_buffer, true);
		}
		buff = buffer[*currentPos];
		data += (buff & 0x7F) << (count++ * 7);
		(*currentPos)++;
	} while (buff & 0x80);
	return (data);
}

unsigned char	OsuReplay_getByte(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned char	data = 0;

	if (buffSize == *currentPos) {
		sprintf(err_buff, "Unexpected end of file\n");
		longjmp(jump_buffer, true);
	}
	data = buffer[*currentPos];
	(*currentPos)++;
	return data;
}

char 		*OsuReplay_getString(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned char	byte = OsuReplay_getByte(buffer, buffSize, currentPos, err_buff, jump_buffer);
	unsigned int	length;

	if (!byte)
		return NULL;
	else if (byte != 11) {
		(*currentPos)--;
		sprintf(err_buff, "An invalid byte was found\n");
		longjmp(jump_buffer, true);
	}
	length = OsuReplay_getVarLenInt(buffer, buffSize, currentPos, err_buff, jump_buffer);
	return (char *)OsuReplay_getRawData(buffer, buffSize, length, currentPos, err_buff, jump_buffer);
}

unsigned int	OsuReplay_getInteger(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned int	data = 0;

	for (int i = 0; i < 4; i++) {
		if (buffSize == *currentPos) {
			sprintf(err_buff, "Unexpected end of file\n");
			longjmp(jump_buffer, true);
		}
		data += buffer[*currentPos] << (i * 8);
		(*currentPos)++;
	}
	return data;
}

unsigned short	OsuReplay_getShort(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned short	data = 0;

	for (int i = 0; i < 2; i++) {
		if (buffSize == *currentPos) {
			sprintf(err_buff, "Unexpected end of file\n");
			longjmp(jump_buffer, true);
		}
		data += buffer[*currentPos] << (i * 8);
		(*currentPos)++;
	}
	return data;
}

size_t	OsuReplay_getPointerArraySize(void **array)
{
	size_t	size = 0;

	for (; array[size]; size++);
	return size;
}

char	**OsuReplay_splitString(char *str, char separator)
{
	void	*buffer;
	char	**array = malloc(2 * sizeof(*array));
	size_t	size = 1;

	*array = str;
	array[1] = NULL;
	for (int i = 0; str[i]; i++) {
		if (str[i] == separator) {
			str[i] = '\0';
			buffer = realloc(array, (++size + 1) * sizeof(*array));
			if (!buffer) {
				free(array);
				return NULL;
			}
			array = buffer;
			array[size - 1] = &str[i + 1];
			array[size] = NULL;
		}
	}
	return array;
}

bool	OsuReplay_isInt(char *str)
{
	int	i = 0;

	for (; str[i] == '+' || str[i] == '-'; i++);
	for (; isdigit(str[i]); i++);
	return str[i] == 0;
}

bool	OsuReplay_isFloat(char *str)
{
	int	i = 0;

	for (; str[i] == '+' || str[i] == '-'; i++);
	for (; isdigit(str[i]); i++);
	if (str[i] == '.')
		i++;
	for (; isdigit(str[i]); i++);
	return str[i] == 0;
}

OsuLifeEventArray	OsuReplay_parseLifeBarEvents(char *buffer, char *err_buff, jmp_buf jump_buffer)
{
	OsuLifeEventArray	events = {0, NULL};
	char			**eventArray = OsuReplay_splitString(buffer, ',');
	char			***numberArray;

	if (!eventArray) {
		sprintf(err_buff, "Memory allocation error\n");
		longjmp(jump_buffer, true);
	}

	events.length = OsuReplay_getPointerArraySize((void **)eventArray);
	numberArray = malloc(events.length * sizeof(*numberArray));
	if (!numberArray) {
		sprintf(err_buff, "Memory allocation error (%luB)\n", (unsigned long)(events.length * sizeof(*numberArray)));
		longjmp(jump_buffer, true);
	}

	memset(numberArray, 0, events.length * sizeof(*numberArray));
	for (int i = 0; eventArray[i]; i++) {
		if (!eventArray[i + 1] && !strlen(eventArray[i]))
			break;
		numberArray[i] = OsuReplay_splitString(eventArray[i], '|');
		if (OsuReplay_getPointerArraySize((void **)numberArray[i]) != 2) {
			sprintf(
				err_buff,
				"Lifebar events array is invalid. Expected 2 elements but %lu were given\n",
				(unsigned long)OsuReplay_getPointerArraySize((void **)numberArray[i])
			);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][0])) {
			sprintf(err_buff, "Invalid integer '%s'\n", numberArray[i][0]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][1])) {
			sprintf(err_buff, "Invalid floating number '%s'\n", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (atof(numberArray[i][1]) > 1 || atof(numberArray[i][1]) < 0) {
			sprintf(err_buff, "Invalid life value '%s' is not in range 0-1\n", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		}
	}

	events.content = malloc(events.length * sizeof(*events.content));
	if (!events.content) {
		sprintf(err_buff, "Memory allocation error (%luB)\n", (unsigned long)(events.length * sizeof(*events.content)));
		for (int i = 0; numberArray[i]; i++)
			free(numberArray[i]);
		free(numberArray);
		free(eventArray);
		longjmp(jump_buffer, true);
	}
	for (int i = 0; numberArray[i]; i++) {
		events.content[i].timeToHappen = DELNEG(atol(numberArray[i][0]) - events.content[i - 1].timeToHappen);
		events.content[i].newValue = atof(numberArray[i][1]);
		free(numberArray[i]);
	}
	free(eventArray);
	free(numberArray);
	return events;
}

OsuGameEventArray	OsuReplay_parseGameEvents(char *buffer, char *err_buff, jmp_buf jump_buffer)
{
	OsuGameEventArray	events = {0, NULL};
	char			**eventArray = OsuReplay_splitString(buffer, ',');
	char			***numberArray;

	if (!eventArray) {
		sprintf(err_buff, "Memory allocation error\n");
		longjmp(jump_buffer, true);
	}

	events.length = OsuReplay_getPointerArraySize((void **)eventArray);
	numberArray = malloc(events.length * sizeof(*numberArray));
	if (!numberArray) {
		sprintf(err_buff, "Memory allocation error (%luB)\n", (unsigned long)(events.length * sizeof(*numberArray)));
		longjmp(jump_buffer, true);
	}

	memset(numberArray, 0, events.length * sizeof(*numberArray));
	for (int i = 0; eventArray[i] && eventArray[i + 1]; i++) {
		if (!eventArray[i + 1] && !strlen(eventArray[i]))
			break;
		numberArray[i] = OsuReplay_splitString(eventArray[i], '|');
		if (OsuReplay_getPointerArraySize((void **)numberArray[i]) != 4) {
			sprintf(
				err_buff,
				"Game events array is invalid. Expected 4 elements but %lu were given\n",
				(unsigned long)OsuReplay_getPointerArraySize((void **)numberArray[i])
			);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][0])) {
			sprintf(err_buff, "Invalid integer '%s'\n", numberArray[i][0]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][1])) {
			sprintf(err_buff, "Invalid floating number '%s'\n", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][2])) {
			sprintf(err_buff, "Invalid floating number '%s'\n", numberArray[i][2]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][3])) {
			sprintf(err_buff, "Invalid integer '%s'\n", numberArray[i][3]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		}
	}

	events.content = malloc(events.length * sizeof(*events.content));
	if (!events.content) {
		sprintf(err_buff, "Memory allocation error (%luB)\n", (unsigned long)(events.length * sizeof(*events.content)));
		for (int i = 0; numberArray[i]; i++)
			free(numberArray[i]);
		free(numberArray);
		longjmp(jump_buffer, true);
	}
	for (int i = 0; numberArray[i]; i++) {
		events.content[i].timeToHappen = DELNEG(atol(numberArray[i][0]));
		events.content[i].cursorPos.x = atof(numberArray[i][1]);
		events.content[i].cursorPos.y = atof(numberArray[i][2]);
		events.content[i].keysPressed = atoi(numberArray[i][3]);
		free(numberArray[i]);
	}
	free(numberArray);
	return events;
}

OsuReplay	OsuReplay_parseReplayString(unsigned char *string, size_t buffSize)
{
	OsuReplay	result;
	size_t		currentPos = 0;
	size_t		posInCompressedBuffer = 0;
	jmp_buf		jump_buffer;
	static	char	error[PATH_MAX + 1024];
	char		buffer[PATH_MAX + 1024];
	int		LZMA_result;
	OsuString	compressedReplayData = {0, NULL};
	OsuString	uncompressedReplayData = {0, NULL};
	char		*lifeBar = NULL;

	//Init the error handler
	if (setjmp(jump_buffer)) {
		strcpy(buffer, error);
		sprintf(error, "An error occurred when parsing string:\nCharacter %lu: %s\n", (unsigned long)currentPos, buffer);
		result.error = error;
		free(compressedReplayData.content);
		free(uncompressedReplayData.content);
		free(lifeBar);
		return result;
	}

	//Grab all the data from the buffer
	memset(&result, 0, sizeof(result));
	result.mode = OsuReplay_getByte(string, buffSize, &currentPos, error, jump_buffer);
	if (result.mode > 3) {
		sprintf(error, "Invalid game mode found (%i)\n", result.mode);
		longjmp(jump_buffer, true);
	}
	result.version = OsuReplay_getInteger(string, buffSize, &currentPos, error, jump_buffer);
	result.mapHash = OsuReplay_getString(string, buffSize, &currentPos, error, jump_buffer);
	result.playerName = OsuReplay_getString(string, buffSize, &currentPos, error, jump_buffer);
	result.replayHash = OsuReplay_getString(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOf300 = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOf100 = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOf50 = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOfGekis = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOfKatus = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.nbOfMiss = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.totalScore = OsuReplay_getInteger(string, buffSize, &currentPos, error, jump_buffer);
	result.score.maxCombo = OsuReplay_getShort(string, buffSize, &currentPos, error, jump_buffer);
	result.score.noComboBreak = OsuReplay_getByte(string, buffSize, &currentPos, error, jump_buffer);
	result.mods = OsuReplay_getInteger(string, buffSize, &currentPos, error, jump_buffer);
	lifeBar = OsuReplay_getString(string, buffSize, &currentPos, error, jump_buffer);
	result.timestamp = OsuReplay_getLongInteger(string, buffSize, &currentPos, error, jump_buffer);
	compressedReplayData.length = OsuReplay_getInteger(string, buffSize, &currentPos, error, jump_buffer);
	compressedReplayData.content = OsuReplay_getRawData(string, buffSize, compressedReplayData.length, &currentPos, error, jump_buffer);
	result.something = OsuReplay_getLongInteger(string, buffSize, &currentPos, error, jump_buffer);
	if (buffSize != currentPos) {
		sprintf(error, "Invalid data remaining at the end of the file\n");
		longjmp(jump_buffer, true);
	}

	//Uncompress the replay data
	posInCompressedBuffer = compressedReplayData.length - LZMA_PROPS_SIZE - 8;
	uncompressedReplayData.length = 0;
	for (int i = 0; i < 8; i++)
		uncompressedReplayData.length += (size_t)compressedReplayData.content[LZMA_PROPS_SIZE + i] << (i * 8);
	uncompressedReplayData.content = malloc(uncompressedReplayData.length);
	if (!uncompressedReplayData.content) {
		sprintf(error, "Memory allocation error (%luB)\n", (unsigned long)uncompressedReplayData.length);
		longjmp(jump_buffer, true);
	}
	memset(uncompressedReplayData.content, 0, uncompressedReplayData.length);
	LZMA_result = LzmaUncompress(
		uncompressedReplayData.content,
		&posInCompressedBuffer,
		&compressedReplayData.content[LZMA_PROPS_SIZE + 8],
		&compressedReplayData.length,
		compressedReplayData.content,
		LZMA_PROPS_SIZE
	);

	//Handle uncompression errors
	if (LZMA_result == SZ_ERROR_DATA) {
		sprintf(error, "A data error occurred when uncompressing replay data (Byte %lu in compressed data)\n", (unsigned long)posInCompressedBuffer);
		longjmp(jump_buffer, true);
	} else if (LZMA_result == SZ_ERROR_MEM) {
		sprintf(error, "A memory allocation error occurred when uncompressing replay data (Byte %lu in compressed data)\n", (unsigned long)posInCompressedBuffer);
		longjmp(jump_buffer, true);
	} else if (LZMA_result == SZ_ERROR_UNSUPPORTED) {
		sprintf(error, "The compressed replay data are invalid (Byte %lu in compressed data)\n", (unsigned long)posInCompressedBuffer);
		longjmp(jump_buffer, true);
	} else if (LZMA_result == SZ_ERROR_INPUT_EOF) {
		sprintf(error, "Unexpected end of file when uncompressing replay data (Byte %lu in compressed data)\n", (unsigned long)posInCompressedBuffer);
		longjmp(jump_buffer, true);
	}

	//Parse uncompressed game events data and lifebar data
	result.lifeBar = OsuReplay_parseLifeBarEvents(lifeBar, error, jump_buffer);
	result.gameEvents = OsuReplay_parseGameEvents((char *)uncompressedReplayData.content, error, jump_buffer);

	free(uncompressedReplayData.content);
	free(compressedReplayData.content);
	free(lifeBar);
	return result;
}

OsuReplay	OsuReplay_parseReplayFile(char *path)
{
	size_t		size = 0;
	struct stat	stats;
	OsuReplay	result;
	static	char	error[PATH_MAX + 1024];
	FILE		*stream;
	int		fd;
	unsigned char	*buffer;
	int		readSize;

	//Open file
	stream = fopen(path, "rb");
	if (!stream) {
		sprintf(error, "%s: %s\n", path, strerror(errno));
		result.error = error;
		return result;
	}

	//Create buffer
	memset(&result, 0, sizeof(result));
	if (stat(path, &stats) < 0) {
		sprintf(error, "%s: %s\n", path, strerror(errno));
		result.error = error;
		return result;
	}

	size = stats.st_size;
	buffer = malloc(size);
	if (!buffer) {
		sprintf(error, "%s: Memory allocation error (%luB)\n", path, (long unsigned)size);
		result.error = error;
		return result;
	}

	//Read file
	fd = fileno(stream);
	readSize = read(fd, buffer, size);
	if (readSize < 0) {
		sprintf(error, "%s: %s\n", path, strerror(errno));
		result.error = error;
		return result;
	}
	fclose(stream);

	//Parse content
	result = OsuReplay_parseReplayString(buffer, readSize);
	if (result.error) {
		sprintf(error, "An error occured when parsing %s:\n%s\n", path, result.error);
		result.error = error;
		return result;
	}
	return result;
}