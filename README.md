# Router

A simple forward engine and Routing Information Protocol(RIP).

### Environment

* Ubuntu 16.04
* quagga 0.99

### Forward Engine

* Maintain a route table
* Receive packages and forward them after looking up its route table
* You can add or delete route in its route table by sending information to port 800, or just use quagga-0.99.21-student.rar as your quagga.

### RIP

* can receive RIP packages from other service and deal with them according to RIP.