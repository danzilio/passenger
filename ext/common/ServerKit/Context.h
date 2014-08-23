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
#ifndef _PASSENGER_SERVER_KIT_CONTEXT_H_
#define _PASSENGER_SERVER_KIT_CONTEXT_H_

#include <boost/make_shared.hpp>
#include <stddef.h>
#include <MemoryKit/mbuf.h>
#include <SafeLibev.h>
#include <Constants.h>

namespace Passenger {
namespace ServerKit {


class Context {
private:
	void initialize() {
		mbuf_pool.mbuf_block_chunk_size = DEFAULT_MBUF_CHUNK_SIZE;
		MemoryKit::mbuf_pool_init(&mbuf_pool);
	}

public:
	SafeLibevPtr libev;
	struct MemoryKit::mbuf_pool mbuf_pool;
	string secureModePassword;

	Context(const SafeLibevPtr &_libev)
		: libev(_libev)
	{
		initialize();
	}

	Context(struct ev_loop *loop)
		: libev(make_shared<SafeLibev>(loop))
	{
		initialize();
	}

	~Context() {
		MemoryKit::mbuf_pool_deinit(&mbuf_pool);
	}
};

class HooksImpl;

struct Hooks {
	HooksImpl *impl;
	void *userData;
};

class HooksImpl {
public:
	virtual ~HooksImpl() { }

	virtual bool hook_isConnected(Hooks *hooks, void *source) {
		return true;
	}

	virtual void hook_ref(Hooks *hooks, void *source) { }
	virtual void hook_unref(Hooks *hooks, void *source) { }
};

struct RefGuard {
	Hooks *hooks;
	void *source;

	RefGuard(Hooks *_hooks, void *_source)
		: hooks(_hooks),
		  source(_source)
	{
		if (_hooks != NULL && _hooks->impl != NULL) {
			_hooks->impl->hook_ref(_hooks, _source);
		}
	}

	~RefGuard() {
		if (hooks != NULL && hooks->impl != NULL) {
			hooks->impl->hook_unref(hooks, source);
		}
	}
};


} // namespace ServerKit
} // namespace Passenger

#endif /* _PASSENGER_SERVER_KIT_CONTEXT_H_ */
