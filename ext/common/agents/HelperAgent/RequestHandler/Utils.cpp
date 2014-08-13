// This file is included inside the RequestHandler class.

private:

void
disconnectWithError(Client **client, const StaticString &message) {
	Client *c = *client;
	RH_WARN(c, "Disconnecting with error: " << message);
	if (c->req.useUnionStation()) {
		c->req.logMessage("Disconnecting with error: " + message);
	}
	disconnect(client);
}

void
disconnectWithClientSocketWriteError(Client *client, int e) {
	stringstream message;
	message << "client socket write error: ";
	message << strerror(e);
	message << " (errno=" << e << ")";
	disconnectWithError(&client, message.str());
}

void
disconnectWithAppSocketWriteError(Client *client, int e) {
	stringstream message;
	message << "app socket write error: ";
	message << strerror(e);
	message << " (errno=" << e << ")";
	disconnectWithError(&client, message.str());
}

void
disconnectWithWarning(Client *client, const StaticString &message) {
	P_DEBUG("Disconnected client " << client->name() << " with warning: " << message);
	disconnect(&client);
}

void
resetClientStateForNextRequest(Client *client) {
	RH_TRACE(client, 2, "Resetting request state for next keep-alive request");
	client->req.deinitialize();
	client->req.reinitialize(client->getFd());
	client->input.start();
}

void
_writeSimpleRawResponse(Client *client, const StaticString &data) {
	const char *pos = data.data();
	const char *end = data.data() + data.size();

	while (pos < end && client->connected()) {
		size_t size = std::min<size_t>(end - pos, getContext()->mbuf_pool.mbuf_block_chunk_size);
		MemoryKit::mbuf buffer(MemoryKit::mbuf_get(&getContext()->mbuf_pool));
		buffer = MemoryKit::mbuf(buffer, 0, size);
		memcpy(buffer.start, pos, size);
		client->output.feed(buffer);
		pos += size;
	}
}

void
writeSimpleRawResponse(Client *client, const StaticString &data) {
	client->req.state = Request::HANDLE_NEXT_REQUEST_WHEN_OUTPUT_FLUSHED;
	client->input.stop();
	_writeSimpleRawResponse(client, data);
}

void
writeSimpleRawResponseAndDisconnect(Client *client, const StaticString &data) {
	client->req.state = Request::DISCONNECT_WHEN_OUTPUT_FLUSHED;
	_writeSimpleRawResponse(client, data);
}

template<typename Number>
static Number clamp(Number n, Number min, Number max) {
	if (n < min) {
		return min;
	} else if (n > max) {
		return max;
	} else {
		return n;
	}
}
