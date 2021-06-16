# On start

- connect to primary
    - connect to any other server
    - ask who's primary
    - reconnect if needed
    - (if could not connect to anyone, it is the primary)
- receive info from primary
- if one's id > primary id, start election


# On election

(if detected that primary server is down)
- connect to every server...
- send message `ELECTION` to servers with bigger id
- wait for message `ANSWER` for a timeout T
    - if not received, send message `COORDINATOR` to all servers
    - if received, wait for message `COORDINATOR` for a timeout T'
        - if not received, restart election

(if message `ELECTION` received)
- send message `ANSWER` to the sender
- start own election if hasn't already


# PRIMARY SERVER

- wait for client connections and create threads to deal with them
- update backup servers (what infos??)


# BACKUP SERVER

- listen to and register the passed info from primary server
- wait for client connections and reply the primary server's id
- detect that the primary server is down (through a timeout??) and start election
