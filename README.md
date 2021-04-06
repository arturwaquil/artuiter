# artuiter

This is a Twitter-like system made as the final project for the class of Operating Systems II in the second semester of 2020 (March–May 2021) at UFRGS.

To compile the project, use the command `make`. The compilation will generate two binaries, `app_server` and `app_client`, in the main directory.

To execute the server, simply run `./app_server`. To execute the client, assuming the server is running, run `./app_client @<username> 127.0.0.1 4000` (standard IP address and port).