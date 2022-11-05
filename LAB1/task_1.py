import dataclasses
import os
import random
import time
from enum import Enum
from multiprocessing import Process
from typing import Set

from ipcqueue import sysvmq


class Ingredient(Enum):
    PAPER = "Paper"
    TOBACCO = "Tobacco"
    MATCHES = "Matches"

    def __str__(self):
        return self.value

    def __repr__(self):
        return self.value


@dataclasses.dataclass
class Smoker:
    has_ingredient: Ingredient
    needs_ingredients: Set[Ingredient]

    input_message_type: int
    output_message_type: int

    message_queue: sysvmq.Queue

    pid: int = os.getpid()

    def smoke(self):
        print(f"{os.getpid()} -> Has ingredient: {self.has_ingredient}")

        while True:
            message: Set[Ingredient] = self.message_queue.get(msg_type=self.input_message_type)

            if message == self.needs_ingredients:
                print(f"{os.getpid()} -> Received: {set_to_string(message)}")

                self._smoke()

                self.message_queue.put(set(), msg_type=self.output_message_type)

            else:
                print(f"{os.getpid()} -> Received: {set_to_string(message)}")
                print(f"{os.getpid()} -> Passing to next smoker")

                self.message_queue.put(message, msg_type=self.output_message_type)

    @staticmethod
    def _smoke():
        print(f"{os.getpid()} -> Lighting cigarette")
        time.sleep(1)
        print(f"{os.getpid()} -> Cigarette smoked")
        time.sleep(1)


@dataclasses.dataclass
class Salesman:
    input_message_type: int
    output_message_type: int

    message_queue: sysvmq.Queue

    pid: int = os.getpid()

    def sell(self):
        while True:
            self.message_queue.get(msg_type=self.input_message_type)
            time.sleep(2)

            all_ingredients = (Ingredient.PAPER, Ingredient.TOBACCO, Ingredient.MATCHES)
            random_ingredients = set(random.sample(all_ingredients, 2))

            print(f"\n{self.pid} -> Selling: {set_to_string(random_ingredients)}")
            self.message_queue.put(random_ingredients, msg_type=self.output_message_type)


def set_to_string(message: set[Ingredient, Ingredient]) -> str:
    if message:
        return f"'{', '.join(sorted([str(x) for x in message]))}'"

    return "''"


def main():
    message_queue_key = 125125
    sysvmq.Queue(message_queue_key).put({}, block=True, msg_type=4)

    smoker1 = Smoker(Ingredient.PAPER, {Ingredient.TOBACCO, Ingredient.MATCHES}, 1, 2, sysvmq.Queue(message_queue_key))
    smoker2 = Smoker(Ingredient.TOBACCO, {Ingredient.PAPER, Ingredient.MATCHES}, 2, 3, sysvmq.Queue(message_queue_key))
    smoker3 = Smoker(Ingredient.MATCHES, {Ingredient.PAPER, Ingredient.TOBACCO}, 3, 4, sysvmq.Queue(message_queue_key))
    salesman = Salesman(4, 1, sysvmq.Queue(message_queue_key))

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


if __name__ == "__main__":
    main()
