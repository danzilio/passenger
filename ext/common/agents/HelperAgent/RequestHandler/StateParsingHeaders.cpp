// This file is included inside the RequestHandler class.
// It handles the PARSING_HEADERS client state.

private:

int
state_parsingHeaders_onClientData(Client *client, const MemoryKit::mbuf &buffer) {
	Request &req = client->req;
	size_t ret = client->reqParser.feed(buffer);

	if (req.parseError != NULL) {
		RH_DEBUG(client, "Error parsing headers: " << client->req.parseError);
		disconnect(&client);
	} else if (req.headersComplete) {
		RH_DEBUG(client, "parsing headers done");
		/* if (benchmarkPoint == BP_AFTER_PARSING_HEADER) {
			writeSimpleResponse(client, "Benchmark point: after_parsing_header\n");
			return ret;
		} */

		checkAndInternalizeRequestHeaders(&client);
		if (client == NULL) {
			return ret;
		}

		if (req.shouldKeepAlive()) {
			writeSimpleRawResponse(client,
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 3\r\n"
				"Content-Type: text/plain\r\n"
				"Connection: keep-alive\r\n"
				"\r\n"
				"ok\n"
			);
		} else {
			writeSimpleRawResponseAndDisconnect(client,
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 3\r\n"
				"Content-Type: text/plain\r\n"
				"Connection: close\r\n"
				"\r\n"
				"ok\n"
			);
		}
	}
	return ret;
}

void
checkAndInternalizeRequestHeaders(Client **_client) {
	ServerKit::HttpRequest &req = (*_client)->req;

	//const LString *contentLength = req.headers.lookup("content-length");
	//P_WARN("content-length = " << StaticString(contentLength->start->data, contentLength->start->size));
/*
	// Check Content-Length and Transfer-Encoding.
	long long contentLength = getULongLongOption(client, "CONTENT_LENGTH");
	StaticString transferEncoding = parser.getHeader("HTTP_TRANSFER_ENCODING");
	if (contentLength != -1 && !transferEncoding.empty()) {
		reportBadRequestAndDisconnect(client, "Bad request (request may not contain both Content-Length and Transfer-Encoding)");
		return;
	}
	if (!transferEncoding.empty() && transferEncoding != "chunked") {
		reportBadRequestAndDisconnect(client, "Bad request (only Transfer-Encoding chunked is supported)");
		return;
	}
	// According to the HTTP/1.1 spec, Content-Length may not be 0.
	// We could reject the request, but some important HTTP clients are broken
	// (*cough* Ruby Net::HTTP *cough*) and fixing them is too much of
	// a pain, so we choose support it.
	if (contentLength == 0) {
		contentLength = -1;
		assert(transferEncoding.empty());
	}

	StaticString upgrade = parser.getHeader("HTTP_UPGRADE");
	const bool requestIsGetOrHead = requestMethod == "GET" || requestMethod == "HEAD";
	const bool requestBodyOffered = contentLength != -1 || !transferEncoding.empty();

	// Reject requests that have a request body and an Upgrade header.
	if (!requestIsGetOrHead && !upgrade.empty()) {
		reportBadRequestAndDisconnect(client, "Bad request (Upgrade header is only allowed for non-GET and non-HEAD requests)");
		return;
	}

	if (!requestBodyOffered) {
		if (upgrade.empty()) {
			client->requestBodyLength = 0;
		} else {
			client->requestBodyLength = -1;
		}
	} else {
		client->requestBodyLength = contentLength;
		client->requestIsChunked = !transferEncoding.empty();
	} */
}
