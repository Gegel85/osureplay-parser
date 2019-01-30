#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <setjmp.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <common.h>
#include "decompressor.h"
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
		sprintf(err_buff, "Unexpected end of file");
		longjmp(jump_buffer, true);
	}
	data = malloc(size + 1);
	if (!data) {
		sprintf(err_buff, "Memory allocation error (%luB)", (unsigned long)size);
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
			sprintf(err_buff, "Unexpected end of file");
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
			sprintf(err_buff, "Variable length int is longer than 4 bytes");
			longjmp(jump_buffer, true);
		} else if (*currentPos == buffSize) {
			sprintf(err_buff, "Unexpected end of file");
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
		sprintf(err_buff, "Unexpected end of file");
		longjmp(jump_buffer, true);
	}
	data = buffer[*currentPos];
	(*currentPos)++;
	return data;
}

char 	*OsuReplay_getString(const unsigned char *buffer, size_t buffSize, size_t *currentPos, char *err_buff, jmp_buf jump_buffer)
{
	unsigned char	byte = OsuReplay_getByte(buffer, buffSize, currentPos, err_buff, jump_buffer);
	unsigned int	length;

	if (!byte)
		return NULL;
	else if (byte != 11) {
		(*currentPos)--;
		sprintf(err_buff, "An invalid byte was found (0 or 11 expected but %i found)", byte);
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
			sprintf(err_buff, "Unexpected end of file");
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
			sprintf(err_buff, "Unexpected end of file");
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
		sprintf(err_buff, "Memory allocation error");
		longjmp(jump_buffer, true);
	}

	events.length = OsuReplay_getPointerArraySize((void **)eventArray);
	numberArray = malloc(events.length * sizeof(*numberArray));
	if (!numberArray) {
		sprintf(err_buff, "Memory allocation error (%luB)", (unsigned long)(events.length * sizeof(*numberArray)));
		longjmp(jump_buffer, true);
	}

	memset(numberArray, 0, events.length * sizeof(*numberArray));
	for (int i = 0; eventArray[i]; i++) {
		if (!eventArray[i + 1] && !strlen(eventArray[i])) {
			events.length--;
			break;
		}
		numberArray[i] = OsuReplay_splitString(eventArray[i], '|');
		if (OsuReplay_getPointerArraySize((void **)numberArray[i]) != 2) {
			sprintf(
				err_buff,
				"Lifebar events array is invalid. Expected 2 elements but %lu were given",
				(unsigned long)OsuReplay_getPointerArraySize((void **)numberArray[i])
			);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][0])) {
			sprintf(err_buff, "Invalid integer '%s'", numberArray[i][0]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][1])) {
			sprintf(err_buff, "Invalid floating number '%s'", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (atof(numberArray[i][1]) > 1 || atof(numberArray[i][1]) < 0) {
			sprintf(err_buff, "Invalid life value '%s' is not in range 0-1", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		}
	}

	events.content = malloc(events.length * sizeof(*events.content));
	if (!events.content) {
		sprintf(err_buff, "Memory allocation error (%luB)", (unsigned long)(events.length * sizeof(*events.content)));
		for (int i = 0; numberArray[i]; i++)
			free(numberArray[i]);
		free(numberArray);
		free(eventArray);
		longjmp(jump_buffer, true);
	}
	for (int i = 0; numberArray[i]; i++) {
		events.content[i].timeToHappen = atol(numberArray[i][0]);
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
	unsigned long		totalTime = 0;

	if (!eventArray) {
		sprintf(err_buff, "Memory allocation error");
		longjmp(jump_buffer, true);
	}

	events.length = OsuReplay_getPointerArraySize((void **)eventArray);
	numberArray = malloc(events.length * sizeof(*numberArray));
	if (!numberArray) {
		sprintf(err_buff, "Memory allocation error (%luB)", (unsigned long)(events.length * sizeof(*numberArray)));
		longjmp(jump_buffer, true);
	}

	memset(numberArray, 0, events.length * sizeof(*numberArray));
	for (int i = 0; eventArray[i] && eventArray[i + 1]; i++) {
		if (!eventArray[i + 1] && !strlen(eventArray[i])) {
			events.length--;
			break;
		}
		numberArray[i] = OsuReplay_splitString(eventArray[i], '|');
		if (OsuReplay_getPointerArraySize((void **)numberArray[i]) != 4) {
			sprintf(
				err_buff,
				"Game events array is invalid. Expected 4 elements but %lu were given",
				(unsigned long)OsuReplay_getPointerArraySize((void **)numberArray[i])
			);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][0])) {
			sprintf(err_buff, "Invalid integer '%s'", numberArray[i][0]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][1])) {
			sprintf(err_buff, "Invalid floating number '%s'", numberArray[i][1]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isFloat(numberArray[i][2])) {
			sprintf(err_buff, "Invalid floating number '%s'", numberArray[i][2]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		} else if (!OsuReplay_isInt(numberArray[i][3])) {
			sprintf(err_buff, "Invalid integer '%s'", numberArray[i][3]);
			for (int j = 0; j < i; i++)
				free(numberArray[i]);
			free(numberArray);
			free(eventArray);
			longjmp(jump_buffer, true);
		}
	}

	events.content = malloc(events.length * sizeof(*events.content));
	if (!events.content) {
		sprintf(err_buff, "Memory allocation error (%luB)", (unsigned long)(events.length * sizeof(*events.content)));
		for (int i = 0; numberArray[i]; i++)
			free(numberArray[i]);
		free(numberArray);
		longjmp(jump_buffer, true);
	}
	for (int i = 0; numberArray[i]; i++) {
		totalTime += atol(numberArray[i][0]);
		events.content[i].timeToHappen = totalTime;
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
	jmp_buf		jump_buffer;
	static	char	error[PATH_MAX + 1084];
	char		buffer[PATH_MAX + 1024];
	OsuString	compressedReplayData = {0, NULL};
	OsuString	uncompressedReplayData = {0, NULL};
	char		*lifeBar = NULL;
	struct DataStream stream;

	//Init the error handler
	if (setjmp(jump_buffer)) {
		strcpy(buffer, error);
		sprintf(error, "An error occurred when parsing string:\nCharacter %lu: %s", (unsigned long)currentPos, buffer);
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
		sprintf(error, "Invalid game mode found (%i)", result.mode);
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
		sprintf(error, "Invalid data remaining at the end of the file");
		longjmp(jump_buffer, true);
	}

	//Uncompress the replay data
	stream.outData = NULL;
	stream.outLen = 0;
	stream.inData = compressedReplayData.content;
	stream.inLen = compressedReplayData.length;
	switch (decompressStreamData(&stream)) {
	case ELZMA_E_BAD_PARAMS:
		sprintf(error, "LZMA: Bad parameter sent to an LZMA function");
		longjmp(jump_buffer, true);
	case ELZMA_E_ENCODING_PROPERTIES_ERROR:
		sprintf(error, "LZMA: Could not initialize the encode with configured parameters");
		longjmp(jump_buffer, true);
	case ELZMA_E_COMPRESS_ERROR:
		sprintf(error, "LZMA: An error occured during compression");
		longjmp(jump_buffer, true);
	case ELZMA_E_UNSUPPORTED_FORMAT:
		sprintf(error, "LZMA: Currently unsupported lzma file format was specified");
		longjmp(jump_buffer, true);
	case ELZMA_E_INPUT_ERROR:
		sprintf(error, "LZMA: An error occured when reading input");
		longjmp(jump_buffer, true);
	case ELZMA_E_OUTPUT_ERROR:
		sprintf(error, "LZMA: An error occured when writing output");
		longjmp(jump_buffer, true);
	case ELZMA_E_CORRUPT_HEADER:
		sprintf(error, "LZMA: LZMA header couldn't be parsed");
		longjmp(jump_buffer, true);
	case ELZMA_E_DECOMPRESS_ERROR:
		sprintf(error, "LZMA: An error occured during decompression");
		longjmp(jump_buffer, true);
	case ELZMA_E_INSUFFICIENT_INPUT:
		sprintf(error, "LZMA: The input stream returns EOF before the decompression could complete");
		longjmp(jump_buffer, true);
	case ELZMA_E_CRC32_MISMATCH:
		sprintf(error, "LZMA: Corrupted data");
		longjmp(jump_buffer, true);
	case ELZMA_E_SIZE_MISMATCH:
		sprintf(error, "LZMA: Size read mismatch expected length");
		longjmp(jump_buffer, true);
	}
	uncompressedReplayData.length = stream.outLen;
	uncompressedReplayData.content = stream.outData;

	//Parse uncompressed game events data and lifebar data
	result.lifeBar = OsuReplay_parseLifeBarEvents(lifeBar, error, jump_buffer);
	result.gameEvents = OsuReplay_parseGameEvents((char *)uncompressedReplayData.content, error, jump_buffer);
	for (int i = 0; i < result.gameEvents.length; i++)
		if (result.gameEvents.content[i].timeToHappen > (int)result.replayLength)
			result.replayLength = result.gameEvents.content[i].timeToHappen;

	free(uncompressedReplayData.content);
	free(compressedReplayData.content);
	free(lifeBar);

	if ((result.mode & MODE_HARD_ROCK) && (result.mode & MODE_EASY)) {
		sprintf(error, "Invalid modes found: HARD_ROCK and EASY can't be both active.");
		longjmp(jump_buffer, true);
	} else if ((result.mode & MODE_PERFECT) && (result.mode & MODE_NO_FAIL)) {
		sprintf(error, "Invalid modes found: SUDDEN_DEATH and NO_FAIL can't be both active.");
		longjmp(jump_buffer, true);
	} else if ((result.mode & MODE_SUDDEN_DEATH) && (result.mode & MODE_NO_FAIL)) {
		sprintf(error, "Invalid modes found: PERFECT and NO_FAIL can't be both active.");
		longjmp(jump_buffer, true);
	} else if ((result.mode & MODE_NIGHTCORE) && (result.mode & MODE_HALF_TIME)) {
		sprintf(error, "Invalid modes found: NIGHTCORE and HALF_TIME can't be both active.");
		longjmp(jump_buffer, true);
	} else if ((result.mode & MODE_DOUBLE_TIME) && (result.mode & MODE_HALF_TIME)) {
		sprintf(error, "Invalid modes found: DOUBLE_TIME and HALF_TIME can't be both active.");
		longjmp(jump_buffer, true);
	}
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
		sprintf(error, "%s: %s", path, strerror(errno));
		result.error = error;
		return result;
	}

	//Create buffer
	memset(&result, 0, sizeof(result));
	if (stat(path, &stats) < 0) {
		sprintf(error, "%s: %s", path, strerror(errno));
		result.error = error;
		return result;
	}

	size = stats.st_size;
	buffer = malloc(size);
	if (!buffer) {
		sprintf(error, "%s: Memory allocation error (%luB)", path, (long unsigned)size);
		result.error = error;
		return result;
	}

	//Read file
	fd = fileno(stream);
	readSize = read(fd, buffer, size);
	if (readSize < 0) {
		sprintf(error, "%s: %s", path, strerror(errno));
		result.error = error;
		return result;
	}
	fclose(stream);

	//Parse content
	result = OsuReplay_parseReplayString(buffer, readSize);
	if (result.error) {
		sprintf(error, "An error occured when parsing %s:\n%s", path, result.error);
		result.error = error;
		return result;
	}
	return result;
}

void	OsuReplay_destroy(OsuReplay *replay)
{
	free(replay->gameEvents.content);
	free(replay->lifeBar.content);
	free(replay->mapHash);
	free(replay->playerName);
	free(replay->replayHash);
}
