try:
    import uasyncio as asyncio
except:
    import asyncio

import time


async def neopixel_task():
    while True:
        print("neopixel")
        await asyncio.sleep(1)


async def handle_request(reader, writer):
    while True:
        bytes_in = await reader.readline()
        line = bytes_in.decode().strip()
        print("LINE %s" % line)
        if not line:
            break
    print(writer.get_extra_info("peername"))

    writer.write(b"thank you\r\n")
    await writer.drain()

    writer.close()
    await writer.wait_closed()


async def http_server_task():
    await asyncio.start_server(handle_request, "0.0.0.0", "8080")


async def main():
#    server = await asyncio.start_server(handle_request, "0.0.0.0", "8080")
#    addr = server.sockets[0].getsockname()
#    print("Server on ", addr)

#    async with server:
#        await server.serve_forever()

    asyncio.create_task(neopixel_task())
    asyncio.create_task(http_server_task())
    await asyncio.sleep(30)




#asyncio.run(main())


loop = asyncio.get_event_loop()
loop.create_task(neopixel_task())
loop.create_task(http_server_task())
loop.run_forever()
