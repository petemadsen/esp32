try:
    import uasyncio as asyncio
    import ujson as json
except:
    import asyncio
    import json

import time


class XmasTree:
    def __init__(self):
        self.mode = -1

    def set_mode(self, mode):
        self.mode = mode

    def get_mode(self):
        return self.mode

    def get_status(self):
        return json.dumps({
            "ident": "ahttp",
        })


async def neopixel_task(xmas):
    mode = xmas.mode
    while True:
        new_mode = xmas.get_mode()
        if new_mode != mode:
            mode = new_mode
            print("MODE CHANGE %d" % mode)
        await asyncio.sleep(1)

        if mode == 22:
            print("Mode %d update" % mode)


async def handle_request(reader, writer):
    getline = await reader.readline()
    getline = getline.decode().strip()
    print("LINE %s" % getline)
    while True:
        bytes_in = await reader.readline()
        line = bytes_in.decode().strip()
        print("LINE %s" % line)
        if not line:
            break
    print(writer.get_extra_info("peername"))

    xmas.set_mode(len(getline))

    if " /status " in getline:
        status = xmas.get_status()
        writer.write(status.encode())
    else:
        writer.write(b"ERR\r\n")
    await writer.drain()

    writer.close()
    await writer.wait_closed()


async def http_server_task(xmas, port):
    print("Listening on port %d" % port)
    await asyncio.start_server(handle_request, "0.0.0.0", port)


xmas = XmasTree()

loop = asyncio.get_event_loop()
loop.create_task(neopixel_task(xmas))
loop.create_task(http_server_task(xmas, 8080))
loop.run_forever()
