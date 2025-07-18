import requests
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

    counter = 0
    while True:
        if args.count > 0 and counter >= args.count:
            break

        response = requests.get(args.url)
        print(f"counter: {counter}")
        counter += 1

        if args.wait_time > 0:
            time.sleep(args.wait_time)
