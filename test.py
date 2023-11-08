import asyncio
import build.asyncie as ie
import os

# q = ie.EventsQueue()


# q.enqueue_task(1, {
#     "hi": 'hello',
#     'what': 1,
#     'data': {
#         'size': 121,
#         'raw': 'safasfsfs'
#     }
# })


class EventLoop:
    def __init__(self) -> None:
        self.futures: dict[int, asyncio.Future] = {}

    def subscribe(self, loop: asyncio.AbstractEventLoop):
        events_fd = ie.get_event_fd()
        print('events_fd', events_fd)

        self.loop = loop
        self.loop.add_reader(events_fd, self._event_callback, events_fd)

    def _event_callback(self, events_fd):
        message = os.read(events_fd, 8)
        print('[py] Recieve event', int.from_bytes(message, byteorder='little'))

        results = ie.get_completed_results()
        if not results:
            print('[py] WARNING get_completed_results() return no completed results')
            return

        print('len:', len(results))
        for event_id, result in results:
            future = self.futures.pop(event_id)
            if not future:
                print(f'[WARNING] event {event_id} have not got a future')
            future.set_result(result)

    # async def execute_task(self, payload):
    #     future = asyncio.Future()
    #     self.event_id_counter += 1
    #     event_id = self.event_id_counter
    #     print(event_id)
    #     self.futures[event_id] = future
    #     self.processor.run_task(event_id, payload)
    #     return await future
    
    def cqawait(self, target_func) -> asyncio.Future:
        def fn(ctx):
            event_id = target_func(ctx)
            if event_id in self.futures:
                raise Exception('event_id already in self.futures')
            future = asyncio.Future()
            self.futures[event_id] = future
            return future

        return fn



cq = EventLoop()


class Model:
    def __init__(self) -> None:
        self._impl = ie.Model()

    @cq.cqawait
    def forward(self):
        return self._impl.forward()



async def main():
    # There are sometimes segmentatio fault...
    cq.subscribe(asyncio.get_event_loop())
    # m = ie.Model()
    # @cq.cqawait
    # def target_func():
    #     return m.forward()

    # result = await target_func()
    # print('Result', result)

    m = Model()

    async def task():
        res = await m.forward()
        print(res)

    # result = await m.forward()
    # print('Result', result)

    # result = await m.forward()
    # print('Result', result)

    # print('Result', await task())
    # print('Result', await task())

    await asyncio.gather(
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
        asyncio.create_task(task()),
    )


asyncio.run(main())