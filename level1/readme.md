Open the log file.
It looks for a file named requests.log.

Read the file in chunks.
Instead of loading a huge file all at once, it reads manageable pieces. This helps keep memory use down.

Look at each request.
It separates each line into five pieces: timestamp, method, endpoint, status, and latency.

Put requests into groups.
A group is based on the hour and endpoint. So requests to /api/items at 9 o’clock go together, while requests to /api/items at 10 o’clock go in a different group.

Count server errors.
Status codes from 500 to 599 count as errors. A status like 200 is not counted as an error.

Work out the latency percentiles.
Latency is how long a request took.
- p50 is the middle value: about half the requests were this fast or faster.
- p95 means about 95 out of 100 requests were this fast or faster.
- p99 gives a sense of how slow the slowest few requests were.
These numbers help show both typical performance and unusually slow requests.

Save the summary.
It writes the results to summary.txt, with one line per hour and endpoint.
