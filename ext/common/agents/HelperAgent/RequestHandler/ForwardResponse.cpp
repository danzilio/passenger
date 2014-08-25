// This file is included inside the RequestHandler class.

private:

static Channel::Result
onAppOutputData(Channel *_channel, const MemoryKit::mbuf &buffer, int errcode) {
	FdInputChannel *channel = reinterpret_cast<FdInputChannel *>(_channel);
	Request *req = static_cast<Request *>(static_cast<
		ServerKit::BaseHttpRequest *>(channel->getHooks()->userData));
	Client *client = static_cast<Client *>(req->client);
	RequestHandler *self = static_cast<RequestHandler *>(getServerFromClient(client));

	if (buffer.size() > 0) {
		// Data
		self->writeResponse(client, buffer);
		return Channel::Result(buffer.size(), false);
	} else if (errcode != 0 || errcode == ECONNRESET) {
		// EOF
		self->endRequest(&client, &req);
		return Channel::Result(0, true);
	} else {
		// Error
		const unsigned int BUFSIZE = 1024;
		char *message = (char *) psg_pnalloc(req->pool, BUFSIZE);
		int size = snprintf(message, BUFSIZE,
			"cannot read from application socket: %s (errno=%d)",
			strerror(errcode), errcode);
		self->disconnectWithError(&client, StaticString(message, size));
		return Channel::Result(0, true);
	}
}
