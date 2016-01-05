# speedtest
A TCP utility to test the available bandwidth of the network.

This utility consists of two binaries server and client. The server can run on any machine, the client shoule know the ip address
or the domain name of the server to connect and start the test. The tests will check both the uplink and downlink bandwidth for
the client.

## Compiling
Go to the root directory of the project and run:
```
make
```
This will compile the two binaries server and client in the same directory. No special libraries are required.

## Running
The server can be run from the command line without arguments as:
```
./server
```
The default port for the server is 9000. The port can be changed by using the -p switch
```
./server -p <port no. between 1024 and 65536>
```

The client will be run as:
```
/client --server-ip <server ip or domain name> --server-port <port no>
```
## Througput calculation
The throughput is calculated on receiver side (so for uplink it is calculated by server and sent using a control message
to the client, for downling client will calculate and display the data).

## Working
A control channel exists between the client and server on the same TCP link.
When the client starts and establishes a TCP connection it will send commands to the server on the contril channel 
to run UPLINK AND DOWNLINK tests.

## Logs
We get some logs in the server and client console, the Uplink and Downling stats are quite apparent. Following is the snippet of
logs on the client.
```
192.168.1.137
Send Control Message: O
Received Control Message: A
Server is Ready for Download Test...
Running RX Test

Downlink Summary Stats:
---------------------------------------------
packets received: 1693 
bytes_received: 1024000 
average packets per second: 575.067935 
average Mega Bytes per second: 0.331713 (2.653702 Mbps)
duration (ms): 2944.000000 
---------------------------------------------
Send Control Message: U
Received Control Message: A
Server is Ready for Upload Test...
Running TX Test
Waiting for Summary Stats
Received Control Message: S

Uplink Summary Stats:
---------------------------------------------
packets received: 1254 
bytes_received: 1024000 
average packets per second: 1947.204969 
average Mega Bytes per second: 1.516401 (12.131211 Mbps)
duration (ms): 644.000000 
----------------------------------------------
```



