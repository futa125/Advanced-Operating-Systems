from __future__ import annotations

import dataclasses
import os
import pickle
import random
import signal
import sys
import time
from enum import Enum, auto
from multiprocessing import Process
from typing import Union

import sysv_ipc

message_queue_key = 125
message_queue = sysv_ipc.MessageQueue(message_queue_key, flags=sysv_ipc.IPC_CREX)


class Ingredient(Enum):
    PAPER = auto()
    TOBACCO = auto()
    MATCHES = auto()

    def __str__(self):
        return self.name.capitalize()


@dataclasses.dataclass
class IngredientPair:
    first: Ingredient
    second: Ingredient

    def __eq__(self, other):
        if not self or not other:
            return False

        return {self.first, self.second} == {other.first, other.second}

    def __str__(self):
        if self.first.value < self.second.value:
            return f"{self.first.name.capitalize()}, {self.second.name.capitalize()}"

        return f"{self.second.name.capitalize()}, {self.first.name.capitalize()}"


@dataclasses.dataclass
class Message:
    ingredients: Union[IngredientPair, None]
    type: int

    def __str__(self):
        return "Ingredients: {}, Type: {}".format(self.ingredients, self.type)

    def serialize(self) -> bytes:
        return pickle.dumps(self)

    @staticmethod
    def deserialize(message: bytes) -> Message:
        return pickle.loads(message)


@dataclasses.dataclass
class Smoker:
    has_ingredient: Ingredient
    needs_ingredients: IngredientPair

    input_message_type: int
    output_message_type: int

    message_queue: sysv_ipc.MessageQueue

    pid: int = 0

    def smoke(self):
        self.pid = os.getpid()
        sys.stdout.write(f"{self.pid} -> Has ingredient: '{self.has_ingredient}'\n")

        while True:
            _message, _type = self.message_queue.receive(type=self.input_message_type)
            message: Message = Message.deserialize(_message)

            if message.ingredients == self.needs_ingredients:
                sys.stdout.write(f"{self.pid} -> Received message: '{message}'\n")

                sys.stdout.write(f"{self.pid} -> Entering critical section\n")

                sys.stdout.write(f"{self.pid} -> Taking ingredients from the table: '{message.ingredients}'\n")

                sys.stdout.write(f"{self.pid} -> Exiting critical section\n")

                message = Message(None, self.output_message_type)

                self.message_queue.send(message.serialize(), type=self.output_message_type)

                sys.stdout.write(f"{self.pid} -> Sent message: '{message}'\n")

                proc = Process(target=self._smoke)
                proc.start()
                proc.join()

            else:
                sys.stdout.write(f"{self.pid} -> Received message: '{message}'\n")

                message.type = self.output_message_type
                self.message_queue.send(message.serialize(), type=self.output_message_type)

                sys.stdout.write(f"{self.pid} -> Sent message: '{message}'\n")

    @staticmethod
    def _smoke():
        sys.stdout.write(f"{os.getppid()} -> Started smoking\n")
        sys.stdout.write(f"{os.getppid()} -> Finished smoking\n")


@dataclasses.dataclass
class Salesman:
    input_message_type: int
    output_message_type: int

    message_queue: sysv_ipc.MessageQueue

    pid: int = 0

    def sell(self):
        self.pid = os.getppid()

        while True:
            _message, _type = self.message_queue.receive(type=self.input_message_type)
            time.sleep(1)

            message = Message.deserialize(_message)

            sys.stdout.write(f"\n{self.pid} -> Received message: '{message}'\n")

            random_ingredients = IngredientPair(*random.sample(list(Ingredient), 2))

            sys.stdout.write(f"{self.pid} -> Placing ingredients on the table: '{random_ingredients}'\n")

            message = Message(random_ingredients, self.output_message_type)
            self.message_queue.send(message.serialize(), type=self.output_message_type)

            sys.stdout.write(f"{self.pid} -> Sent message: '{message}'\n")


def main():
    message_queue.send(pickle.dumps(Message(None, 4)), type=4)

    smoker1 = Smoker(Ingredient.PAPER, IngredientPair(Ingredient.TOBACCO, Ingredient.MATCHES), 1, 2,
                     sysv_ipc.MessageQueue(message_queue_key))
    smoker2 = Smoker(Ingredient.TOBACCO, IngredientPair(Ingredient.PAPER, Ingredient.MATCHES), 2, 3,
                     sysv_ipc.MessageQueue(message_queue_key))
    smoker3 = Smoker(Ingredient.MATCHES, IngredientPair(Ingredient.PAPER, Ingredient.TOBACCO), 3, 4,
                     sysv_ipc.MessageQueue(message_queue_key))

    salesman = Salesman(4, 1, sysv_ipc.MessageQueue(message_queue_key))

    processes = []
    process = Process(target=smoker1.smoke)
    processes.append(process)
    process = Process(target=smoker2.smoke)
    processes.append(process)
    process = Process(target=smoker3.smoke)
    processes.append(process)
    process = Process(target=salesman.sell)
    processes.append(process)

    for process in processes:
        process.start()

    for process in processes:
        process.join()


def handle_exit(_sig, _frame):
    raise SystemExit


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)

    try:
        main()
    except (KeyboardInterrupt, SystemExit):
        message_queue.remove()
