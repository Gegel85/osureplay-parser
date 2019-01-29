//
// Created by Gegel85 on 29/01/2019.
//

#ifndef OSUREPLAYPARSER_DECOMPRESSOR_H
#define OSUREPLAYPARSER_DECOMPRESSOR_H

struct DataStream
{
	const unsigned char	*inData;
	size_t			inLen;

	unsigned char		*outData;
	size_t			outLen;
};

int		decompressStreamData(struct DataStream *_stream);

#endif //OSUREPLAYPARSER_DECOMPRESSOR_H
