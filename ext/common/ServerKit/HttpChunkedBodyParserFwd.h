/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2012-2014 Phusion
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
#ifndef _PASSENGER_SERVER_KIT_CHUNKED_BODY_PARSER_FWD_H_
#define _PASSENGER_SERVER_KIT_CHUNKED_BODY_PARSER_FWD_H_

#include <boost/cstdint.hpp>

namespace Passenger {
namespace ServerKit {

using namespace std;


class BaseHttpRequest;

/**
 * Parses data in HTTP/1.1 chunked transfer encoding.
 *
 * This is a POD struct so that we can put it in a union inside BaseHttpRequest.
 */
struct HttpChunkedBodyParser {
	/***** Types and constants *****/

	enum Error {
		CHUNK_SIZE_STRING_TOO_LARGE = -1000,
		CHUNK_SIZE_PARSE_ERROR      = -1001,
		CHUNK_FOOTER_PARSE_ERROR    = -1002,
		CHUNK_FINALIZER_PARSE_ERROR = -1003,
		UNEXPECTED_EOF              = -1004
	};

	// (2^32-1)/10 (409 MB), because `remainingDataSize` is 32-bit. Divided by 10 to
	// prevent overflow during parsing of the chunk size.
	static const unsigned int MAX_CHUNK_SIZE = 429496729;
	static const char CR = '\x0D';
	static const char LF = '\x0A';

	enum State {
		EXPECTING_SIZE_FIRST_DIGIT,
		EXPECTING_SIZE,
		EXPECTING_CHUNK_EXTENSION,
		EXPECTING_HEADER_LF,
		EXPECTING_DATA,
		EXPECTING_NON_FINAL_CR,
		EXPECTING_NON_FINAL_LF,
		EXPECTING_FINAL_CR,
		EXPECTING_FINAL_LF,
		DONE,
		ERROR
	};


	/***** Working state *****/

	BaseHttpRequest *req;
	boost::uint32_t remainingDataSize;
	State state;
};


} // namespace ServerKit
} // namespace Passenger

#endif /* _PASSENGER_SERVER_KIT_CHUNKED_BODY_PARSER_FWD_H_ */
