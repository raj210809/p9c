#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ixp.h>

Ixp9Srv p9srv;
IxpServer server;

/* QIDs identify files in 9P */
enum {
    QROOT = 0,
    QEXEC = 1,
};

/* ============================= */
/* 9P FILESYSTEM HANDLERS        */
/* ============================= */

void fs_attach(Ixp9Req *r) {
    printf("📡 Client attached\n");

    r->fid->qid.type = P9_QTDIR;
    r->fid->qid.path = QROOT;

    ixp_respond(r, NULL);
}

/* WALK is where we define our filesystem tree */
void fs_walk(Ixp9Req *r) {

    /* If client is walking to root (clone) */
    if (r->ifcall.twalk.nwname == 0) {
        ixp_respond(r, NULL);
        return;
    }

    char *name = r->ifcall.twalk.wname[0];

    if (strcmp(name, "exec") == 0) {
        printf("📂 Client walked to /exec\n");

        r->fid->qid.type = 0;      // regular file
        r->fid->qid.path = QEXEC;

        /* Must return number of QIDs walked */
        r->ofcall.rwalk.nwqid = 1;
        r->ofcall.rwalk.wqid[0] = r->fid->qid;

        ixp_respond(r, NULL);
        return;
    }

    ixp_respond(r, "file not found");
}


/* OPEN is called before read/write */
void fs_open(Ixp9Req *r) {
    if (r->fid->qid.path != QEXEC) {
        ixp_respond(r, "cannot open");
        return;
    }

    printf("📄 /exec opened\n");

    r->ofcall.ropen.qid = r->fid->qid;
    ixp_respond(r, NULL);
}

/* WRITE is where we run commands 😄 */
void fs_write(Ixp9Req *r) {
    if (r->fid->qid.path != QEXEC) {
        ixp_respond(r, "cannot write");
        return;
    }

    char cmd[256];
    memcpy(cmd, r->ifcall.twrite.data, r->ifcall.twrite.count);
    cmd[r->ifcall.twrite.count] = 0;

    printf("🧠 Command received: %s\n", cmd);

    int ret = system(cmd);

    r->ofcall.rwrite.count = r->ifcall.twrite.count;
    ixp_respond(r, NULL);
}


/* ============================= */
/* CONNECTION CALLBACKS          */
/* ============================= */

void conn_read(IxpConn *c) {
    IxpMsg msg;
    int n = ixp_recvmsg(c->fd, &msg);
    if (n <= 0) {
        printf("⚠️ Error reading from client\n");
        return;
    }

    printf("📨 Received raw message (%d bytes)\n", n);
}

void conn_close(IxpConn *c) {
    
    printf("🔌 Client disconnected\n");
}

/* ============================= */
/* MAIN                          */
/* ============================= */

int main() {
    printf("🚀 9P EXEC WORKER STARTING (port 5640)\n");

    /* Register filesystem operations */
    p9srv.attach = fs_attach;
    p9srv.walk   = fs_walk;
    p9srv.open   = fs_open;
    p9srv.write  = fs_write;

    /* Create TCP socket */
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(5640),
        .sin_addr   = { INADDR_ANY }
    };

    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(fd, 5);

    printf("🌐 Listening for 9P connections...\n");

    IxpConn* server_conn = ixp_listen(&server, fd, &p9srv, conn_read, conn_close);

    ixp_serve9conn(server_conn);
    ixp_serverloop(&server);

    return 0;
}
