import requests
import sys
import time
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--wait-time", type=int, default=0, help="Time to wait between requests"
    )
    parser.add_argument(
        "--url", type=str, required=True, help="URL to send the request to"
    )
    parser.add_argument(
        "--count", type=int, default=0, help="Number of requests to send"
    )
    args = parser.parse_args()

    nrequests = 0
    nerrors = 0

    while True:
        if args.count > 0 and nrequests >= args.count:
            break

        try:
            response = requests.get(args.url)
        except Exception as e:
            nerrors += 1
            print(f"error: {e}", file=sys.stderr)
            continue

        print(f"requests: {nrequests} (errors: {nerrors})")
        nrequests += 1

        if args.wait_time > 0:
            time.sleep(args.wait_time)
