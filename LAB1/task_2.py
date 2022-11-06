import dataclasses
import random
import signal
import sys
import time
from enum import Enum
from multiprocessing import Process, Pipe, Manager
from multiprocessing.connection import Connection
from typing import Dict, Tuple, List

read_index = 0
write_index = 1

number_of_processes = 3
critical_section_max_entry_count = 5

manager = Manager()
db: Dict[int, List[int]] = manager.dict()

processes: List[Process] = []
pipes: Dict[int, Tuple[Connection, Connection]] = {}


class MessageType(Enum):
    Request = "Request"
    Response = "Response"

    def __str__(self):
        return self.value

    def __repr__(self):
        return self.value


@dataclasses.dataclass
class Message:
    type: MessageType
    id: int
    clock: int

    def __str__(self):
        return f"Message Type: {self.type}, User ID: {self.id}, Clock: {self.clock}"

    def __repr__(self):
        return f"Message Type: {self.type}, User ID: {self.id}, Clock: {self.clock}"


@dataclasses.dataclass
class DatabaseUser:
    id: int
    clock: int
    wants_to_enter_critical_section: bool = True

    def send_deferred_responses(self, deferred_responses: List[Message]):
        for req in deferred_responses:
            response = Message(MessageType.Response, self.id, req.clock)
            pipes[req.id][write_index].send(response)
            sys.stdout.write(f"Sending Deferred Response: {response}\n")

    def enter_critical_section(self):
        sys.stdout.write(f"Starting Database Access for User with ID: {self.id}\n")
        sys.stdout.write("Database Before Update:\n{}\n".format(db))

        db[self.id] = [self.id, self.clock, db[self.id][2] + 1]

        if db[self.id][2] == critical_section_max_entry_count:
            self.wants_to_enter_critical_section = False

        sys.stdout.write("Database After Update:\n{}\n".format(db))
        sys.stdout.write(f"Finishing Database Access for User with ID: {self.id}\n")
        time.sleep(random.randint(100, 2000) / 1000)

    def wait_for_responses(self) -> List[Message]:
        response_count = 0
        deferred_responses: List[Message] = []
        current_clock = self.clock

        while True:
            msg: Message = pipes[self.id][read_index].recv()
            self.clock += 1

            if msg.type == MessageType.Request:
                if not self.wants_to_enter_critical_section or current_clock > msg.clock \
                        or (current_clock == msg.clock and self.id > msg.id):
                    response = Message(MessageType.Response, self.id, msg.clock)
                    pipes[msg.id][write_index].send(response)
                    sys.stdout.write(f"Sending Response: {msg}\n")
                else:
                    deferred_responses.append(msg)
                    sys.stdout.write(f"Deferring Response: {msg}\n")

            elif msg.type == MessageType.Response:
                response_count += 1
                sys.stdout.write(f"Saving Response: {msg}\n")

            if response_count == (number_of_processes - 1):
                break

        return deferred_responses

    def send_requests(self):
        if not self.wants_to_enter_critical_section:
            return

        request = Message(MessageType.Request, self.id, self.clock)

        for other_user_id, other_user_connections in pipes.items():
            if other_user_id == self.id:
                continue

            conn = other_user_connections[write_index]
            conn.send(request)
            sys.stdout.write(f"Sending Request: {request}\n")


def start_database_user(user_id: int):
    db_user = DatabaseUser(user_id, random.randint(1, number_of_processes))

    while True:
        db_user.send_requests()
        deferred_responses = db_user.wait_for_responses()
        db_user.enter_critical_section()
        db_user.send_deferred_responses(deferred_responses)


def handle_exit(_sig, _frame):
    raise SystemExit


def main():
    for i in range(number_of_processes):
        read_conn, write_conn = Pipe()
        pipes[i] = (read_conn, write_conn)
        db[i] = [i, 0, 0]

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
        manager.shutdown()
