/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2011-2013 Phusion
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
#ifndef _PASSENGER_APPLICATION_POOL_SESSION_H_
#define _PASSENGER_APPLICATION_POOL_SESSION_H_

#include <sys/types.h>
#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <oxt/macros.hpp>
#include <oxt/system_calls.hpp>
#include <oxt/backtrace.hpp>
#include <ApplicationPool2/Common.h>
#include <ApplicationPool2/Socket.h>
#include <FileDescriptor.h>
#include <Utils/ScopeGuard.h>

namespace Passenger {
namespace ApplicationPool2 {

using namespace oxt;


/**
 * Represents a communication session with a process. A communication session
 * within Phusion Passenger is usually a single request + response but the API
 * allows arbitrary I/O. See Process's class overview for normal usage of Session.
 *
 * Not thread-safe, but Pool's and Process's API encourage that
 * a Session is only used by 1 thread and then thrown away.
 */
class Session {
public:
	typedef void (*Callback)(Session *session);

private:
	Process *process;
	Socket *socket;

	Connection connection;
	mutable boost::atomic<int> refcount;
	bool closed;

	void deinitiate(bool success) {
		connection.fail = !success;
		socket->checkinConnection(connection);
		connection.fd = -1;
	}

	void callOnInitiateFailure() {
		if (OXT_LIKELY(onInitiateFailure != NULL)) {
			onInitiateFailure(this);
		}
	}

	void callOnClose() {
		if (OXT_LIKELY(onClose != NULL)) {
			onClose(this);
		}
		closed = true;
	}

public:
	Callback onInitiateFailure;
	Callback onClose;

	Session(Process *_process, Socket *_socket)
		: process(_process),
		  socket(_socket),
		  refcount(1),
		  closed(false),
		  onInitiateFailure(NULL),
		  onClose(NULL)
		{ }

	~Session() {
		TRACE_POINT();
		// If user doesn't close() explicitly, we penalize performance.
		if (OXT_LIKELY(initiated())) {
			deinitiate(false);
		}
		if (OXT_LIKELY(!closed)) {
			callOnClose();
		}
	}

	const string &getConnectPassword() const;
	pid_t getPid() const;
	const string &getGupid() const;
	unsigned int getStickySessionId() const;
	Group *getGroup() const;
	void requestOOBW();
	int kill(int signo);
	void destroySelf() const;

	bool isClosed() const {
		return closed;
	}

	/**
	 * @pre !isClosed()
	 * @post result != NULL
	 */
	Process *getProcess() const {
		assert(!closed);
		return process;
	}

	Socket *getSocket() const {
		return socket;
	}

	const string &getProtocol() const {
		return socket->protocol;
	}

	void initiate() {
		assert(!closed);
		ScopeGuard g(boost::bind(&Session::callOnInitiateFailure, this));
		connection = socket->checkoutConnection();
		connection.fail = true;
		g.clear();
	}

	bool initiated() const {
		return connection.fd != -1;
	}

	int fd() const {
		return connection.fd;
	}

	/**
	 * This Session object becomes fully unsable after closing.
	 */
	void close(bool success) {
		if (OXT_LIKELY(initiated())) {
			deinitiate(success);
		}
		if (OXT_LIKELY(!closed)) {
			callOnClose();
		}
	}

	void ref() const {
		refcount.fetch_add(1, boost::memory_order_relaxed);
	}

	void unref() const {
		if (refcount.fetch_sub(1, boost::memory_order_release) == 1) {
			boost::atomic_thread_fence(boost::memory_order_acquire);
			destroySelf();
		}
	}
};


inline void
intrusive_ptr_add_ref(const Session *session) {
	session->ref();
}

inline void
intrusive_ptr_release(const Session *session) {
	session->unref();
}


} // namespace ApplicationPool2
} // namespace Passenger


namespace boost {
	inline void intrusive_ptr_add_ref(const Passenger::ApplicationPool2::Session *session) {
		session->ref();
	}

	inline void intrusive_ptr_release(const Passenger::ApplicationPool2::Session *session) {
		session->unref();
	}
}

#endif /* _PASSENGER_APPLICATION_POOL2_SESSION_H_ */
