#include <windows.h>
#include <time.h>
#include <iostream>
using namespace std;

#include <net_fdm.hxx>

double htond (double x)	
{
    int * p = (int*)&x;
    int tmp = p[0];
    p[0] = htonl(p[1]);
    p[1] = htonl(tmp);

    return x;
}

float htonf (float x)	
{
    int * p = (int *)&x;
    *p = htonl(*p);
    return x;
}

SOCKET sendSocket = -1;
struct sockaddr_in sendAddr;

// IP and port where FG is listening
char * fg_ip = "127.0.0.1";
int fg_port = 5500;

// update period.  controls how often updates are
// sent to FG.  in seconds.
int update_period = 1000;

void run();

int main(int argc, char ** argv)
{
    WSAData wd;
    if (WSAStartup(MAKEWORD(2,0),&wd) == 0)
    {
        memset(&sendAddr,0,sizeof(sendAddr));
        sendAddr.sin_family = AF_INET;
        sendAddr.sin_port = htons(fg_port);
        sendAddr.sin_addr.S_un.S_addr = inet_addr(fg_ip);

        sendSocket = socket(AF_INET,SOCK_DGRAM,0);
        if (sendSocket != INVALID_SOCKET)
        {
            run();
        }
        else
        {
            cout << "socket() failed" << endl;
        }
    }
    else
    {
        cout << "WSAStartup() failed" << endl;
    }

    return 0;
}

#define D2R (3.14159 / 180.0)

void run()
{
    double latitude = 45.59823; // degs
    double longitude = -120.69202; // degs
    double altitude = 150.0; // meters above sea level
 
    float roll = 0.0; // degs
    float pitch = 0.0; // degs
    float yaw = 0.0; // degs

    float visibility = 5000.0; // meters

    while (true)
    {
        Sleep(update_period);

        FGNetFDM fdm;
        memset(&fdm,0,sizeof(fdm));
        fdm.version = htonl(FG_NET_FDM_VERSION);

        fdm.latitude = htond(latitude * D2R);
        fdm.longitude = htond(longitude * D2R);
        fdm.altitude = htond(altitude);

        fdm.phi = htonf(roll * D2R);
        fdm.theta = htonf(pitch * D2R);
        fdm.psi = htonf(yaw * D2R);

        fdm.num_engines = htonl(1);

        fdm.num_tanks = htonl(1);
        fdm.fuel_quantity[0] = htonf(100.0);

        fdm.num_wheels = htonl(3);

        fdm.cur_time = htonl(time(0));
        fdm.warp = htonl(1);

        fdm.visibility = htonf(visibility);

        sendto(sendSocket,(char *)&fdm,sizeof(fdm),0,(struct sockaddr *)&sendAddr,sizeof(sendAddr));

        static bool flag = true;
        if (flag)
        {
            roll += 5.0;
        }
        else
        {            
            roll -= 5.0;
        }
        flag = !flag;
    }
}






















