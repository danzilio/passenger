// This file is included inside the RequestHandler class.

private:

struct ev_loop *
getLoop() {
	return getContext()->libev->getLoop();
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
disconnectWithWarning(Client **client, const StaticString &message) {
	SKC_DEBUG(*client, "Disconnected client with warning: " << message);
	disconnect(client);
}

template<typename Number>
static Number clamp(Number value, Number min, Number max) {
	return std::max(std::min(value, max), min);
}
