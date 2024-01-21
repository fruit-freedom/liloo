import asyncio

import liloo
from . import libtest_module

class Model:
    def __init__(self, *args, **kwargs) -> None:
        self._impl = libtest_module.Model(*args, **kwargs)

    @liloo.futurize
    def forward(self, *args, **kwargs):
        return self._impl.forward(*args, **kwargs)

    @liloo.futurize
    def initialize(self, *args, **kwargs):
        return self._impl.initialize(*args, **kwargs)


async def main():
    liloo.global_cq_.subscribe(asyncio.get_event_loop())

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