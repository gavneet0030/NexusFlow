import json
import time
import uuid

from confluent_kafka import Consumer, Producer


BROKER = "localhost:9092"
TOPIC = "nexusflow.events"
GROUP = "nexusflow-e2e-%s" % uuid.uuid4().hex


producer = Producer(
    {
        "bootstrap.servers": BROKER,
        "acks": "all",
    }
)


consumer = Consumer(
    {
        "bootstrap.servers": BROKER,
        "group.id": GROUP,
        "auto.offset.reset": "latest",
        "enable.auto.commit": False,
    }
)


assignment_ready = False


def on_assign(consumer, partitions):
    global assignment_ready

    assignment_ready = True

    print("CONSUMER ASSIGNMENT: PASS")
    print("ASSIGNED PARTITIONS: %d" % len(partitions))


consumer.subscribe(
    [TOPIC],
    on_assign=on_assign
)


print("Waiting for consumer partition assignment...")


assignment_deadline = time.time() + 15


while not assignment_ready and time.time() < assignment_deadline:
    consumer.poll(1.0)


assert assignment_ready, "Consumer partition assignment failed"


events = []


for index in range(10):
    event = {
        "event_id": "nexusflow-e2e-%04d-%s" % (
            index,
            uuid.uuid4().hex[:8],
        ),
        "event_type": "synthetic",
        "timestamp_ms": int(time.time() * 1000),
        "priority": "HIGH" if index < 2 else "MEDIUM",
        "value": index * 10,
    }

    events.append(event)

    producer.produce(
        TOPIC,
        key=event["event_id"],
        value=json.dumps(event),
    )

    producer.poll(0)


remaining = producer.flush(15)


assert remaining == 0, (
    "Producer still has %d messages pending" % remaining
)


print("REAL REDPANDA PRODUCER: PASS")
print("EVENTS PRODUCED: %d" % len(events))


expected_ids = {
    event["event_id"]
    for event in events
}


received_ids = set()


deadline = time.time() + 20


while len(received_ids) < len(expected_ids) and time.time() < deadline:

    message = consumer.poll(1.0)

    if message is None:
        continue

    if message.error():
        print(
            "CONSUMER ERROR: %s"
            % str(message.error())
        )
        continue

    payload = json.loads(
        message.value().decode("utf-8")
    )

    event_id = payload.get("event_id")

    if event_id:
        received_ids.add(event_id)


consumer.close()


print("EVENTS RECEIVED: %d" % len(received_ids))


missing_ids = expected_ids - received_ids


assert not missing_ids, (
    "Missing events: %s"
    % sorted(missing_ids)
)


print("REAL REDPANDA CONSUMER: PASS")
print("END-TO-END STREAM: PASS")