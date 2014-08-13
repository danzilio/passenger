// This file is included inside the RequestHandler class.
// It handles main events, and may forward events to
// respective state-specific handlers.

protected:

virtual int
onClientDataReceived(Client *client, const MemoryKit::mbuf &buffer, int errcode) {
	RH_LOG_EVENT(client, "onClientInputData");

	if (errcode != 0) {
		return onClientInputError(client, errcode);
	} else if (buffer.empty()) {
		return onClientEof(client);
	} else {
		return onClientRealData(client, buffer);
	}
}

private:

int
onClientRealData(Client *client, const MemoryKit::mbuf &buffer) {
	int consumed;

	RH_TRACE(client, 3, "Processing client data: \"" <<
		cEscapeString(StaticString(buffer.start, buffer.size())) <<
		"\"");
	switch (client->req.state) {
	case Request::PARSING_HEADERS:
		consumed = state_parsingHeaders_onClientData(client, buffer);
		break;
	/* case Request::BUFFERING_REQUEST_BODY:
		//consumed = state_bufferingRequestBody_onClientData(client, buffer);
		break;
	case Request::FORWARDING_BODY_TO_APP:
		//consumed = state_forwardingBodyToApp_onClientData(client, buffer);
		break; */
	default:
		abort();
	}

	RH_TRACE(client, 3, "Processed client data: consumed " << consumed << " bytes");
	return consumed;
}

int
onClientEof(Client *client) {
	RH_LOG_EVENT(client, "onClientEof; client sent EOF");
	switch (client->req.state) {
	/* case Request::BUFFERING_REQUEST_BODY:
		return state_bufferingRequestBody_onClientEof(client);
	case Request::FORWARDING_BODY_TO_APP:
		return state_forwardingBodyToApp_onClientEof(client); */
	case Request::HANDLE_NEXT_REQUEST_WHEN_OUTPUT_FLUSHED:
		// This should never happen because the input should
		// be stopped while we're in this state.
		P_BUG("EOF encountered in state HANDLE_NEXT_REQUEST_WHEN_OUTPUT_FLUSHED");
		return 0;
	case Request::DISCONNECT_WHEN_OUTPUT_FLUSHED:
		return 0;
	default:
		disconnect(&client);
		return 0;
	}
}

int
onClientInputError(Client *client, int errcode) {
	RH_LOG_EVENT(client, "onClientInputError");

	if (errcode == ECONNRESET) {
		// We might as well treat ECONNRESET like an EOF.
		// http://stackoverflow.com/questions/2974021/what-does-econnreset-mean-in-the-context-of-an-af-local-socket
		RH_TRACE(client, 3, "Client socket ECONNRESET error; treating it as EOF");
		return onClientEof(client);
	} else {
		stringstream message;
		message << "client socket read error: ";
		message << strerror(errcode);
		message << " (errno=" << errcode << ")";
		disconnectWithError(&client, message.str());
		return 0;
	}
}


static void
_onClientOutputFlushed(ServerKit::FileBufferedFdOutputChannel *channel) {
	Client *client = static_cast<Client *>(channel->getHooks()->userData);
	RequestHandler *self = static_cast<RequestHandler *>(client->getServer());
	self->onClientOutputFlushed(client, channel);
}

void
onClientOutputFlushed(Client *client, ServerKit::FileBufferedFdOutputChannel *channel) {
	RH_LOG_EVENT(client, "onClientOutputFlushed; done sending data to client");

	if (!client->connected()) {
		return;
	}

	switch (client->req.state) {
	case Request::DISCONNECT_WHEN_OUTPUT_FLUSHED:
		disconnect(&client);
		break;
	case Request::HANDLE_NEXT_REQUEST_WHEN_OUTPUT_FLUSHED:
		resetClientStateForNextRequest(client);
		break;
	default:
		break;
	}
}
