from __future__ import annotations

import dataclasses
import json
import os
import pickle
import random
import signal
import sys
import time
from enum import Enum
from multiprocessing import Process
from typing import Dict, Tuple, List, Union, BinaryIO

import sysv_ipc

read_index = 0
write_index = 1
pipes: Dict[int, Tuple[BinaryIO, BinaryIO]] = {}

number_of_processes = 3
processes: List[Process] = []

database_max_access_count = 5

shared_memory_key = 125
shared_memory = sysv_ipc.SharedMemory(shared_memory_key, flags=sysv_ipc.IPC_CREX)

message_size = 128


@dataclasses.dataclass
class DatabaseEntry:
    user_id: int
    clock_value: int
    access_count: int


@dataclasses.dataclass
class Database:
    entries: Dict[int, DatabaseEntry]

    def __str__(self):
        return json.dumps(self.entries, indent=2, default=str)

    def get_entry(self, index) -> Union[DatabaseEntry, None]:
        if index not in self.entries:
            return None

        return self.entries[index]

    def set_entry(self, entry: DatabaseEntry):
        self.entries[entry.user_id] = entry

    def serialize(self) -> bytes:
        return pickle.dumps(self)

    @staticmethod
    def deserialize(database_bytes: bytes) -> Database:
        return pickle.loads(database_bytes)


class MessageType(Enum):
    Request = "Request"
    Response = "Response"


@dataclasses.dataclass
class Message:
    type: MessageType
    id: int
    clock_value: int

    def __str__(self):
        return f"Message Type: {self.type.value}, User ID: {self.id}, Clock Value: {self.clock_value}"

    def __repr__(self):
        return f"Message Type: {self.type.value}, User ID: {self.id}, Clock Value: {self.clock_value}"

    def serialize(self) -> bytes:
        return pickle.dumps(self).ljust(message_size, b"\x00")

    @staticmethod
    def deserialize(serialized_message: bytes) -> Message:
        return pickle.loads(serialized_message.lstrip(b"\x00"))


@dataclasses.dataclass
class DatabaseUser:
    id: int
    clock_value: int
    wants_to_enter_critical_section: bool = True

    def send_deferred_responses(self, deferred_responses: List[Message]):
        for req in deferred_responses:
            response = Message(MessageType.Response, self.id, req.clock_value)
            pipes[req.id][write_index].write(response.serialize())
            pipes[req.id][write_index].flush()
            sys.stdout.write(f"Sending Deferred Response: {response}\n")

    def enter_critical_section(self):
        sys.stdout.write(f"Starting Database Access for User with ID: {self.id}\n")
        database = Database.deserialize(shared_memory.read())
        sys.stdout.write(f"Database Before Update:\n{database}\n")

        old_entry = database.get_entry(self.id)
        new_entry = DatabaseEntry(self.id, self.clock_value, old_entry.access_count + 1)
        database.set_entry(new_entry)

        if new_entry.access_count == database_max_access_count:
            self.wants_to_enter_critical_section = False

        shared_memory.write(database.serialize())

        sys.stdout.write(f"Database After Update:\n{database}\n")
        sys.stdout.write(f"Finishing Database Access for User with ID: {self.id}\n")
        time.sleep(random.randint(100, 2000) / 1000)

    def wait_for_responses(self) -> List[Message]:
        response_count = 0
        deferred_responses: List[Message] = []

        max_received_message_clock_value = 0

        while True:
            msg = Message.deserialize(pipes[self.id][read_index].read(message_size))
            max_received_message_clock_value = max(max_received_message_clock_value, msg.clock_value)

            if msg.type == MessageType.Request:
                if (
                    not self.wants_to_enter_critical_section
                    or self.clock_value > msg.clock_value
                    or (self.clock_value == msg.clock_value and self.id > msg.id)
                ):
                    response = Message(MessageType.Response, self.id, msg.clock_value)
                    pipes[msg.id][write_index].write(response.serialize())
                    pipes[msg.id][write_index].flush()
                    sys.stdout.write(f"Sending Response: {msg}\n")
                else:
                    deferred_responses.append(msg)
                    sys.stdout.write(f"Deferring Response: {msg}\n")

            elif msg.type == MessageType.Response:
                response_count += 1
                sys.stdout.write(f"Saving Response: {msg}\n")

            if response_count == (number_of_processes - 1):
                new_clock_value = time.time_ns()
                if new_clock_value > max_received_message_clock_value:
                    self.clock_value = new_clock_value
                else:
                    self.clock_value = max_received_message_clock_value + 1

                break

        return deferred_responses

    def send_requests(self):
        if not self.wants_to_enter_critical_section:
            return

        request = Message(MessageType.Request, self.id, self.clock_value)

        for other_user_id, other_user_connections in pipes.items():
            if other_user_id == self.id:
                continue

            other_user_connections[write_index].write(request.serialize())
            other_user_connections[write_index].flush()

            sys.stdout.write(f"Sending Request: {request}\n")


def start_database_user(user_id: int):
    db_user = DatabaseUser(user_id, time.time_ns())

    while True:
        db_user.send_requests()
        deferred_responses = db_user.wait_for_responses()
        db_user.enter_critical_section()
        db_user.send_deferred_responses(deferred_responses)


def handle_exit(_sig, _frame):
    raise SystemExit


def main():
    database = Database({i: DatabaseEntry(i, 0, 0) for i in range(number_of_processes)})
    shared_memory.write(database.serialize())

    for i in range(number_of_processes):
        rd, wd = os.pipe()
        pipes[i] = (os.fdopen(rd, "rb"), os.fdopen(wd, "wb"))

    for i in range(number_of_processes):
        process = Process(target=start_database_user, args=(i,))
        processes.append(process)
        process.start()

    for process in processes:
        process.join()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)

    try:
        main()
    except (SystemExit, KeyboardInterrupt):
        shared_memory.remove()
        for r, w in pipes.values():
            r.close(), w.close()
