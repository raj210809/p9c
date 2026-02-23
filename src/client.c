#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ixp.h>

int main() {
    printf("🔌 Connecting to 9P server...\n");

    /* Connect to server */
    IxpClient *client = ixp_mount("tcp!127.0.0.1!5640");
    if(client == NULL) {
        fprintf(stderr, "❌ Failed to connect\n");
        return 1;
    }

    printf("✅ Connected!\n");

    /* Walk to /exec */
    IxpCFid *fid = ixp_open(client, "/exec", P9_OWRITE);
    if(fid == NULL) {
        fprintf(stderr, "❌ Failed to open /exec\n");
        return 1;
    }

    printf("📤 Sending command to worker...\n");

    char *cmd = "uname -a";
    ixp_write(fid, cmd, strlen(cmd));

    printf("✅ Command sent!\n");

    ixp_close(fid);
    ixp_unmount(client);

    return 0;
}
