// This file is included inside the RequestHandler class.
// It handles main events, and may forward events to
// respective state-specific handlers.

protected:

virtual void
onRequestBegin(Client *client, Request *req) {
	req->startedAt = ev_now(getLoop());

	//checkAndInternalizeRequestHeaders(&client);

	writeResponse(client,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 3\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"ok\n"
	);
	endRequest(&client, &req);
}
