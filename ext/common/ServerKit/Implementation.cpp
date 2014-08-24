/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2014 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#include <DataStructures/HashedStaticString.h>

namespace Passenger {
namespace ServerKit {


extern const HashedStaticString TRANSFER_ENCODING;
extern const char DEFAULT_INTERNAL_SERVER_ERROR_RESPONSE[];
extern const unsigned int DEFAULT_INTERNAL_SERVER_ERROR_RESPONSE_SIZE;

const HashedStaticString TRANSFER_ENCODING("transfer-encoding");
const char DEFAULT_INTERNAL_SERVER_ERROR_RESPONSE[] =
	"Status: 500 Internal Server Error\r\n"
	"Content-Length: 22\r\n"
	"Content-Type: text/plain\r\n"
	"Connection: close\r\n"
	"\r\n"
	"Internal server error\n";
const unsigned int DEFAULT_INTERNAL_SERVER_ERROR_RESPONSE_SIZE =
	sizeof(DEFAULT_INTERNAL_SERVER_ERROR_RESPONSE) - 1;

// The following functions are defined in this compilation unit so that they're
// compiled with optimizations on by default.

void
forceLowerCase(unsigned char *data, size_t len) {
	const unsigned char *end = data + len;
	while (data < end) {
		unsigned char c = *data;
		*data = ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c);
		data++;
	}
}

} // namespace ServerKit
} // namespace
