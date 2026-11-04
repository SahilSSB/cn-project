# Network Time Synchronization using UDP Sockets over SDN
<br>

## Overview

The basic premise of the project is to sync clocks between clients using a central reference server.The network itself is built with Mininet and Open vSwitch, and its forwarding behaviour is managed by a __Ryu SDN controller__ speaking __OpenFlow 1.3__.

The core issue is that a timestamp is already out of date by the time it arrives, because the packet spent some time travelling through the network. The project measures the round-trip time, estimates the one-way delay from it, and compensates for it.





## Background Theory

### Why clocks need synchronization

Every computer keeps time with its own clock, and no two clocks run at exactly the same rate. Over time clocks drift apart. Distributed systems depend on a shared notion of time for things like log ordering, event correlation, authentication tokens, scheduling and transaction ordering, so clocks have to be periodically corrected against a trusted reference.

### The network delay problem

If a server simply sends its current time, the client receives a value that describes the moment the server sent it, not the moment the client received it. The gap is the network delay. Ignoring it would leave the client permanently behind by roughly one trip's worth of latency.

### Round-trip time and offset estimation

The client cannot measure one-way delay directly, because that would require the two clocks to already agree. It can, however, measure the **round-trip time (RTT)** using only its own clock:

- **T<sub>1</sub>** – client clock just before sending the request
- **T<sub>server</sub>** – timestamp the server places in its reply
- **T<sub>2</sub>** – client clock when the reply arrives <br><br>


```
RTT    = T_2 - T_1
offset = (T_server + RTT/2) - T_2
```

The offset is the amount the client must add to its own clock to match the server.

Dividing the RTT by two assumes the request and the reply each took the same time. If the path is symmetric, the server's timestamp was already **RTT/2** old when the reply arrived, so adding **RTT/2** corrects it.



### Why UDP

UDP suits time synchronization well:

- No handshake and no retransmission: latency stays predictable and the delay measurement is not distorted by protocol machinery.
- Symmetric timing is more realistic:  request and reply each take a single trip.
- Minimal overhead: the request is 1 byte and the reply is 8 bytes.
- No delivery guarantee: reliability has to be handled by the application. The client therefore uses a receive timeout and a bounded number of retries.

### Why SDN

Software-Defined Networking separates the data plane (the switch that forwards packets) from the control plane (the controller that decides how to forward them).

- The switch (Open vSwitch) forwards packets using a table of flow rules.
- When a packet matches no rule, the switch hands it to the controller.
- The controller decides what to do, installs a flow rule, and from then on the switch handles matching packets on its own.
- Different traffic classes can receive different rules. Here, time-sync traffic on UDP port 8000 gets its own higher-priority rule.

Communication between the switch and controller uses OpenFlow 1.3.

#### How the learning switch works

The controller acts as a learning switch:

1. A packet with no matching rule arrives and is sent to the controller.
2. The controller records which switch port the packet's source MAC address was seen on.
3. It looks up the destination MAC address. If it has been learned, the packet goes out that port. If not, the packet is flooded to all ports.
4. When the destination is known, the controller installs a flow rule so later packets bypass the controller entirely.

#### Flow rule priorities

When several rules could match a packet, the highest priority wins.

| Priority | Purpose |
|---|---|
| 0 | Table-miss: matches everything and sends it to the controller |
| 1 | Layer-2 forwarding: for ordinary known traffic |
| 10 | Time-sync traffic: specific to UDP port 8000 |


## System Architecture

```
                      +-------------------+
                      |   Ryu controller  |
                      |    control plane  |
                      +-------------------+
                                | 
                                | OpenFlow 1.3
                                |
+-------------------+ :         |
|   h2 · Client 1   |\ :        |
|     10.0.0.2      | \       +-------------------+                     +-------------------+
+-------------------+  \      |   Open vSwitch s1 |                     |  h1 · Time server |
                        ----->|     data plane    |-------------------->|     10.0.0.1      |
+-------------------+  /      +-------------------+    UDP port 8000    +-------------------+
|   h3 · Client 2   | /                                                
|     10.0.0.3      |/
+-------------------+
                          
```
Clients send a UDP request to the server and compute their clock offset from the reply. The switch forwards traffic using rules pushed by the controller.


## Protocol and Communication Flow

1. The client records T<sub>1</sub> and sends a 1-byte request to UDP port 8000.
2. The server reads its own clock and replies with it as an 8-byte big-endian timestamp in microseconds.
3. The client records T<sub>2</sub> when the reply arrives.
4. The client computes the RTT and offset, then applies the offset to its system clock.
5. If no reply arrives within 2 seconds, the client retries, up to 3 attempts in total. After that it exits with an error.

- Big-endian ("network byte order") is used so the timestamp is interpreted the same way regardless of the CPU architecture of either machine.


## Component Behaviour

- On switch connection: installs the priority-0 table-miss rule so unmatched packets reach the controller.
- On each packet-in: learns the source MAC to port mapping, looks up the destination, and floods if unknown.
- Known destination, UDP port 8000: installs a priority-10 rule matching the ingress port, MAC addresses and UDP destination port.
- Known destination, other traffic: installs a priority-1 layer-2 rule. 
- After a rule exists, the switch forwards matching packets without involving the controller.





