import asyncio
import build.asyncie as ie
import os
import numpy as np



class EventLoop:
    def __init__(self) -> None:
        self.futures: dict[int, asyncio.Future] = {}
        self.loop: asyncio.AbstractEventLoop = None

    def subscribe(self, loop: asyncio.AbstractEventLoop):
        if self.loop is not None:
            raise RuntimeError('event loop already subscribed')

        self.events_fd = ie._get_event_fd()
        self.loop = loop
        self.loop.add_reader(self.events_fd, self._event_callback, self.events_fd)

    def _event_callback(self, events_fd):
        message = os.read(events_fd, 8)
        print('[py] Recieve event', int.from_bytes(message, byteorder='little'))

        results = ie._get_completed_results()
        if not results:
            print('[py] WARNING get_completed_results() return no completed results')
            return

        for event_id, result in results:
            future = self.futures.pop(event_id)
            if not future:
                raise RuntimeError(f'event {event_id} have not got a future')
            future.set_result(result)

    def __del__(self):
        if self.loop:
            self.loop.remove_reader(self.events_fd)

    def futurise(self, target_func) -> asyncio.Future:
        """
        Decorator for wrapping liloo async functions that returns `liloo::EventId`.
        
        Return `asyncio.Future` that resolves by event loop notification.
        """
        def fn(*args, **kwargs):
            event_id = target_func(*args, **kwargs)
            if event_id in self.futures:
                raise RuntimeError(f'event {event_id} already registered')
            future = asyncio.Future()
            self.futures[event_id] = future
            return future

        return fn



cq = EventLoop()


class Model:
    def __init__(self, *args, **kwargs) -> None:
        self._impl = ie.Model(*args, **kwargs)

    @cq.futurise
    def forward(self, *args, **kwargs):
        return self._impl.forward(*args, **kwargs)

    @cq.futurise
    def initialize(self, *args, **kwargs):
        return self._impl.initialize(*args, **kwargs)


async def main():
    # There are sometimes segmentatio fault...
    cq.subscribe(asyncio.get_event_loop())

    model = Model()

    async def task():
        res = await model.forward()
        print(res)

    print(await model.initialize('model-1'))

    await asyncio.gather(
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
    )



asyncio.run(main())




# ie.ntest(np.array([
#     [1, 2, 3, 4, 5],
#     [6, 7, 8, 9, 0]
# ]))
