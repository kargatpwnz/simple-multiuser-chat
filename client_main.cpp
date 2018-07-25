#include "Client.h"

int main(void)
{
    Client client("127.0.0.1", 1500);
    client.Connect();
    return 0;
}