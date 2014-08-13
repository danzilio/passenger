/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2011-2014 Phusion
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
#ifndef _PASSENGER_REQUEST_HANDLER_CLIENT_H_
#define _PASSENGER_REQUEST_HANDLER_CLIENT_H_

#include <ev++.h>
#include <string>

#include <agents/HelperAgent/RequestHandler/Request.h>
#include <ServerKit/Client.h>
#include <Logging.h>

namespace Passenger {

using namespace std;
using namespace boost;
using namespace ApplicationPool2;


class Client: public ServerKit::Client {
private:
	/* static const char *boolStr(bool val) {
		static const char *strs[] = { "false", "true" };
		return strs[val];
	} */

public:
	Request req;
	ServerKit::HttpRequestParser reqParser;

	ev_tstamp connectedAt;

	DEFINE_SERVER_KIT_CLIENT_FOOTER(Client);


	Client(void *server)
		: ServerKit::Client(server),
		  reqParser(&req)
		{ }

	~Client() {
		deinitialize();
	}

	virtual void reinitialize(int fd) {
		ServerKit::Client::reinitialize(fd);
		req.reinitialize(fd);
		reqParser.reinitialize();
		connectedAt = ev_time();
	}

	virtual void deinitialize() {
		reqParser.deinitialize();
		req.deinitialize();
		ServerKit::Client::deinitialize();
	}

	string name() const {
		if (fdnum == -1) {
			return "(null)";
		} else {
			return toString(fdnum);
		}
	}


	template<typename Stream>
	void inspect(Stream &stream) const {
		const char *indent = "    ";
		time_t the_time;
		struct tm the_tm;
		char timestr[60];

		the_time = (time_t) connectedAt;
		localtime_r(&the_time, &the_tm);
		strftime(timestr, sizeof(timestr) - 1, "%F %H:%M:%S", &the_tm);

		//stream << indent << "host                        = " << (scgiParser.getHeader("HTTP_HOST").empty() ? "(empty)" : scgiParser.getHeader("HTTP_HOST")) << "\n";
		//stream << indent << "uri                         = " << (scgiParser.getHeader("REQUEST_URI").empty() ? "(empty)" : scgiParser.getHeader("REQUEST_URI")) << "\n";
		stream << indent << "connected at                = " << timestr << " (" << (unsigned long long) (ev_time() - connectedAt) << " sec ago)\n";
		/*
		stream << indent << "state                       = " << getStateName() << "\n";
		if (session == NULL) {
			stream << indent << "session                     = NULL\n";
		} else {
			stream << indent << "session pid                 = " << session->getPid() << " (" <<
				session->getGroup()->name << ")\n";
			stream << indent << "session gupid               = " << session->getGupid() << "\n";
			stream << indent << "session initiated           = " << boolStr(session->initiated()) << "\n";
		}

		stream
			<< indent << "requestBodyIsBuffered       = " << boolStr(requestBodyIsBuffered) << "\n"
			<< indent << "requestIsChunked            = " << boolStr(requestIsChunked) << "\n"
			<< indent << "requestBodyLength           = " << requestBodyLength << "\n"
			<< indent << "requestBodyAlreadyRead      = " << requestBodyAlreadyRead << "\n"
			<< indent << "responseContentLength       = " << responseContentLength << "\n"
			<< indent << "responseBodyAlreadyRead     = " << responseBodyAlreadyRead << "\n"
			<< indent << "clientBodyBuffer started    = " << boolStr(clientBodyBuffer->isStarted()) << "\n"
			<< indent << "clientBodyBuffer reachedEnd = " << boolStr(clientBodyBuffer->reachedEnd()) << "\n"
			<< indent << "clientOutputPipe started    = " << boolStr(clientOutputPipe->isStarted()) << "\n"
			<< indent << "clientOutputPipe reachedEnd = " << boolStr(clientOutputPipe->reachedEnd()) << "\n"
			<< indent << "clientOutputWatcher active  = " << boolStr(clientOutputWatcher.is_active()) << "\n"
			<< indent << "appInput                    = " << appInput.get() << " " << appInput->inspect() << "\n"
			<< indent << "appInput started            = " << boolStr(appInput->isStarted()) << "\n"
			<< indent << "appInput reachedEnd         = " << boolStr(appInput->endReached()) << "\n"
			<< indent << "responseHeaderSeen          = " << boolStr(responseHeaderSeen) << "\n"
			<< indent << "useUnionStation             = " << boolStr(useUnionStation()) << "\n"
			;
		*/
	}
};


} // namespace Passenger

#endif /* _PASSENGER_REQUEST_HANDLER_CLIENT_H_ */
