Introduction
This Python program analyzes a file of web request records. It reads a file named requests.log, groups requests by hour and endpoint, calculates latency statistics and server error counts, and writes the results to summary.txt.

The program is designed to handle large log files efficiently. It includes a fast processing method that can use multiple worker processes, along with a simpler backup method.
Input
Each line in requests.log is expected to contain five fields separated by spaces:
timestamp method endpoint status latency
For example:
2026-11-03T10:15:30.123Z GET /login 500 240
This example represents a request made at 10:15:30 to the /login endpoint. It used the GET method, returned status code 500, and had a latency of 240 microseconds.
How the Program Works
The program reads the request records and groups them by hour and endpoint. For example, requests to /login between 10:00 and 10:59 are included in the same group.

For each group, it calculates:
- Request count: the number of requests in the group.
- p50 latency: the middle latency value when the group’s latencies are ordered.
- p95 latency: a latency value near the slower end of the group.
- p99 latency: a latency value near the slowest end of the group.
- Error count: the number of requests with status codes from 500 to 599.
- Error rate: the number of errors per million requests.
These statistics help describe both the typical response time and the slower requests.
Program Structure
The clearest place to see the main analysis is reference_main. It reads the file one line at a time, groups the data, calculates the statistics, and writes the summary.

The _percentile function selects a latency value at a particular percentile position. Before calling this function, the program sorts the latency values.

The optimized version is organized around fast_main. It reads the file in chunks and can divide the work among multiple processes. Functions such as parse_chunk and ranks_of support this faster processing and percentile calculation.

The main function starts the fast method. If that method cannot handle the input or encounters a problem, the program tries a backup method.
Output
The program writes its results to summary.txt. Each output line represents one hour and one endpoint. It includes the request count, three latency percentiles, the error count, and the error rate.

By default, the program reads from /data/requests.log and writes to /out/summary.txt. The --data and --out options allow the input and output directories to be changed.
Efficiency and Limitations
The fast method is intended to process large files using chunks and worker processes. This can reduce processing time, but its parsing relies on assumptions about the input format. When the fast method cannot use those assumptions, the program falls back to a simpler reference method.

The input must follow the expected five-field layout for the analysis to work correctly. The latency field must also be a number because the program converts it to an integer.
Conclusion
This program turns raw web request logs into a compact summary of traffic, latency, and server errors. Its main purpose is to show how many requests reached each endpoint during each hour, how quickly they were handled, and how often they resulted in server errors. The optimized processing and fallback method allow it to handle large logs while retaining a simpler backup approach.
