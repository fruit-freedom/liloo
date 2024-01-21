import asyncio
import os
from typing import Callable, Any

from . import libliloo

class CompletitionQueue:
    def __init__(self) -> None:
        self.__futures: dict[int, asyncio.Future] = {}
        self.__loop: asyncio.AbstractEventLoop = None
        self.__completition_queue = libliloo.CompletitionQueue()

    def subscribe(self, loop: asyncio.AbstractEventLoop):
        if self.__loop is not None:
            raise RuntimeError('event loop already subscribed')

        events_fd = self.__completition_queue.get_event_fd()
        self.__loop = loop
        self.__loop.add_reader(events_fd, self.__event_callback, events_fd)

    def __event_callback(self, events_fd):
        message = os.read(events_fd, 8)
        print('[py] Recieve event', int.from_bytes(message, byteorder='little'))

        results = self.__completition_queue.get_completed_results()
        if not results:
            print('[py] WARNING get_completed_results() return no completed results')
            return

        for event_id, result in results:
            future = self.__futures.pop(event_id)
            if not future:
                raise RuntimeError(f'event {event_id} have not got a future')
            future.set_result(result)

    def __del__(self):
        if self.__loop:
            self.__loop.remove_reader(self.__completition_queue.get_event_fd())

    def futurize(self, target_func) -> Callable[[Any], asyncio.Future]:
        """
        Decorator for wrapping liloo async functions that returns `liloo::EventId`.
    
        # TODO: Handle methods that has not got `self` property
        
        Return `asyncio.Future` that resolves by event loop notification.
        """
        def fn(*args, **kwargs):
            saved_self = args[0]
            event_id = target_func(saved_self, self.__completition_queue, *args[1:], **kwargs)
            if event_id in self.__futures:
                raise RuntimeError(f'event {event_id} already registered')
            future = asyncio.Future()
            self.__futures[event_id] = future
            return future

        return fn

global_cq_ = CompletitionQueue()

def futurize(method):
    return global_cq_.futurize(method)
