> Why does the client pass the identifier of its message queue in the body of the
> message (in the _clientId_ field), rather than in the message type (_mtype_)?

While that is technically possible (`clientId` being an `int` and `mtype` a `long`),
that would not be advisable for a number of reasons:

* If the server needs to implement priority request handling, using the message queue
  identifier as the message type makes that an impossibility.

* more refined communication between the client and the server becomes more difficult
  as well. The client could be interested in gathering stats about the server. Having
  a message type the server can listen to separately would make it easier to add
  such features.
