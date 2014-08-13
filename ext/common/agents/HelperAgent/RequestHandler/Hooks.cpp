// This file is included inside the RequestHandler class.
// It implements the most important ServerKit::Server hooks.

protected:

virtual void onClientObjectCreated(Client *client) {
	ParentClass::onClientObjectCreated(client);
	Request &req = client->req;

	//client->output.errorCallback = ...;
	client->output.setFlushedCallback(_onClientOutputFlushed);

	req.clientBodyBuffer.setContext(getContext());
	req.clientBodyBuffer.setHooks(&client->hooks);
	req.appInput.setContext(getContext());
	req.appInput.setHooks(&client->hooks);
	req.appOutput.setContext(getContext());
	req.appOutput.setHooks(&client->hooks);
	//req.appOutput.errorCallback = ...;
	//req.appOutput.setFlushedCallback();

	//req.responseDechunker.onData   = onAppInputChunk;
	//req.responseDechunker.onEnd    = onAppInputChunkEnd;
	//req.responseDechunker.userData = this;
}
