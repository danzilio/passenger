private:

void
checkoutSession(Client *client, Request *req) {
	GetCallback callback;
	Options &options = req->options;

	SKC_TRACE(client, 2, "Checking out session: appRoot=" << options.appRoot);
	req->state = Request::CHECKING_OUT_SESSION;
	req->beginScopeLog(&req->scopeLogs.getFromPool, "get from pool");

	callback.func = sessionCheckedOut;
	callback.userData = req;

	options.currentTime = (unsigned long long) (ev_now(getLoop()) * 1000000);

	refRequest(req);
	appPool->asyncGet(options, callback);
}

static void
sessionCheckedOut(const SessionPtr &session, const ExceptionPtr &e,
	void *userData)
{
	Request *req = static_cast<Request *>(userData);
	Client *client = static_cast<Client *>(req->client);
	RequestHandler *self = static_cast<RequestHandler *>(client->getServer());

	if (self->getContext()->libev->onEventLoopThread()) {
		self->sessionCheckedOutFromEventLoopThread(client, req, session, e);
		self->unrefRequest(req);
	} else {
		self->getContext()->libev->runLater(
			boost::bind(&RequestHandler::sessionCheckedOutFromAnotherThread,
				self, client, RequestRef(req), session, e));
		self->unrefRequest(req);
	}
}

void
sessionCheckedOutFromAnotherThread(Client *client, RequestRef req,
	SessionPtr session, ExceptionPtr e)
{
	sessionCheckedOutFromEventLoopThread(client, req.get(), session, e);
}

void
sessionCheckedOutFromEventLoopThread(Client *client, Request *req,
	const SessionPtr &session, const ExceptionPtr &e)
{
	if (e == NULL) {
		SKC_DEBUG(client, "Session checked out: pid=" << session->getPid() <<
			", gupid=" << session->getGupid());
		req->session = session;
		initiateSession(client, req);
	} else {
		client->endScopeLog(&client->scopeLogs.getFromPool, false);
		reportSessionCheckoutError(client, req, e);
	}
}

void
initiateSession(Client *client, Request *req, const SessionPtr &session) {

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

void
reportSessionCheckoutError(Client *client, Request *req, const ExceptionPtr &e) {
	{
		boost::shared_ptr<RequestQueueFullException> e2 =
			dynamic_pointer_cast<RequestQueueFullException>(e);
		if (e2 != NULL) {
			writeRequestQueueFullExceptionErrorResponse(client, req);
			return;
		}
	}
	{
		boost::shared_ptr<SpawnException> e2 = dynamic_pointer_cast<SpawnException>(e);
		if (e2 != NULL) {
			writeSpawnExceptionErrorResponse(client, e2);
			return;
		}
	}
	writeOtherExceptionErrorResponse(client, e);
}

void
writeRequestQueueFullExceptionErrorResponse(Client *client, Request *req) {
	const LString *value = req->secureHeaders.lookup("!~PASSENGER_REQUEST_QUEUE_OVERFLOW_STATUS_CODE");
	int requestQueueOverflowStatusCode = 503;
	if (value != NULL && value->size > 0) {
		value = psg_lstr_make_contiguous(value, req->pool);
		requestQueueOverflowStatusCode = stringToInt(
			StaticString(value->start->data, value->size));
	}
	writeResponse(client, req, requestQueueOverflowStatusCode,
		"<h1>This website is under heavy load</h1>"
		"<p>We're sorry, too many people are accessing this website at the same "
		"time. We're working on this problem. Please try again later.</p>");
	endRequest(&client, &req);
}

void
writeSpawnExceptionErrorResponse(Client *client, Request *req,
	const boost::shared_ptr<SpawnException> &e)
{
	RH_ERROR(client, "Cannot checkout session because a spawning error occurred. " <<
		"The identifier of the error is " << e->get("error_id") << ". Please see earlier logs for " <<
		"details about the error.");
	writeErrorResponse(client, e->getErrorPage(), e.get());
}

void
writeOtherExceptionErrorResponse(Client *client, Request *req, const ExceptionPtr &e) {
	string typeName;
	#ifdef CXX_ABI_API_AVAILABLE
		int status;
		char *tmp = abi::__cxa_demangle(typeid(*e).name(), 0, 0, &status);
		if (tmp != NULL) {
			typeName = tmp;
			free(tmp);
		} else {
			typeName = typeid(*e).name();
		}
	#else
		typeName = typeid(*e).name();
	#endif

	RH_WARN(client, "Cannot checkout session (exception type " <<
		typeName << "): " << e->what());

	string response = "An internal error occurred while trying to spawn the application.\n";
	response.append("Exception type: ");
	response.append(typeName);
	response.append("\nError message: ");
	response.append(e->what());
	boost::shared_ptr<tracable_exception> e3 = dynamic_pointer_cast<tracable_exception>(e);
	if (e3 != NULL) {
		response.append("\nBacktrace:\n");
		response.append(e3->backtrace());
	}

	writeErrorResponse(client, response);
}
