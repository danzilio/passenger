// This file is included inside the RequestHandler class.

private:

void
sendHeaderToApp(Client *client, Request *req) {
	SKC_TRACE(client, 2, "Sending headers to application");
	req->state = Request::SENDING_HEADER_TO_APP;
	if (req->session->getProtocol() == "session") {
		req->halfCloseAppConnection = true;
		sendHeaderToAppWithSessionProtocol(client, req);
	} else {
		req->halfCloseAppConnection = false;
		sendHeaderToAppWithHttpProtocol(client, req);
	}
}

void
sendHeaderToAppWithSessionProtocol(Client *client, Request *req) {
	unsigned int bufferSize = determineHeaderSizeForSessionProtocol(req);
	MemoryKit::mbuf_pool &mbuf_pool = getContext()->mbuf_pool;
	bool ok;

	if (bufferSize <= mbuf_pool.mbuf_block_chunk_size) {
		MemoryKit::mbuf buffer(MemoryKit::mbuf_get(&mbuf_pool));
		bufferSize = mbuf_pool.mbuf_block_chunk_size;

		ok = constructHeaderForSessionProtocol(req, buffer.start,
			bufferSize);
		assert(ok);
		buffer = MemoryKit::mbuf(buffer, 0, bufferSize);
		req->appInput.feed(boost::move(buffer));
	} else {
		char *buffer = (char *) psg_pnalloc(req->pool, bufferSize);

		ok = constructHeaderForSessionProtocol(req, buffer,
			bufferSize);
		assert(ok);
		req->appInput.feed(buffer, bufferSize);
	}

	(void) ok; // Shut up compiler warning

	if (!req->ended()) {
		if (!req->appInput.ended()) {
			if (!req->appInput.passedThreshold()) {
				sendBodyToApp(client, req);
			} else {
				req->appInput.setBuffersFlushedCallback(sendBodyToAppWhenBuffersFlushed);
			}
		} else {
			req->state = Request::WAITING_FOR_APP_OUTPUT;
		}
	}
}

static void
sendBodyToAppWhenBuffersFlushed(FileBufferedChannel *_channel) {
	FileBufferedFdOutputChannel *channel =
		reinterpret_cast<FileBufferedFdOutputChannel *>(_channel);
	Request *req = static_cast<Request *>(static_cast<
		ServerKit::BaseHttpRequest *>(channel->getHooks()->userData));
	Client *client = static_cast<Client *>(req->client);
	RequestHandler *self = static_cast<RequestHandler *>(
		getServerFromClient(client));

	req->appInput.setBuffersFlushedCallback(NULL);
	self->sendBodyToApp(client, req);
}

unsigned int
determineHeaderSizeForSessionProtocol(Request *req) {
	unsigned int dataSize = sizeof(boost::uint32_t);

	dataSize += sizeof("REQUEST_URI");
	dataSize += req->path.size + 1;

	dataSize += sizeof("PATH_INFO");
	dataSize += req->path.size + 1;

	dataSize += sizeof("SCRIPT_NAME");
	dataSize += sizeof("");

	dataSize += sizeof("SERVER_NAME");
	dataSize += serverName.size();

	dataSize += sizeof("PASSENGER_CONNECT_PASSWORD");
	dataSize += req->session->getGroupSecret().size() + 1;

	if (req->options.analytics) {
		dataSize += sizeof("PASSENGER_TXN_ID");
		dataSize += req->options.transaction->getTxnId().size() + 1;
	}

	ServerKit::HeaderTable::Iterator it(req->headers);
	while (*it != NULL) {
		dataSize += sizeof("HTTP_") - 1 + it->header->key.size + 1;
		dataSize += it->header->val.size + 1;
	}

	return dataSize + 1;
}

bool
constructHeaderForSessionProtocol(Request *req, char *buffer, unsigned int &size) {
	char *pos = buffer;
	const char *end = buffer + size;
	const LString::Part *part;

	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("REQUEST_URI"));
	part = req->path.start;
	while (part != NULL) {
		pos = appendData(pos, end, part->data, part->size);
		part = part->next;
	}

	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("PATH_INFO"));
	part = req->path.start;
	while (part != NULL) {
		pos = appendData(pos, end, part->data, part->size);
		part = part->next;
	}

	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("SCRIPT_NAME"));
	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL(""));

	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("SERVER_NAME"));
	pos = appendData(pos, end, serverName);

	pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("PASSENGER_CONNECT_PASSWORD"));
	pos = appendData(pos, end, req->session->getGroupSecret());
	pos = appendData(pos, end, "", 1);

	if (req->options.analytics) {
		pos = appendData(pos, end, P_STATIC_STRING_WITH_NULL("PASSENGER_TXN_ID"));
		pos = appendData(pos, end, req->options.transaction->getTxnId());
		pos = appendData(pos, end, "", 1);
	}

	ServerKit::HeaderTable::Iterator it(req->headers);
	while (*it != NULL) {
		if ((it->header->hash == HTTP_CONTENT_TYPE_HASH
			|| it->header->hash == HTTP_CONTENT_LENGTH_HASH)
		 && (psg_lstr_cmp(&it->header->key, P_STATIC_STRING("content-type"))
			|| psg_lstr_cmp(&it->header->key, P_STATIC_STRING("content-length"))))
		{
			continue;
		}

		pos = appendData(pos, end, P_STATIC_STRING("HTTP_"));
		const LString::Part *part = it->header->key.start;
		while (part != NULL) {
			char *start = pos;
			pos = appendData(pos, end, part->data, part->size);
			httpHeaderToScgiUpperCase((unsigned char *) start, pos - start);
			part = part->next;
		}
		pos = appendData(pos, end, "", 1);

		part = it->header->val.start;
		while (part != NULL) {
			pos = appendData(pos, end, part->data, part->size);
			part = part->next;
		}
		pos = appendData(pos, end, "", 1);

		it.next();
	}

	size = pos - buffer;
	return pos < end;
}

void
httpHeaderToScgiUpperCase(unsigned char *data, unsigned int size) {
	const unsigned char *end = data + size;

	while (data < end) {
		unsigned char ch = *data;
		if (ch >= 'a' && ch <= 'z') {
			*data = ch & 0x95;
		} else if (ch == '-') {
			*data = '_';
		}
		data++;
	}
}

/**
 * Construct an array of buffers, which together contain the 'session' protocol header
 * data that should be sent to the application. This method does not copy any data:
 * it just constructs buffers that point to the data stored inside `req->pool`,
 * `req->headers`, etc.
 *
 * The buffers will be stored in the array pointed to by `buffer`. This array must
 * have space for at least `maxbuffers` items. The actual number of buffers constructed
 * is stored in `nbuffers`, and the total data size of the buffers is stored in `dataSize`.
 * Upon success, returns true. If the actual number of buffers necessary exceeds
 * `maxbuffers`, then false is returned.
 *
 * You can also set `buffers` to NULL, in which case this method will not construct any
 * buffers, but only count the number of buffers necessary, as well as the total data size.
 * In this case, this method always returns true.
 */
/*
bool
constructHeaderBuffersForSessionProtocol(Request *req, struct iovec *buffers,
	unsigned int maxbuffers, restrict_ref unsigned int &nbuffers,
	restrict_ref boost::uint32_t &dataSize)
{
	#define INC_BUFFER_ITER(i) \
		do { \
			i++; \
			if (buffers != NULL && i == maxbuffers) { \
				return false; \
			} \
		} while (false)

	ServerKit::HeaderTable::Iterator it(req->headers);
	unsigned int i = 0;

	nbuffers = 0;
	dataSize = 0;

	if (buffers != NULL) {
		void *sizeField = psg_pnalloc(req->pool, sizeof(boost::uint32_t));
		buffers[i].iov_base = sizeField;
		buffers[i].iov_len  = sizeof(boost::uint32_t);
	}
	INC_BUFFER_ITER(i);
	dataSize += sizeof(boost::uint32_t);

	while (*it != NULL) {
		dataSize += it->header->key.size + 1;
		dataSize += it->header->val.size + 1;

		const LString::Part *part = it->header->key.start;
		while (part != NULL) {
			if (buffers != NULL) {
				buffers[i].iov_base = (void *) part->data;
				buffers[i].iov_len  = part->size;
			}
			INC_BUFFER_ITER(i);
			if (buffers != NULL) {
				buffers[i].iov_base = (void *) "";
				buffers[i].iov_len  = 1;
			}
			INC_BUFFER_ITER(i);
			part = part->next;
		}

		part = it->header->val.start;
		while (part != NULL) {
			if (buffers != NULL) {
				buffers[i].iov_base = (void *) part->data;
				buffers[i].iov_len  = part->size;
			}
			INC_BUFFER_ITER(i);
			if (buffers != NULL) {
				buffers[i].iov_base = (void *) "";
				buffers[i].iov_len  = 1;
			}
			INC_BUFFER_ITER(i);
			part = part->next;
		}

		it.next();
	}

	if (buffers != NULL) {
		buffers[i].iov_base = (void *) "PASSENGER_GROUP_SECRET";
		buffers[i].iov_len  = sizeof("PASSENGER_GROUP_SECRET");
	}
	INC_BUFFER_ITER(i);
	dataSize += sizeof("PASSENGER_GROUP_SECRET");

	if (buffers != NULL) {
		buffers[i].iov_base = (void *) req->session->getGroupSecret().data();
		buffers[i].iov_len  = req->session->getGroupSecret().size();
	}
	INC_BUFFER_ITER(i);
	if (buffers != NULL) {
		buffers[i].iov_base = (void *) "";
		buffers[i].iov_len  = 1;
	}
	INC_BUFFER_ITER(i);
	dataSize += req->session->getGroupSecret().size() + 1;

	if (req->options.analytics) {
		if (buffers != NULL) {
			buffers[i].iov_base = (void *) "PASSENGER_TXN_ID";
			buffers[i].iov_len  = sizeof("PASSENGER_TXN_ID");
		}
		INC_BUFFER_ITER(i);
		dataSize += sizeof("PASSENGER_TXN_ID");

		if (buffers != NULL) {
			buffers[i].iov_base = (void *) req->options.transaction->getTxnId().data();
			buffers[i].iov_len  = req->options.transaction->getTxnId().size();
		}
		INC_BUFFER_ITER(i);
		if (buffers != NULL) {
			buffers[i].iov_base = (void *) "";
			buffers[i].iov_len  = 1;
		}
		INC_BUFFER_ITER(i);
		dataSize += req->options.transaction->getTxnId().size() + 1;
	}

	if (buffers != NULL) {
		Uint32Message::generate((char *) buffers[0].iov_base,
			dataSize - sizeof(boost::uint32_t));
	}

	nbuffers = i;
	return true;

	#undef INC_BUFFER_ITER
}

bool
sendHeaderToAppWithSessionProtocolAndWritev(Request *req, ssize_t &bytesWritten) {
	unsigned int maxbuffers = std::min<unsigned int>(
		req->headers.size() * 4 + 8, IOV_MAX);
	struct iovec *buffers = (struct iovec *) psg_palloc(req->pool,
		sizeof(struct iovec) * maxbuffers);
	unsigned int nbuffers;
	boost::uint32_t dataSize;

	if (constructHeaderBuffersForSessionProtocol(req, buffers,
		maxbuffers, nbuffers, dataSize))
	{
		ssize_t ret;
		do {
			ret = writev(req->session->fd(), buffers, nbuffers);
		} while (ret == -1 && errno == EINTR);
		bytesWritten = ret;
		return ret == (ssize_t) dataSize;
	} else {
		bytesWritten = 0;
		return false;
	}
}

void
sendHeaderToAppWithSessionProtocolWithBuffering(Client *client, Request *req,
	unsigned int offset)
{
	struct iovec *buffers;
	unsigned int nbuffers;
	boost::uint32_t written, dataSize;
	bool ok;

	ok = constructHeaderBuffersForSessionProtocol(req, NULL, 0, nbuffers, dataSize);
	assert(ok);

	buffers = (struct iovec *) psg_palloc(req->pool,
		sizeof(struct iovec) * nbuffers);
	ok = constructHeaderBuffersForSessionProtocol(req, buffers, nbuffers,
		nbuffers, dataSize);
	assert(ok);
	(void) ok; // Shut up compiler warning

	char *buffer = (char *) psg_pnalloc(req->pool, dataSize);
	char *pos = buffer;
	const char *end = buffer + dataSize;

	for (unsigned int i = 0; i < nbuffers; i++) {
		assert(pos + buffers[i].iov_len <= end);
		memcpy(pos, buffers[i].iov_base, buffers[i].iov_len);
		pos += buffers[i].iov_len;
	}

	req->appInput.feed(buffer + offset, dataSize - offset);
	if (req->appInput.ended()) {

	} else if (req->appInput.passedThreshold()) {

	} else {
		sendBodyToApp(client, req);
	}
}
*/

void
sendHeaderToAppWithHttpProtocol(Client *client, Request *req) {
	// TODO
	endRequest(&client, &req);
}

void
sendBodyToApp(Client *client, Request *req) {
	if (req->hasRequestBody()) {
		// onRequestBody() will take care of forwarding
		// the request body to the app.
		req->state = Request::FORWARDING_BODY_TO_APP;
		req->requestBodyChannel.start();
	} else {
		// Our task is done. ForwardResponse.cpp will take
		// care of ending the request, once all response
		// data is forwarded.
		req->state = Request::WAITING_FOR_APP_OUTPUT;
	}
}


protected:

virtual Channel::Result
onRequestBody(Client *client, Request *req, const MemoryKit::mbuf &buffer,
	int errcode)
{
	assert(req->state == Request::FORWARDING_BODY_TO_APP);

	if (buffer.size() > 0) {
		// Data
		req->appInput.feed(buffer);
		if (!req->appInput.ended()) {
			if (req->appInput.passedThreshold()) {
				req->requestBodyChannel.stop();
				req->appInput.setBuffersFlushedCallback(resumeRequestBodyChannelWhenBuffersFlushed);
			}
			return Channel::Result(buffer.size(), false);
		} else {
			return Channel::Result(buffer.size(), true);
		}
	} else if (errcode == 0) {
		// EOF
		if (req->halfCloseAppConnection) {
			// TODO...
			::shutdown(req->session->fd(), SHUT_WR);
		}
		// Our task is done. ForwardResponse.cpp will take
		// care of ending the request, once all response
		// data is forwarded.
		req->state = Request::WAITING_FOR_APP_OUTPUT;
		return Channel::Result(0, true);
	} else {
		const unsigned int BUFSIZE = 1024;
		char *message = (char *) psg_pnalloc(req->pool, BUFSIZE);
		int size = snprintf(message, BUFSIZE,
			"error reading request body: %s (errno=%d)",
			strerror(errcode), errcode);
		disconnectWithError(&client, StaticString(message, size));
		return Channel::Result(0, true);
	}
}

static void
resumeRequestBodyChannelWhenBuffersFlushed(FileBufferedChannel *_channel) {
	FileBufferedFdOutputChannel *channel =
		reinterpret_cast<FileBufferedFdOutputChannel *>(_channel);
	Request *req = static_cast<Request *>(static_cast<
		ServerKit::BaseHttpRequest *>(channel->getHooks()->userData));

	assert(req->state == Request::FORWARDING_BODY_TO_APP);

	req->appInput.setBuffersFlushedCallback(NULL);
	req->requestBodyChannel.start();
}


private:

static void
onAppInputError(FileBufferedFdOutputChannel *channel, int errcode) {
	Request *req = static_cast<Request *>(static_cast<
		ServerKit::BaseHttpRequest *>(channel->getHooks()->userData));
	Client *client = static_cast<Client *>(req->client);
	RequestHandler *self = static_cast<RequestHandler *>(getServerFromClient(client));

	assert(req->state == Request::SENDING_HEADER_TO_APP
		|| req->state == Request::FORWARDING_BODY_TO_APP
		|| req->state == Request::WAITING_FOR_APP_OUTPUT);

	if (errcode == EPIPE) {
		// We consider an EPIPE non-fatal: we don't care whether the
		// app stopped reading, we just care about its output.
		SKC_DEBUG_FROM_STATIC(self, client, "cannot write to application socket: "
			"the application closed the socket prematurely");
	} else if (req->responded) {
		const unsigned int BUFSIZE = 1024;
		char *message = (char *) psg_pnalloc(req->pool, BUFSIZE);
		int size = snprintf(message, BUFSIZE,
			"cannot write to application socket: %s (errno=%d)",
			strerror(errcode), errcode);
		self->disconnectWithError(&client, StaticString(message, size));
	} else {
		self->endRequest(&client, &req);
	}
}
