# artuiter

This is a Twitter-like system made as the final project for the class of Operating Systems II in the second semester of 2020 (March–May 2021) at UFRGS.

The project was divided in two parts, of which only the first (TP1) was fully implemented. The implementation of the second part (TP2) was started but not finished. The following instructions to compile and run the program refer to thw first part (both the [`tp1`](https://github.com/arturwaquil/artuiter/releases/tag/tp1) and the [`tp1_enhanced`](https://github.com/arturwaquil/artuiter/releases/tag/tp1_enhanced) releases).

To compile the project, use the command `make`. The compilation will generate two binaries, `app_server` and `app_client`, in the main directory.

To execute the server, simply run `./app_server`. To execute the client, assuming the server is running, run `./app_client @<username> 127.0.0.1 4000` (standard IP address and port).