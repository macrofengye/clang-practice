/**
 * @date 2015-08-22
 * @author macro chen
 * @reference https://www.pacificsimplicity.ca/blog/libevent-echo-server-tutorial
 */
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define UNIX_SOCK_PATH "/tmp/mysocket"
static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
		struct sockaddr *address, int socklen, void *ctx) ;
static void accept_error_cb(struct evconnlistener *listener, void *ctx) ;
static void echo_read_cb(struct bufferevent *bev, void *ctx);
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx);

int main(int argc, char **argv) {
	struct event_base *base;
	struct evconnlistener *listener;
	struct sockaddr_un sin;

	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		return 1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sun_family = AF_LOCAL;
	strcpy(sin.sun_path , UNIX_SOCK_PATH);

	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
	LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (struct sockaddr *) &sin,
			sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);
	event_base_dispatch(base);
	return 0;
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
		struct sockaddr *address, int socklen, void *ctx) {
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(base, fd,
			BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an err %d (%s) on the listener . Shutting down .\n",
			err, evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}

static void echo_read_cb(struct bufferevent *bev, void *ctx) {
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	size_t len = evbuffer_get_length(input);
	char *data;
	data = malloc(len);
	evbuffer_copyout(input , data , len);
	printf("we got some data : %s\n", data);

	evbuffer_add_buffer(output, input);
	free(data);
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
	}
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
}
